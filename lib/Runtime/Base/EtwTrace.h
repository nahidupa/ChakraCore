//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef ENABLE_JS_ETW

//
// ETW (Event Tracing for Windows) is a high-performance, low overhead and highly scalable
// tracing facility provided by the Windows Operating System. There are
// four main types of components in ETW: event providers, controllers, consumers, and event trace sessions.
// An event provider is a logical entity that writes events to ETW sessions. The event provider must register
// a provider ID with ETW through the registration API. A provider first registers with ETW and writes events
// from various points in the code by invoking the ETW logging API. When a provider is enabled dynamically by
// the ETW controller application, calls to the logging API sends events to a specific trace session
// designated by the controller. Javascript runtime is an event provider.
//
// When ETW's sample profile is enabled it logs the entry point of each function as part of
// stackwalking on the platform. For javascript functions these entry points are not meaningful.
// Therefore, we log events which map the entry point to the name of the function. There are two types of events
// that we log for symbol decoding:
// Rundown: Represents the state of the process and logged when tracing is enabled. Enables the 'attach' scenario.
// Runtime: Represents the change of state of the process and logged as the state changes e.g. when a jitted function gets loaded.
//

// C-style callback
extern "C" {
    void EtwCallback(
        ULONG controlCode,
        PVOID callbackContext);
}

#include "TestEtwEventSink.h"

//
// Represents type of method entry point.
//
enum MethodType : uint16
{
    MethodType_Interpreted = 0x1,
    MethodType_Jit = 0x2,
    MethodType_LoopBody = 0x3,
    MethodType_HostMethod = 0x4,
};

#ifdef TEST_ETW_EVENTS
#define WriteMethodEvent(EventName, ScriptContextID, MethodStartAddress, MethodSize, MethodID, MethodFlags, MethodAddressRangeID, SourceID, Line, Column, MethodName) \
    if(TestEtwEventSink::IsLoaded())   \
    { \
        TestEtwEventSink::Instance->WriteMethodEvent(EventName,  ScriptContextID, MethodStartAddress, MethodSize, MethodID, MethodFlags, MethodAddressRangeID, SourceID, Line, Column, MethodName); \
    }
#define WriteSourceEvent(EventName, SourceContext, ScriptContextID, SourceFlags, Url) \
    if(TestEtwEventSink::IsLoaded())   \
    { \
        TestEtwEventSink::Instance->WriteSourceEvent(EventName, SourceContext, ScriptContextID, SourceFlags, Url); \
    }
#else
#define WriteMethodEvent(Event, ...)
#define WriteSourceEvent(Event, ...)
#endif
// Helper macro to log all the method level events

#define LogSourceEvent(Function, SourceContext, ScriptContext, SourceFlags, Url) \
    JS_ETW(Function(SourceContext,           \
        ScriptContext,                      \
        SourceFlags,                        \
        Url));                              \
                                            \
    WriteSourceEvent(STRINGIZEW(Function),  \
         SourceContext,                     \
         ScriptContext,                     \
         SourceFlags,                       \
         Url);

#define LogMethodNativeEvent(Function, Body, entryPoint)                                            \
    Assert(entryPoint->GetNativeAddress() != NULL);                                                 \
    Assert(entryPoint->GetCodeSize() > 0);                                                          \
    Assert(entryPoint->IsNativeCode());                                                             \
    char16 functionNameArray[NameBufferLength];                                                     \
    const char16 *functionName;                                                                     \
    size_t requiredCharCapacity = 0;                                                                \
    bool deleteFunctionName = false;                                                                \
    const ExecutionMode jitMode = entryPoint->GetJitMode();                                         \
    if(jitMode == ExecutionMode::SimpleJit)                                                         \
    {                                                                                               \
        requiredCharCapacity =                                                                      \
            GetSimpleJitFunctionName(Body, functionNameArray, _countof(functionNameArray));         \
        if(requiredCharCapacity == 0)                                                               \
        {                                                                                           \
            functionName = functionNameArray;                                                       \
        }                                                                                           \
        else                                                                                        \
        {                                                                                           \
            Assert(requiredCharCapacity > NameBufferLength);                                        \
            char16 *const allocatedFunctionName = HeapNewNoThrowArray(char16, requiredCharCapacity);\
            if(allocatedFunctionName)                                                               \
            {                                                                                       \
                const size_t newRequiredCharCapacity =                                              \
                    GetSimpleJitFunctionName(Body, allocatedFunctionName, requiredCharCapacity);    \
                Assert(newRequiredCharCapacity == 0);                                               \
                functionName = allocatedFunctionName;                                               \
                deleteFunctionName = true;                                                          \
            }                                                                                       \
            else                                                                                    \
            {                                                                                       \
                functionNameArray[0] = _u('\0');                                                    \
                functionName = functionNameArray;                                                   \
            }                                                                                       \
        }                                                                                           \
    }                                                                                               \
    else                                                                                            \
    {                                                                                               \
        functionName = GetFunctionName(Body);                                                       \
    }                                                                                               \
    JS_ETW(Function(                                                                                \
        Body->GetScriptContext(),                                                                   \
        (void *)entryPoint->GetNativeAddress(),                                                     \
        entryPoint->GetCodeSize(),                                                                  \
        GetFunctionId(Body),                                                                        \
        0 /* methodFlags - for future use*/,                                                        \
        MethodType_Jit,                                                                             \
        GetSourceId(Body),                                                                          \
        Body->GetLineNumber(),                                                                      \
        Body->GetColumnNumber(),                                                                    \
        functionName));                                                                             \
    WriteMethodEvent(STRINGIZEW(Function),                                                          \
        Body->GetScriptContext(),                                                                   \
        (void *)entryPoint->GetNativeAddress(),                                                     \
        entryPoint->GetCodeSize(),                                                                  \
        GetFunctionId(Body),                                                                        \
        0 /* methodFlags - for future use*/,                                                        \
        MethodType_Jit,                                                                             \
        GetSourceId(Body),                                                                          \
        Body->GetLineNumber(),                                                                      \
        Body->GetColumnNumber(),                                                                    \
        functionName);                                                                              \
    if(deleteFunctionName)                                                                          \
    {                                                                                               \
        HeapDeleteArray(requiredCharCapacity, functionName);                                        \
    }

#define LogMethodInterpretedThunkEvent(Function, Body)                        \
    Assert(Body->GetDynamicInterpreterEntryPoint() != NULL);                  \
    JS_ETW(Function(Body->GetScriptContext(),                                  \
    Body->GetDynamicInterpreterEntryPoint(),                                  \
    Body->GetDynamicInterpreterThunkSize(),                                   \
    GetFunctionId(Body),                                                      \
    0 /* methodFlags - for future use*/,                                      \
    MethodType_Interpreted,                                                   \
    GetSourceId(Body),                                                        \
    Body->GetLineNumber(),                                   \
    Body->GetColumnNumber(),                                 \
    GetFunctionName(Body)));                                                  \
                                                                              \
    WriteMethodEvent(STRINGIZEW(Function),                                    \
    Body->GetScriptContext(),                                                 \
    Body->GetDynamicInterpreterEntryPoint(),                                  \
    Body->GetDynamicInterpreterThunkSize(),                                   \
    GetFunctionId(Body),                                                      \
    0 /* methodFlags - for future use*/,                                      \
    MethodType_Interpreted,                                                   \
    GetSourceId(Body),                                                        \
    Body->GetLineNumber(),                                   \
    Body->GetColumnNumber(),                                 \
    GetFunctionName(Body))


#define LogLoopBodyEvent(Function, Body, entryPoint, loopNumber)                                           \
    Assert(entryPoint->GetNativeAddress() != NULL);                                                        \
    Assert(entryPoint->GetCodeSize() > 0);                                                                 \
    WCHAR loopBodyNameArray[NameBufferLength];                                                             \
    WCHAR* loopBodyName = loopBodyNameArray;                                                               \
    size_t bufferSize = Body->GetLoopBodyName(loopNumber, loopBodyName, NameBufferLength);                 \
    if(bufferSize > NameBufferLength) /* insufficient buffer space*/                                       \
    {                                                                                                      \
        loopBodyName = HeapNewNoThrowArray(WCHAR, bufferSize);                                             \
        if(loopBodyName)                                                                                   \
        {                                                                                                  \
            Body->GetLoopBodyName(loopNumber, loopBodyName, NameBufferLength);                             \
        }                                                                                                  \
        else                                                                                               \
        {                                                                                                  \
            loopBodyNameArray[0] = _u('\0');                                                               \
            loopBodyName = loopBodyNameArray;                                                              \
        }                                                                                                  \
    }                                                                                                      \
    JS_ETW(Function(Body->GetScriptContext(),                                                              \
        (void *)entryPoint->GetNativeAddress(),                                                            \
        entryPoint->GetCodeSize(),                                                                         \
        GetFunctionId(Body),                                                                               \
        0 /* methodFlags - for future use*/,                                                               \
        MethodType_LoopBody + (uint16)loopNumber,                                                          \
        GetSourceId(Body),                                                                                 \
        /*line*/ 0,                                                                                        \
        /*column*/ 0,                                                                                      \
        loopBodyName));                                                                                    \
   WriteMethodEvent(STRINGIZEW(Function),                                                                  \
        Body->GetScriptContext(),                                                                          \
        (void *)entryPoint->GetNativeAddress(),                                                            \
        entryPoint->GetCodeSize(),                                                                         \
        GetFunctionId(Body),                                                                               \
        0 /* methodFlags - for future use*/,                                                               \
        MethodType_LoopBody + (uint16)loopNumber,                                     \
        GetSourceId(Body),                                                                                 \
        /*line*/ 0,                                                                                        \
        /*column*/ 0,                                                                                      \
        loopBodyName);                                                                                     \
    if(loopBodyNameArray != loopBodyName)                                                                  \
    {                                                                                                      \
        HeapDeleteArray(bufferSize, loopBodyName);                                                         \
    }

//
// Encapsulates all ETW event logging and registration related to symbol decoding.
//
class EtwTrace
{
private:
    static const size_t NameBufferLength = 256;

public:
    static void Register();
    static void UnRegister();

    static void PerformRundown(bool start);

    /* Unload events */
    static void LogSourceUnloadEvents(Js::ScriptContext* scriptContext);
    static void LogMethodNativeUnloadEvent(Js::FunctionBody* body, Js::FunctionEntryPointInfo* entryPoint);
    static void LogMethodInterpreterThunkUnloadEvent(Js::FunctionBody* body);
    static void LogLoopBodyUnloadEvent(Js::FunctionBody* body, Js::LoopEntryPointInfo* entryPoint, uint loopNumber);

    /* Load events */
    static void LogMethodInterpreterThunkLoadEvent(Js::FunctionBody* body);
    static void LogMethodNativeLoadEvent(Js::FunctionBody* body, Js::FunctionEntryPointInfo* entryPoint);


    static void LogLoopBodyLoadEvent(Js::FunctionBody* body, Js::LoopEntryPointInfo* entryPoint, uint16 loopNumber);
    static void LogScriptContextLoadEvent(Js::ScriptContext* scriptContext);
    static void LogSourceModuleLoadEvent(Js::ScriptContext* scriptContext, DWORD_PTR sourceContext, _In_z_ const char16* url);


    static const char16* GetFunctionName(Js::FunctionBody* body);
    static size_t GetLoopBodyName(_In_ Js::FunctionBody* body, _In_ Js::LoopHeader* loopHeader, _Out_writes_opt_z_(size) char16* nameBuffer, _In_ size_t size );
    _Success_(return == 0)
    static size_t GetSimpleJitFunctionName(Js::FunctionBody *const body, _Out_writes_opt_z_(nameCharCapacity) char16 *const name, const size_t nameCharCapacity);
    static DWORD_PTR GetSourceId(Js::FunctionBody* body);
    static uint GetFunctionId(Js::FunctionProxy* body);
};

#endif
