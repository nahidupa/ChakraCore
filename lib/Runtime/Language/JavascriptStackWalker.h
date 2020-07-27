//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if defined(_M_IX86)
#include "Language/i386/StackFrame.h"
typedef Js::X86StackFrame StackFrame;
#elif defined(_M_X64)
#include "Language/amd64/StackFrame.h"
#ifdef _WIN32 // xplat-todo
#include "Language/amd64/StackFrame.inl"
#endif
typedef Js::Amd64StackFrame StackFrame;
#elif defined(_M_ARM)
#include "Language/arm/StackFrame.h"
typedef Js::ArmStackFrame StackFrame;
#elif defined(_M_ARM64)
#include "Language/arm64/StackFrame.h"
typedef Js::Arm64StackFrame StackFrame;
#else
#error JavascriptStackWalker is not supported on this architecture.
#endif


namespace Js
{
    struct ScriptEntryExitRecord;

    enum InternalFrameType {
        InternalFrameType_None,
        InternalFrameType_EhFrame,
        InternalFrameType_LoopBody
    };

    struct AsmJsCallStackLayout
    {
        Js::JavascriptFunction * functionObject;
        Js::Var args[0];
    };
    class JavascriptCallStackLayout
    {
    public:
        Js::JavascriptFunction * functionObject;
        Js::CallInfo callInfo;
        Js::Var args[0];
        Js::ArgumentsObject * GetArgumentsObject() const;
        Js::Var * GetArgumentsObjectLocation() const;
        void SetArgumentsObject(Js::ArgumentsObject* obj);
        Js::Var* GetArgv() const;
        Js::Var GetOffset(int offset) const;
        double GetDoubleAtOffset(int offset) const;
        int32 GetInt32AtOffset(int offset) const;
        SIMDValue GetSimdValueAtOffset(int offset) const;
        char * GetValueChangeOffset(int offset) const;
        ForInObjectEnumerator * GetForInObjectEnumeratorArrayAtOffset(int offset) const;

        static JavascriptCallStackLayout *FromFramePointer(void *const framePointer);
        static void* ToFramePointer(JavascriptCallStackLayout* callstackLayout);
    private:
        JavascriptCallStackLayout() : callInfo(0) {};
    };

#if ENABLE_NATIVE_CODEGEN
    /*
     * The InlinedFrameStackWalker knows how to walk an inlinee's virtual frames inside a
     * physical frame. If the stack walker is in the inlineeFramesBeingWalked mode it
     * defers pretty much all functionality to its helpers. The virtual stack frames themselves
     * are laid out in the reverse order on the stack. So we do one pass to find out the count of
     * frames and another to construct an array of pointers to frames in the correct order
     * (top most first like a real stack). Each frame begins with a count. Since frames are laid
     * out continuously in memory, this is all the stack walker needs to find the next frame.
     * We don't maintain explicit prev, next pointers. We also clear the count field of the frame
     * next to the top most frame to indicate that the top most frame is, well, the top most frame.
     * Whenever an inlinee's code ends, the count field in its frame gets set to 0 indicating this
     * frame isn't valid anymore. This keeps the fast case fast and offloads the heavy lifting
     * to the stack walker.
     */
    class InlinedFrameWalker
    {
    public:
        InlinedFrameWalker()
            : parentFunction(nullptr)
            , frames(nullptr)
            , currentIndex(-1)
            , frameCount(0)
        {
        }

        ~InlinedFrameWalker()
        {
            Assert(!parentFunction);
            Assert(!this->frames);
            Assert(!frameCount);
            Assert(currentIndex == -1);
        }

        static bool             FromPhysicalFrame(InlinedFrameWalker& self, StackFrame& physicalFrame, Js::ScriptFunction *parent, bool fromBailout,
                                                  int loopNum, const JavascriptStackWalker * const walker, bool useInternalFrameInfo, bool noAlloc);
        void                    Close();
        bool                    Next(CallInfo& callInfo);
        size_t                  GetArgc() const;
        Js::Var                *GetArgv(bool includeThis, bool boxArgsAndDeepCopy) const;
        Js::JavascriptFunction *GetFunctionObject() const;
        void                    SetFunctionObject(Js::JavascriptFunction * function);
        Js::Var                 GetArgumentsObject() const;
        void                    SetArgumentsObject(Js::Var arguments);
        Js::Var                 GetThisObject() const;
        bool                    IsCallerPhysicalFrame() const;
        bool                    IsTopMostFrame() const;
        int32                   GetFrameIndex() const { Assert(currentIndex != -1); return currentIndex; }
        uint32                  GetCurrentInlineeOffset() const;
        uint32                  GetBottomMostInlineeOffset() const;
        Js::JavascriptFunction *GetBottomMostFunctionObject() const;
        void                    FinalizeStackValues(__in_ecount(argCount) Js::Var args[], size_t argCount, bool deepCopy) const;
        int32                   GetFrameCount() { return frameCount; }

    private:
        enum {
            InlinedFrameArgIndex_This = 0,
            InlinedFrameArgIndex_SecondScriptArg = 1
        };

        struct InlinedFrame : public InlinedFrameLayout
        {
            Js::Var argv[0];    // It's defined here as in C++ can't have 0-size array in the base class.

            struct InlinedFrame *Next()
            {
                InlinedFrameLayout *next = __super::Next();
                return (InlinedFrame*)next;
            }

            static InlinedFrame *FromPhysicalFrame(StackFrame& currentFrame, const JavascriptStackWalker * const stackWalker, void *entry, EntryPointInfo* entryPointInfo, bool useInternalFrameInfo);

        };

    public:
        InlinedFrame *GetFrameAtIndex(signed index) const;

    private:
        void Initialize(int32 frameCount, __in_ecount(frameCount) InlinedFrame **frames, Js::ScriptFunction *parent);
        void MoveNext();
        InlinedFrame *GetCurrentFrame() const;

        Js::ScriptFunction *parentFunction;
        InlinedFrame          **frames;
        int32                   currentIndex;
        int32                   frameCount;
    };

    class InternalFrameInfo
    {
    public:
        void *codeAddress;
        void *framePointer;
        size_t stackCheckCodeHeight;
        InternalFrameType frameType;
        JavascriptFunction* function;
        bool hasInlinedFramesOnStack;
        bool previousInterpreterFrameIsFromBailout;

        InternalFrameInfo() :
            codeAddress(nullptr),
            framePointer(nullptr),
            stackCheckCodeHeight((uint)-1),
            frameType(InternalFrameType_None),
            function(nullptr),
            hasInlinedFramesOnStack(false),
            previousInterpreterFrameIsFromBailout(false)
        {
        }

        void Clear();
        void Set(void *codeAddress, void *framePointer, size_t stackCheckCodeHeight, InternalFrameType frameType, JavascriptFunction* function, bool hasInlinedFramesOnStack, bool previousInterpreterFrameIsFromBailout);
    };
#endif

    class JavascriptStackWalker
    {
        friend Js::ScriptContext;

    public:
        JavascriptStackWalker(ScriptContext * scriptContext, bool useEERContext = TRUE /* use leafinterpreterframe of entryexit record */, PVOID returnAddress = NULL, bool _forceFullWalk = false);
#if ENABLE_NATIVE_CODEGEN
        ~JavascriptStackWalker() { inlinedFrameWalker.Close(); }
#endif
        BOOL Walk(bool includeInlineFrames = true);
        BOOL GetCaller(_Out_opt_ JavascriptFunction ** ppFunc, bool includeInlineFrames = true);
        BOOL GetCallerWithoutInlinedFrames(_Out_opt_ JavascriptFunction ** ppFunc);
        BOOL GetNonLibraryCodeCaller(_Out_opt_ JavascriptFunction ** ppFunc);
        BOOL WalkToTarget(JavascriptFunction * funcTarget);
        BOOL WalkToArgumentsFrame(ArgumentsObject *argsObj);

        uint32 GetByteCodeOffset() const;
        BOOL IsCallerGlobalFunction() const;
        BOOL IsEvalCaller() const;
        bool IsJavascriptFrame() const { return inlinedFramesBeingWalked || isJavascriptFrame; }
        bool IsInlineFrame() const { return inlinedFramesBeingWalked; }
        bool IsBailedOutFromInlinee() const
        {
            Assert(this->IsJavascriptFrame() && this->interpreterFrame);
            return IsInlineFrame();
        }
        bool IsBailedOutFromFunction() const
        {
            Assert(this->IsJavascriptFrame() && this->interpreterFrame);
            return !!JavascriptFunction::IsNativeAddress(this->scriptContext, this->currentFrame.GetInstructionPointer());
        }

        Var GetPermanentArguments() const;

        void *GetCurrentCodeAddr() const;

        JavascriptFunction *GetCurrentFunction(bool includeInlinedFrames = true) const;
        void SetCurrentFunction(JavascriptFunction *  function);
        CallInfo GetCallInfo(bool includeInlinedFrames = true) const;
        CallInfo GetCallInfoFromPhysicalFrame() const;
        void GetThis(Var *pThis, int moduleId) const;
        Js::Var * GetJavascriptArgs(bool boxArgsAndDeepCopy) const;
        void **GetCurrentArgv() const;

        ScriptContext* GetCurrentScriptContext() const;
        InterpreterStackFrame* GetCurrentInterpreterFrame() const
        {
            Assert(this->IsJavascriptFrame());
            return interpreterFrame;
        }

        bool GetSourcePosition(const WCHAR** sourceFileName, ULONG* line, LONG* column);

        static bool TryIsTopJavaScriptFrameNative(ScriptContext* scriptContext, bool* istopFrameNative, bool ignoreLibraryCode = false);
        static bool AlignAndCheckAddressOfReturnAddressMatch(void* addressOfReturnAddress, void* nativeLibraryEntryAddress);

#if ENABLE_NATIVE_CODEGEN
        void ClearCachedInternalFrameInfo();
        void SetCachedInternalFrameInfo(InternalFrameType frameType, JavascriptFunction* function, bool hasInlinedFramesOnStack, bool prevIntFrameIsFromBailout);
        InternalFrameInfo GetCachedInternalFrameInfo() const { return this->lastInternalFrameInfo; }
        void WalkAndClearInlineeFrameCallInfoOnException(void *tryHandlerAddrOfReturnAddr);
#endif
        bool IsCurrentPhysicalFrameForLoopBody() const;

        // noinline, we want to use own stack frame.
        static _NOINLINE BOOL GetCaller(_Out_opt_ JavascriptFunction** ppFunc, ScriptContext* scriptContext);
        static _NOINLINE BOOL GetCaller(_Out_opt_ JavascriptFunction** ppFunc, uint32* byteCodeOffset, ScriptContext* scriptContext);
        static _NOINLINE void GetThis(Var* pThis, int moduleId, ScriptContext* scriptContext);
        static _NOINLINE void GetThis(Var* pThis, int moduleId, JavascriptFunction* func, ScriptContext* scriptContext);

        static bool IsDisplayCaller(JavascriptFunction* func);
        bool GetDisplayCaller(_Out_opt_ JavascriptFunction ** ppFunc);
        PCWSTR GetCurrentNativeLibraryEntryName() const;
        static bool IsLibraryStackFrameEnabled(Js::ScriptContext * scriptContext);
        static bool IsWalkable(ScriptContext *scriptContext);

        // Walk frames (until walkFrame returns true)
        template <class WalkFrame>
        ushort WalkUntil(ushort stackTraceLimit, WalkFrame walkFrame, bool onlyOnDebugMode = false, bool filterDiagnosticsOM = false)
        {
            ushort frameIndex = 0;

            JavascriptFunction* jsFunction;

            BOOL foundCaller = GetNonLibraryCodeCaller(&jsFunction);
            while (foundCaller)
            {
                if (IsDisplayCaller(jsFunction))
                {
                    bool needToPass = (!onlyOnDebugMode || jsFunction->GetScriptContext()->IsScriptContextInDebugMode())
                        && (!filterDiagnosticsOM || !jsFunction->GetScriptContext()->IsDiagnosticsScriptContext());

                    if (needToPass)
                    {
                        if (walkFrame(jsFunction, frameIndex))
                        {
                            break;
                        }
                        frameIndex++;
                    }
                }

                foundCaller = frameIndex < stackTraceLimit && GetCaller(&jsFunction);
            }

            return frameIndex;
        }

        template <class WalkFrame>
        ushort WalkUntil(WalkFrame walkFrame, bool onlyOnDebugMode = false, bool filterDiagnosticsOM = false)
        {
            return WalkUntil(USHORT_MAX, walkFrame, onlyOnDebugMode, filterDiagnosticsOM);
        }
        BYTE** GetCurrentAddresOfReturnAddress() const
        {
            return (BYTE**)this->currentFrame.GetAddressOfReturnAddress();
        }

        BYTE** GetCurrentAddressOfInstructionPointer() const
        {
            return (BYTE**)this->currentFrame.GetAddressOfInstructionPointer();
        }

        void* GetInstructionPointer() const
        {
            return this->currentFrame.GetInstructionPointer();
        }

        void* GetFramePointer() const
        {
            return this->currentFrame.GetFrame();
        }

        bool GetCurrentFrameFromBailout() const
        {
            return previousInterpreterFrameIsFromBailout;
        }

#if DBG
        static bool ValidateTopJitFrame(Js::ScriptContext* scriptContext);
#endif

    private:
        ScriptContext          *scriptContext;
        ScriptEntryExitRecord  *entryExitRecord;
        const NativeLibraryEntryRecord::Entry *nativeLibraryEntry;
        const NativeLibraryEntryRecord::Entry *prevNativeLibraryEntry; // Saves previous nativeLibraryEntry when it moves to next
        InterpreterStackFrame  *interpreterFrame;
        InterpreterStackFrame  *tempInterpreterFrame;
#if ENABLE_NATIVE_CODEGEN
        Js::InlinedFrameWalker  inlinedFrameWalker;
#endif
        CallInfo                inlinedFrameCallInfo;
        bool                    inlinedFramesBeingWalked    : 1;
        bool                    hasInlinedFramesOnStack     : 1;
        bool                    isJavascriptFrame           : 1;
        bool                    isNativeLibraryFrame        : 1;
        bool                    isInitialFrame              : 1; // If we need to walk the initial frame
        bool                    shouldDetectPartiallyInitializedInterpreterFrame : 1;
        bool                    previousInterpreterFrameIsFromBailout : 1;
        bool                    previousInterpreterFrameIsForLoopBody : 1;
        bool                    forceFullWalk               : 1; // ignoring hasCaller

        Var GetThisFromFrame() const;                   // returns 'this' object from the physical frame
        Var GetCurrentArgumentsObject() const;          // returns arguments object from the current frame, which may be virtual (belonging to an inlinee)
        void SetCurrentArgumentsObject(Var args);       // sets arguments object for the current frame, which may be virtual (belonging to an inlinee)
        Var GetCurrentNativeArgumentsObject() const;    // returns arguments object from the physical native frame
        void SetCurrentNativeArgumentsObject(Var args); // sets arguments object on the physical native frame
        bool TryGetByteCodeOffsetFromInterpreterFrame(uint32& offset) const;
#if ENABLE_NATIVE_CODEGEN
        bool TryGetByteCodeOffsetFromNativeFrame(uint32& offset) const;
        bool TryGetByteCodeOffsetOfInlinee(Js::JavascriptFunction* function, uint loopNum, DWORD_PTR pCodeAddr, Js::FunctionBody** inlinee, uint32& offset, bool useInternalFrameInfo) const;
        uint GetLoopNumber(bool& usedInternalFrameInfo) const;
        bool InlinedFramesBeingWalked() const;
        bool HasInlinedFramesOnStack() const;
        bool PreviousInterpreterFrameIsFromBailout() const;
        InternalFrameInfo lastInternalFrameInfo;
#endif
        mutable StackFrame currentFrame;

        Js::JavascriptFunction * UpdateFrame(bool includeInlineFrames);
        bool CheckJavascriptFrame(bool includeInlineFrames);

        JavascriptFunction *GetCurrentFunctionFromPhysicalFrame() const;
     };

    class AutoPushReturnAddressForStackWalker
    {
    private:
        ScriptContext *m_scriptContext;
    public:
        AutoPushReturnAddressForStackWalker(ScriptContext *scriptContext, void* returnAddress) : m_scriptContext(scriptContext)
        {
            scriptContext->SetFirstInterpreterFrameReturnAddress(returnAddress);
        }
        ~AutoPushReturnAddressForStackWalker()
        {
            m_scriptContext->SetFirstInterpreterFrameReturnAddress(NULL);
        }
    };
}
