//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

enum class CallApplyTargetSourceType : byte
{
    None,
    LdFld,
    Callback,
    Other
};

class Inline
{
public:
    Inline(Func *topFunc, 
        InliningHeuristics &heuristics, 
        uint lCount = 0, 
        uint currentInlineeFrameSlot = 0, 
        bool isApplyTargetInliningInProgress = false, 
        Inline * prevFrame = nullptr, 
        IR::Instr * callingInstr = nullptr
    ) :
        topFunc(topFunc), 
        inlineesProcessed(0), 
        currentInlineeFrameSlot(currentInlineeFrameSlot), 
        isInLoop(lCount), 
        inliningHeuristics(heuristics),
        isApplyTargetInliningInProgress(isApplyTargetInliningInProgress), 
        isInInlinedApplyCall(false), 
        prevFrame(prevFrame),
        prevFrameCallInstr(callingInstr)
    {}

    void Optimize();
    static IR::Instr* GetDefInstr(IR::Opnd* linkOpnd);
private:
    Func *topFunc;
    uint inlineesProcessed;
    uint currentInlineeFrameSlot;
    uint currentForInDepth;

    // Indicates if you are in loop, counter can increment beyond 1 for nested inlined functions
    // But for a single function won't increment beyond 1 for nested loops.
    uint isInLoop;

    // Following flag indicates that inlinee is a target function of apply.
    // For example: We are trying to inline init in this.init.apply(this, arguments);
    // We don't support recursively inlining another target function inside init body (such as this.bar.apply(this, arguments))
    // Reason being we will have to patch up the top function actuals recursively in two nested functions and that is not supported.
    bool isApplyTargetInliningInProgress;
    bool isInInlinedApplyCall;

    InliningHeuristics &inliningHeuristics;

    IR::PragmaInstr * lastStatementBoundary;

    Inline * prevFrame;
    IR::Instr * prevFrameCallInstr;

    void Optimize(Func *func, __in_ecount_opt(actuals) IR::Instr *argOuts[] = NULL, Js::ArgSlot actuals = (Js::ArgSlot) - 1, uint recursiveInlineDepth = 0);
    bool TryOptimizeCallInstrWithFixedMethod(IR::Instr *callInstr, const FunctionJITTimeInfo * inlineeInfo, bool isPolymorphic, bool isBuiltIn, bool isCtor, bool isInlined, bool& safeThis,
                                            bool dontOptimizeJustCheck = false, uint i = 0 /*i-th inlinee at a polymorphic call site*/);
    intptr_t TryOptimizeInstrWithFixedDataProperty(IR::Instr *&instr);
    IR::Instr * InlineScriptFunction(IR::Instr *callInstr, const FunctionJITTimeInfo *const inlineeData, const StackSym *symThis, const Js::ProfileId profileId, bool* pIsInlined, IR::Instr * inlineeDefInstr, uint recursiveInlineDepth);
#ifdef ENABLE_DOM_FAST_PATH
    void        InlineDOMGetterSetterFunction(IR::Instr *ldFldInstr, const FunctionJITTimeInfo *const inlineeData, const FunctionJITTimeInfo *const inlinerData);
#endif
    IR::Instr * InlineGetterSetterFunction(IR::Instr *accessorInstr, const FunctionJITTimeInfo *const inlineeData, const StackSym *symCallerThis, const uint inlineCacheIndex, bool isGetter, bool *pIsInlined, uint recursiveInlineDepth);
    IR::Instr * InlineFunctionCommon(IR::Instr *callInstr, bool originalCallTargetOpndIsJITOpt, StackSym* originalCallTargetStackSym, const FunctionJITTimeInfo *funcInfo, Func *inlinee, IR::Instr *instrNext,
                                IR::RegOpnd * returnValueOpnd, IR::Instr *inlineBailoutChecksBeforeInstr, const StackSym *symCallerThis, uint recursiveInlineDepth, bool safeThis = false, bool isApplyTarget = false);
    IR::Instr * SimulateCallForGetterSetter(IR::Instr *accessorInstr, IR::Instr* insertInstr, IR::PropertySymOpnd* methodOpnd, bool isGetter);

    IR::Instr * InlineApply(IR::Instr *callInstr, const FunctionJITTimeInfo * applyData, const FunctionJITTimeInfo * inlinerData, const StackSym *symThis, bool* pIsInlined, uint callSiteId, uint recursiveInlineDepth, uint argsCount);
    IR::Instr * InlineApplyBuiltInTargetWithArray(IR::Instr *callInstr, const FunctionJITTimeInfo * applyInfo, const FunctionJITTimeInfo * builtInInfo, bool * pIsInlined);
    IR::Instr * InlineApplyWithArgumentsObject(IR::Instr * callInstr, IR::Instr * argsObjectArgInstr, const FunctionJITTimeInfo * inlineeInfo);
    IR::Instr * InlineApplyWithoutArrayArgument(IR::Instr *callInstr, const FunctionJITTimeInfo * applyInfo, const FunctionJITTimeInfo * applyTargetInfo);
    bool        InlineApplyScriptTarget(IR::Instr *callInstr, const FunctionJITTimeInfo* inlinerData, const FunctionJITTimeInfo** pInlineeData, const FunctionJITTimeInfo * applyFuncInfo,
                                    const StackSym *symThis, IR::Instr ** returnInstr, uint recursiveInlineDepth, bool isArrayOpndArgumentsObject, uint argsCount);
    void        GetArgInstrsForCallAndApply(IR::Instr* callInstr, IR::Instr** implicitThisArgOut, IR::Instr** explicitThisArgOut, IR::Instr** argumentsOrArrayArgOut, uint &argOutCount);
    _Success_(return != false) bool        TryGetCallApplyAndTargetLdInstrs(IR::Instr * callInstr, _Outptr_result_nullonfailure_ IR::Instr ** callApplyLdInstr, _Outptr_result_nullonfailure_ IR::Instr ** callApplyTargetLdInstr);
    IR::Instr * InlineCall(IR::Instr *callInstr, const FunctionJITTimeInfo * inlineeData, const FunctionJITTimeInfo * inlinerData, const StackSym *symThis, bool* pIsInlined, uint callSiteId, uint recursiveInlineDepth, bool isCallInstanceFunction);
    bool        InlineCallTarget(IR::Instr *callInstr, const FunctionJITTimeInfo* inlinerData, const FunctionJITTimeInfo** pInlineeData, const FunctionJITTimeInfo *callFuncInfo,
                                    const StackSym *symThis, IR::Instr ** returnInstr, uint recursiveInlineDepth, bool isCallInstanceFunction);
    bool        InlineCallBuiltInTarget(IR::Instr *callInstr, const FunctionJITTimeInfo* inlinerData, const FunctionJITTimeInfo* inlineeData, const FunctionJITTimeInfo *callFuncInfo, IR::Instr * callTargetLdInstr, Js::OpCode inlineOpcode,
                                    const StackSym *symThis, uint recursiveInlineDepth, bool isCallInstanceFunction, IR::Instr ** returnInstr);
    void        AdjustArgoutsForCallTargetInlining(IR::Instr* callInstr, IR::Instr** pExplicitThisArgOut, bool isCallInstanceFunction);
    bool        TryGetCallApplyInlineeData(const FunctionJITTimeInfo* inlinerData, IR::Instr * callInstr, IR::Instr * callApplyLdInstr, IR::Instr * callApplyTargetLdInstr, const FunctionJITTimeInfo ** inlineeData, Js::InlineCacheIndex * inlineCacheIndex, 
                                    IR::Instr ** callbackDefInstr, bool isCallInstanceFunction, CallApplyTargetSourceType* targetType);

    bool        InlConstFoldArg(IR::Instr *instr, __in_ecount_opt(callerArgOutCount) IR::Instr *callerArgOuts[], Js::ArgSlot callerArgOutCount);
    bool        InlConstFold(IR::Instr *instr, IntConstType *pValue, __in_ecount_opt(callerArgOutCount) IR::Instr *callerArgOuts[], Js::ArgSlot callerArgOutCount);

    IR::Instr * InlineCallApplyTarget_Shared(IR::Instr *callInstr, bool originalCallTargetOpndIsJITOpt, StackSym* originalCallTargetStackSym, const FunctionJITTimeInfo*const inlineeData,
                    uint inlineCacheIndex, bool safeThis, bool isApplyTarget, CallApplyTargetSourceType targetType, IR::Instr * inlineeDefInstr, uint recursiveInlineDepth, IR::Instr * funcObjCheckInsertInstr);
    bool        SkipCallApplyScriptTargetInlining_Shared(IR::Instr *callInstr, const FunctionJITTimeInfo* inlinerData, const FunctionJITTimeInfo* inlineeData, bool isApplyTarget, bool isCallTarget);
    IR::Instr * TryGetFixedMethodsForBuiltInAndTarget(IR::Instr *callInstr, const FunctionJITTimeInfo* inlinerData, const FunctionJITTimeInfo* inlineeData, const FunctionJITTimeInfo *builtInFuncInfo,
                                IR::Instr* builtInLdInstr, IR::Instr* targetLdInstr, bool& safeThis, bool isApplyTarget, bool isCallback);

    bool        CanInlineBuiltInFunction(IR::Instr *callInstr, const FunctionJITTimeInfo * inlineeData, Js::OpCode inlineCallOpCode, const FunctionJITTimeInfo * inlinerData, Js::BuiltinFunction builtInId, bool isCallApplyTarget);
    IR::Instr * InlineBuiltInFunction(IR::Instr *callInstr, const FunctionJITTimeInfo * inlineeData, Js::OpCode inlineCallOpCode, const FunctionJITTimeInfo * inlinerData, const StackSym *symCallerThis, bool* pIsInlined, uint profileId, uint recursiveInlineDepth, IR::Instr * funcObjCheckInsertInstr);
    IR::Instr * InlineFunc(IR::Instr *callInstr, const FunctionJITTimeInfo *const inlineeData, const uint profileId);
    bool        SplitConstructorCall(IR::Instr *const newObjInstr, const bool isInlined, const bool doneFixedMethodFld, IR::Instr** createObjInstrOut = nullptr, IR::Instr** callCtorInstrOut = nullptr) const;
    bool        SplitConstructorCallCommon(IR::Instr *const newObjInstr, IR::Opnd *const lastArgOpnd, const Js::OpCode newObjOpCode,
        const bool isInlined, const bool doneFixedMethodFld, IR::Instr** createObjInstrOut, IR::Instr** callCtorInstrOut) const;
    IR::Instr * InlinePolymorphicFunction(IR::Instr *callInstr, const FunctionJITTimeInfo * inlinerData, const StackSym *symCallerThis, const Js::ProfileId profileId, bool* pIsInlined, uint recursiveInlineDepth, bool triedUsingFixedMethods = false);
    IR::Instr * InlinePolymorphicFunctionUsingFixedMethods(IR::Instr *callInstr, const FunctionJITTimeInfo * inlinerData, const StackSym *symCallerThis, const Js::ProfileId profileId, IR::PropertySymOpnd* methodValueOpnd, bool* pIsInlined, uint recursiveInlineDepth);

    IR::RegOpnd * GetCallbackFunctionOpnd(IR::Instr * callInstr);
    IR::Instr * TryGetCallbackDefInstr(StackSym * callbackSym);
    IR::Instr * TryGetCallbackDefInstrForCallInstr(IR::Instr * callInstr);
    IR::Instr * TryGetCallbackDefInstrForCallApplyTarget(IR::Instr * callApplyLdInstr);
    IR::Instr * TryGetCallbackDefInstrForCallInstanceFunction(IR::Instr * callInstr);
    bool        TryGetCallApplyCallbackTargetInlineeData(const FunctionJITTimeInfo* inlinerData, IR::Instr * callInstr, IR::Instr * callApplyLdInstr, const FunctionJITTimeInfo ** inlineeData, IR::Instr ** callbackDefInstr, bool isCallInstanceFunction);

    IR::Instr * InlineSpread(IR::Instr *spreadCall);

    void        SetupInlineInstrForCallDirect(Js::BuiltinFunction builtInId, IR::Instr* inlineInstr, IR::Instr* argoutInstr);
    void        WrapArgsOutWithCoerse(Js::BuiltinFunction builtInId, IR::Instr* inlineInstr);
    void        SetupInlineeFrame(Func *inlinee, IR::Instr *inlineeStart, Js::ArgSlot actualCount, IR::Opnd *functionObject);
    void        FixupExtraActualParams(IR::Instr * instr, IR::Instr *argOuts[], IR::Instr *argOutsExtra[], uint index, uint actualCount, Js::ProfileId callSiteId);
    void        RemoveExtraFixupArgouts(IR::Instr* instr, uint argoutRemoveCount, Js::ProfileId callSiteId);
    IR::Instr*  PrepareInsertionPoint(IR::Instr *callInstr, const FunctionJITTimeInfo *funcInfo, IR::Instr *insertBeforeInstr);
    IR::ByteCodeUsesInstr*  EmitFixedMethodOrFunctionObjectChecksForBuiltIns(IR::Instr *callInstr, IR::Instr * funcObjCheckInsertInstr, const FunctionJITTimeInfo * inlineeInfo, bool isPolymorphic, bool isBuiltIn, bool isCtor, bool isInlined);
    Js::ArgSlot MapActuals(IR::Instr *callInstr, __out_ecount(maxParamCount) IR::Instr *argOuts[], Js::ArgSlot formalCount, Func *inlinee, Js::ProfileId callSiteId, bool *stackArgsArgOutExpanded, IR::Instr *argOutsExtra[] = nullptr, Js::ArgSlot maxParamCount = Js::InlineeCallInfo::MaxInlineeArgoutCount);
    uint32      CountActuals(IR::Instr *callIntr);
    void        MapFormals(Func *inlinee, __in_ecount(formalCount) IR::Instr *argOuts[], uint formalCount, uint actualCount, IR::RegOpnd *retOpnd, IR::Opnd * funcObjOpnd, const StackSym *symCallerThis, bool stackArgsArgOutExpanded, bool fixedFunctionSafeThis = false, IR::Instr *argOutsExtra[] = nullptr);
    IR::Instr * DoCheckThisOpt(IR::Instr * instr);
    IR::Instr * RemoveLdThis(IR::Instr *instr);
    bool        GetInlineeHasArgumentObject(Func * inlinee);
    bool        HasArgumentsAccess(IR::Instr * instr, SymID argumentsSymId);
    bool        HasArgumentsAccess(IR::Instr * instr);
    bool        HasArgumentsAccess(IR::Opnd * opnd, SymID argumentsSymId);
    bool        IsArgumentsOpnd(IR::Opnd* opnd,SymID argumentsSymId);
    bool        IsArgumentsOpnd(IR::Opnd* opnd);
    void        Cleanup(IR::Instr *callInstr);
    IR::PropertySymOpnd* GetMethodLdOpndForCallInstr(IR::Instr* callInstr);
    IR::Instr* InsertInlineeBuiltInStartEndTags(IR::Instr* callInstr, uint actualcount, IR::Instr** builtinStartInstr = nullptr);
    bool IsInliningOutSideLoops(){ return topFunc->GetJITFunctionBody()->HasLoops() && isInLoop == 0; }

    uint FillInlineesDataArray(
        const FunctionJITTimeInfo* inlineeJitTimeData,
        _Out_writes_to_(inlineesDataArrayLength, (return >= inlineesDataArrayLength? inlineesDataArrayLength : return))  const FunctionJITTimeInfo ** inlineesDataArray,
        uint inlineesDataArrayLength
        );

    void FillInlineesDataArrayUsingFixedMethods(
        const FunctionJITTimeInfo* inlineeJitTimeData,
        __inout_ecount(inlineesDataArrayLength) const FunctionJITTimeInfo **inlineesDataArray,
        uint inlineesDataArrayLength,
        __inout_ecount(cachedFixedInlineeCount) FixedFieldInfo* fixedFunctionInfoArray,
        uint16 cachedFixedInlineeCount
        );

    // Builds IR for inlinee
    Func * BuildInlinee(JITTimeFunctionBody* funcBody, const FunctionJITTimeInfo * inlineeJITData, const FunctionJITRuntimeInfo * inlineeRuntimeData, Js::RegSlot returnRegSlot, IR::Instr *callInstr, uint recursiveInlineDepth);
    void BuildIRForInlinee(Func *inlinee, JITTimeFunctionBody *funcBody, IR::Instr *callInstr, bool isApplyTarget = false, uint recursiveInlineDepth = 0);
    void InsertStatementBoundary(IR::Instr * instrNext);
    void InsertOneInlinee(IR::Instr* callInstr, IR::RegOpnd* returnValueOpnd,
        IR::Opnd* methodOpnd, const FunctionJITTimeInfo * inlineeJITData, const FunctionJITRuntimeInfo * inlineeRuntimeData, IR::LabelInstr* doneLabel, const StackSym* symCallerThis, bool fixedFunctionSafeThis, uint recursiveInlineDepth);
    void CompletePolymorphicInlining(IR::Instr* callInstr, IR::RegOpnd* returnValueOpnd, IR::LabelInstr* doneLabel, IR::Instr* dispatchStartLabel, IR::Instr* ldMethodFldInstr, IR::BailOutKind bailoutKind);
    uint HandleDifferentTypesSameFunction(__inout_ecount(cachedFixedInlineeCount) FixedFieldInfo* fixedFunctionInfoArray, uint16 cachedFixedInlineeCount);
    void SetInlineeFrameStartSym(Func *inlinee, uint actualCount);
    void CloneCallSequence(IR::Instr* callInstr, IR::Instr* clonedCallInstr);

    void InsertJsFunctionCheck(IR::Instr * callInstr, IR::Instr *insertBeforeInstr, IR::BailOutKind bailOutKind);
    void InsertFunctionObjectCheck(IR::RegOpnd * funcOpnd, IR::Instr *insertBeforeInstr, IR::Instr* bailoutInstr, const FunctionJITTimeInfo *funcInfo);

    void TryResetObjTypeSpecFldInfoOn(IR::PropertySymOpnd* propertySymOpnd);
    void TryDisableRuntimePolymorphicCacheOn(IR::PropertySymOpnd* propertySymOpnd);

    IR::Opnd * ConvertToInlineBuiltInArgOut(IR::Instr * argInstr);
    void GenerateArgOutUse(IR::Instr* argInstr);

    bool    GetIsInInlinedApplyCall() const { return this->isInInlinedApplyCall; }
    void    SetIsInInlinedApplyCall(bool inInlinedApplyCall) { this->isInInlinedApplyCall = inInlinedApplyCall; }

#if defined(ENABLE_DEBUG_CONFIG_OPTIONS)
    static void TraceInlining(const FunctionJITTimeInfo *const inliner, const char16* inlineeName, const char16* inlineeFunctionIdandNumberString, uint inlineeByteCodeCount,
        const FunctionJITTimeInfo* topFunc, uint inlinedByteCodeCount, const FunctionJITTimeInfo *const inlinee, uint callSiteId, bool inLoopBody, uint builtIn = -1);
#endif
};

