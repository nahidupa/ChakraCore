//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// (hintCode, isNotOptimized, hintlevel, description, consequences, suggestion)

PERFHINT_REASON(HasTryBlock,                        true, PerfHintLevels::L1,      _u("Function has try block"),                 _u("Un-optimized JIT code generated for this function"), _u("Move perf sensitive block inside of try to different function"))
PERFHINT_REASON(HasTryBlock_Verbose,                true, PerfHintLevels::VERBOSE, _u("Function has try block"),                 _u("Un-optimized JIT code generated for this function"), _u("Move perf sensitive block inside of try to different function"))
PERFHINT_REASON(SrcIsEval,                          true, PerfHintLevels::L1,      _u("Source is inside eval statement"),        _u("Extra scopes, affect inlining, high overhead in the JIT code"), _u("Check usage of eval statement"))
PERFHINT_REASON(CallsEval,                          true, PerfHintLevels::L1,      _u("Function calls eval statement"),          _u("Extra scopes, affect inlining, high overhead in the JIT code"), _u("Check usage of eval statement"))
PERFHINT_REASON(CallsEval_Verbose,                  true, PerfHintLevels::VERBOSE, _u("Function calls eval statement"),          _u("Extra scopes, affect inlining, high overhead in the JIT code"), _u("Check usage of eval statement"))
PERFHINT_REASON(ChildCallsEval,                     true, PerfHintLevels::VERBOSE, _u("Function's child calls eval statement"),  _u("Extra scopes, affect inlining, high overhead in the JIT code"), _u("Check usage of eval statement"))
PERFHINT_REASON(HasWithBlock,                       true, PerfHintLevels::L1,      _u("Function has with statement"),            _u("Slower lookups, high overhead in the JIT code"), _u("Avoid using with statement"))
PERFHINT_REASON(HeapArgumentsDueToWriteToFormals,   true, PerfHintLevels::L1,      _u("Arguments object not optimized due to write to formals"), _u("Mapping between formals and arguments object cannot be maintained"), _u("Check the usage of write to formals in the function"))
PERFHINT_REASON(HeapArgumentsDueToNonLocalRef,      true, PerfHintLevels::L1,      _u("Arguments object not optimized as there were some nested functions or non-local refs found in nested functions"),  _u("Scope object creation is required in this case"), _u("Check for nested functions and non-local refs inside")  )
PERFHINT_REASON(HeapArgumentsModification,          true, PerfHintLevels::L1,      _u("Modification to arguments"),              _u("Slower lookups, high overhead in the JIT code"), _u("Avoid modification to the arguments"))
PERFHINT_REASON(HeapArgumentsCreated,               true, PerfHintLevels::L1,      _u("Arguments object not optimized"),         _u("Slower lookups, high overhead in the JIT code"), _u("Check the usage of arguments in the function"))
PERFHINT_REASON(PolymorphicInilineCap,              true, PerfHintLevels::L1,      _u("Function has reached polymorphic-inline cap"), _u("This function will not inline more than 4 functions for this call-site."), _u("Check the polymorphic usage of this function"))
