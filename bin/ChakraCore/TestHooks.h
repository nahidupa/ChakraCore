//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef ENABLE_TEST_HOOKS
#include <oaidl.h>
interface ICustomConfigFlags;

#if defined(_WIN32) || defined(_MSC_VER)
#define TESTHOOK_CALL __stdcall
#else
#define TESTHOOK_CALL
#endif // defined(_WIN32) || defined(_MSC_VER)

struct TestHooks
{
    typedef HRESULT(TESTHOOK_CALL *SetConfigFlagsPtr)(int argc, LPWSTR argv[], ICustomConfigFlags* customConfigFlags);
    typedef HRESULT(TESTHOOK_CALL *SetConfigFilePtr)(LPWSTR strConfigFile);
    typedef HRESULT(TESTHOOK_CALL *PrintConfigFlagsUsageStringPtr)(void);
    typedef HRESULT(TESTHOOK_CALL *SetAssertToConsoleFlagPtr)(bool flag);
    typedef HRESULT(TESTHOOK_CALL *SetEnableCheckMemoryLeakOutputPtr)(bool flag);
    typedef void(TESTHOOK_CALL * NotifyUnhandledExceptionPtr)(PEXCEPTION_POINTERS exceptionInfo);
    typedef int(TESTHOOK_CALL *LogicalStringCompareImpl)(const char16* p1, int p1size, const char16* p2, int p2size);

    SetConfigFlagsPtr pfSetConfigFlags;
    SetConfigFilePtr  pfSetConfigFile;
    PrintConfigFlagsUsageStringPtr pfPrintConfigFlagsUsageString;
    SetAssertToConsoleFlagPtr pfSetAssertToConsoleFlag;
    SetEnableCheckMemoryLeakOutputPtr pfSetEnableCheckMemoryLeakOutput;
    LogicalStringCompareImpl pfLogicalCompareStringImpl;

    // Javasscript Bigint hooks
    typedef digit_t(TESTHOOK_CALL *AddDigit)(digit_t a, digit_t b, digit_t* carry);
    typedef digit_t(TESTHOOK_CALL *SubDigit)(digit_t a, digit_t b, digit_t* borrow);
    typedef digit_t(TESTHOOK_CALL *MulDigit)(digit_t a, digit_t b, digit_t* high);
    AddDigit pfAddDigit;
    SubDigit pfSubDigit;
    MulDigit pfMulDigit;

#define FLAG(type, name, description, defaultValue, ...) FLAG_##type##(name)
#define FLAG_String(name) \
    bool (TESTHOOK_CALL *pfIsEnabled##name##Flag)(); \
    HRESULT (TESTHOOK_CALL *pfGet##name##Flag)(BSTR *flag); \
    HRESULT (TESTHOOK_CALL *pfSet##name##Flag)(BSTR flag);
#define FLAG_Boolean(name) \
    bool (TESTHOOK_CALL *pfIsEnabled##name##Flag)(); \
    HRESULT (TESTHOOK_CALL *pfGet##name##Flag)(bool *flag); \
    HRESULT (TESTHOOK_CALL *pfSet##name##Flag)(bool flag);
#define FLAG_Number(name) \
    bool (TESTHOOK_CALL *pfIsEnabled##name##Flag)(); \
    HRESULT (TESTHOOK_CALL *pfGet##name##Flag)(int *flag); \
    HRESULT (TESTHOOK_CALL *pfSet##name##Flag)(int flag);
    // skipping other types
#define FLAG_Phases(name)
#define FLAG_NumberSet(name)
#define FLAG_NumberPairSet(name)
#define FLAG_NumberTrioSet(name)
#define FLAG_NumberRange(name)
#include "ConfigFlagsList.h"
#undef FLAG
#undef FLAG_String
#undef FLAG_Boolean
#undef FLAG_Number
#undef FLAG_Phases
#undef FLAG_NumberSet
#undef FLAG_NumberPairSet
#undef FLAG_NumberTrioSet
#undef FLAG_NumberRange

    NotifyUnhandledExceptionPtr pfnNotifyUnhandledException;
};

typedef HRESULT(__stdcall *OnChakraCoreLoadedPtr)(TestHooks &testHooks);
HRESULT OnChakraCoreLoaded(OnChakraCoreLoadedPtr pfChakraCoreLoaded = NULL);

#endif
