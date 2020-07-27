//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
class BackwardPass
{
    template <typename T>
    friend class TempTracker;
    friend class NumberTemp;
    friend class ObjectTemp;
#if DBG
    friend class ObjectTempVerify;
#endif
public:
    BackwardPass(Func * func, GlobOpt * globOpt, Js::Phase tag);
    void Optimize();

    static bool DoDeadStore(Func* func, StackSym* sym);

private:
    void CleanupBackwardPassInfoInFlowGraph();
    void OptBlock(BasicBlock * block);
    void MergeSuccBlocksInfo(BasicBlock * block);
    void ProcessLoopCollectionPass(BasicBlock *const lastBlock);
    void ProcessLoop(BasicBlock * lastBlock);
    void ProcessBlock(BasicBlock * block);
    bool IsFormalParamSym(Func * func, Sym * sym) const;
    bool CanDeadStoreInstrForScopeObjRemoval(Sym *sym = nullptr) const;
    void TraceDeadStoreOfInstrsForScopeObjectRemoval();
    IR::Instr * TryChangeInstrForStackArgOpt();
    void InsertArgInsForFormals();
    void ProcessBailOnStackArgsOutOfActualsRange();
    void MarkScopeObjSymUseForStackArgOpt();
    bool DeadStoreOrChangeInstrForScopeObjRemoval(IR::Instr ** pInstrPrev);
    void ProcessUse(IR::Opnd * opnd);
    bool ProcessDef(IR::Opnd * opnd);
    void ProcessTransfers(IR::Instr * instr);
    void ProcessFieldKills(IR::Instr * instr);
    bool SymIsIntconstOrSelf(Sym *sym, IR::Opnd *opnd);
    bool InstrPreservesNumberValues(IR::Instr *instr, Sym *defSym);

    template<typename T> void ClearBucketsOnFieldKill(IR::Instr *instr, HashTable<T> *table);
    StackSym* ProcessByteCodeUsesDst(IR::ByteCodeUsesInstr * byteCodeUsesInstr);
    const BVSparse<JitArenaAllocator>* ProcessByteCodeUsesSrcs(IR::ByteCodeUsesInstr * byteCodeUsesInstr);
    bool ProcessByteCodeUsesInstr(IR::Instr * instr);
    bool ProcessBailOutInfo(IR::Instr * instr);
    void ProcessBailOutInfo(IR::Instr * instr, BailOutInfo * bailOutInfo);
    IR::Instr* ProcessPendingPreOpBailOutInfo(IR::Instr *const currentInstr);
    void ClearDstUseForPostOpLazyBailOut(IR::Instr *instr);
    void ProcessBailOutArgObj(BailOutInfo * bailOutInfo, BVSparse<JitArenaAllocator> * byteCodeUpwardExposedUsed);
    void ProcessBailOutConstants(BailOutInfo * bailOutInfo, BVSparse<JitArenaAllocator> * byteCodeUpwardExposedUsed, BVSparse<JitArenaAllocator>* argSymsBv);
    void ProcessBailOutCopyProps(BailOutInfo * bailOutInfo, BVSparse<JitArenaAllocator> * byteCodeUpwardExposedUsed, BVSparse<JitArenaAllocator>* argSymsBv);
    bool ProcessStackSymUse(StackSym * sym, BOOLEAN isNonByteCodeUse);
    bool ProcessSymUse(Sym * sym, bool isRegOpndUse, BOOLEAN isNonByteCodeUse);
    bool MayPropertyBeWrittenTo(Js::PropertyId propertyId);
    void ProcessPropertySymOpndUse(IR::PropertySymOpnd *opnd);
    bool ProcessPropertySymUse(PropertySym *propertySym);
    void ProcessNewScObject(IR::Instr* instr);
    void DisallowMarkTempAcrossYield(BVSparse<JitArenaAllocator>* bytecodeUpwardExposed);
    void MarkTemp(StackSym * sym);
    bool ProcessInlineeStart(IR::Instr* instr);
    void ProcessInlineeEnd(IR::Instr* instr);
    void MarkTempProcessInstr(IR::Instr * instr);

    void RemoveEmptyLoopAfterMemOp(Loop *loop);
    void RemoveEmptyLoops();
    bool IsEmptyLoopAfterMemOp(Loop *loop);
    void RestoreInductionVariableValuesAfterMemOp(Loop *loop);
    bool DoDeadStoreLdStForMemop(IR::Instr *instr);
    bool DeadStoreInstr(IR::Instr *instr);

    void CollectCloneStrCandidate(IR::Opnd *opnd);
    void InvalidateCloneStrCandidate(IR::Opnd *opnd);
#if DBG_DUMP
    void DumpBlockData(BasicBlock * block, IR::Instr* instr = nullptr);
    void TraceInstrUses(BasicBlock * block, IR::Instr* instr, bool isStart);
    void TraceBlockUses(BasicBlock * block, bool isStart);
    void DumpMarkTemp();
#endif

    static bool UpdateImplicitCallBailOutKind(IR::Instr *const instr, bool needsBailOutOnImplicitCall, bool needsLazyBailOut);

    bool ProcessNoImplicitCallUses(IR::Instr *const instr);
    void ProcessNoImplicitCallDef(IR::Instr *const instr);
    template<class F> IR::Opnd *FindNoImplicitCallUse(IR::Instr *const instr, StackSym *const sym, const F IsCheckedUse, IR::Instr * *const noImplicitCallUsesInstrRef = nullptr);
    template<class F> IR::Opnd *FindNoImplicitCallUse(IR::Instr *const instr, IR::Opnd *const opnd, const F IsCheckedUse, IR::Instr * *const noImplicitCallUsesInstrRef = nullptr);
    void ProcessArrayRegOpndUse(IR::Instr *const instr, IR::ArrayRegOpnd *const arrayRegOpnd);
    void UpdateArrayValueTypes(IR::Instr *const instr, IR::Opnd *opnd);
    void UpdateArrayBailOutKind(IR::Instr *const instr);
    void TrackBitWiseOrNumberOp(IR::Instr *const instr);
    void SetSymIsNotUsedOnlyInBitOps(IR::Opnd *const opnd);
    void SetSymIsUsedOnlyInBitOpsIfLastUse(IR::Opnd *const opnd);
    void SetSymIsNotUsedOnlyInNumber(IR::Opnd *const opnd);
    void SetSymIsUsedOnlyInNumberIfLastUse(IR::Opnd *const opnd);
    void TrackIntUsage(IR::Instr *const instr);
    void RemoveNegativeZeroBailout(IR::Instr* instr);
    void SetNegativeZeroDoesNotMatterIfLastUse(IR::Opnd *const opnd);
    void SetNegativeZeroMatters(IR::Opnd *const opnd);
    void SetCouldRemoveNegZeroBailoutForDefIfLastUse(IR::Opnd *const opnd);
    void SetIntOverflowDoesNotMatterIfLastUse(IR::Opnd *const opnd);
    void SetIntOverflowMatters(IR::Opnd *const opnd);
    bool SetIntOverflowDoesNotMatterInRangeIfLastUse(IR::Opnd *const opnd, const int addSubUses);
    bool SetIntOverflowDoesNotMatterInRangeIfLastUse(StackSym *const stackSym, const int addSubUses);
    void SetIntOverflowMattersInRange(IR::Opnd *const opnd);
    void TransferCompoundedAddSubUsesToSrcs(IR::Instr *const instr, const int addSubUses);
    void EndIntOverflowDoesNotMatterRange();

    void TrackFloatSymEquivalence(IR::Instr *const instr);

    bool IsLazyBailOutCurrentlyNeeeded(IR::Instr * instr) const;
    void DeadStoreImplicitCallBailOut(IR::Instr * instr, bool hasLiveFields, bool needsLazyBailOut);
    void DeadStoreTypeCheckBailOut(IR::Instr * instr);
    void DeadStoreLazyBailOut(IR::Instr * instr, bool needsLazyBailOut);
    bool IsImplicitCallBailOutCurrentlyNeeded(IR::Instr * instr, bool mayNeedImplicitCallBailOut, bool needLazyBailOut, bool hasLiveFields);
    bool NeedBailOutOnImplicitCallsForTypedArrayStore(IR::Instr* instr);
    bool TrackNoImplicitCallInlinees(IR::Instr *instr);
    bool ProcessBailOnNoProfile(IR::Instr *instr, BasicBlock *block);

    bool DoByteCodeUpwardExposedUsed() const;
    bool DoCaptureByteCodeUpwardExposedUsed() const;
    void DoSetDead(IR::Opnd * opnd, bool isDead) const;

    bool SatisfyMarkTempObjectsConditions() const;
    bool DoMarkTempObjects() const;
    bool DoMarkTempNumbers() const;
    bool DoMarkTempNumbersOnTempObjects() const;
#if DBG
    bool DoMarkTempObjectVerify() const;
#endif
    static bool DoDeadStore(Func* func);
    bool DoDeadStore() const;
    bool DoDeadStoreSlots() const;
    bool DoTrackNegativeZero() const;
    bool DoTrackBitOpsOrNumber()const;
    bool DoTrackIntOverflow() const;
    bool DoTrackCompoundedIntOverflow() const;
    bool DoTrackNon32BitOverflow() const;
#if DBG_DUMP
    bool IsTraceEnabled() const;
#endif

    bool IsCollectionPass() const { return isCollectionPass; }
    bool IsPrePass() const { return this->currentPrePassLoop != nullptr; }

    void DeleteBlockData(BasicBlock * block);

    void TrackObjTypeSpecProperties(IR::PropertySymOpnd *opnd, BasicBlock *block);
    void TrackObjTypeSpecWriteGuards(IR::PropertySymOpnd *opnd, BasicBlock *block);
    void TrackAddPropertyTypes(IR::PropertySymOpnd *opnd, BasicBlock *block);
    void InsertTypeTransition(IR::Instr *instrInsertBefore, int symId, AddPropertyCacheBucket *data, BVSparse<JitArenaAllocator>* upwardExposedUses);
    void InsertTypeTransition(IR::Instr *instrInsertBefore, StackSym *objSym, AddPropertyCacheBucket *data, BVSparse<JitArenaAllocator>* upwardExposedUses);
    void InsertTypeTransitionAtBlock(BasicBlock *block, int symId, AddPropertyCacheBucket *data, BVSparse<JitArenaAllocator>* upwardExposedUses);
    void InsertTypeTransitionsAtPriorSuccessors(BasicBlock *block, BasicBlock *blockSucc, int symId, AddPropertyCacheBucket *data, BVSparse<JitArenaAllocator>* upwardExposedUses);
    void InsertTypeTransitionAfterInstr(IR::Instr *instr, int symId, AddPropertyCacheBucket *data, BVSparse<JitArenaAllocator>* upwardExposedUses);
    void InsertTypeTransitionsAtPotentialKills();
    bool TransitionUndoesObjectHeaderInlining(AddPropertyCacheBucket *data) const;

    void SetTypeIDWithFinalType(int symId, BasicBlock *block);
    void ClearTypeIDWithFinalType(int symId, BasicBlock *block);
    bool HasTypeIDWithFinalType(BasicBlock *block) const;
    void CombineTypeIDsWithFinalType(BasicBlock *block, BasicBlock *blockSucc);

    template<class Fn> void ForEachAddPropertyCacheBucket(Fn fn);
    static ObjTypeGuardBucket MergeGuardedProperties(ObjTypeGuardBucket bucket1, ObjTypeGuardBucket bucket2);
    static ObjWriteGuardBucket MergeWriteGuards(ObjWriteGuardBucket bucket1, ObjWriteGuardBucket bucket2);
    bool ReverseCopyProp(IR::Instr *instr);
    bool FoldCmBool(IR::Instr *instr);
    void SetWriteThroughSymbolsSetForRegion(BasicBlock * catchBlock, Region * tryRegion);
    bool CheckWriteThroughSymInRegion(Region * region, StackSym * sym);
#if DBG
    void VerifyByteCodeUpwardExposed(BasicBlock* block, Func* func, BVSparse<JitArenaAllocator>* trackingByteCodeUpwardExposedUsed, IR::Instr* instr, uint32 bytecodeOffset);
    void CaptureByteCodeUpwardExposed(BasicBlock* block, Func* func, Js::OpCode opcode, uint32 offset);
    BVSparse<JitArenaAllocator>* GetByteCodeRegisterUpwardExposed(BasicBlock* block, Func* func, JitArenaAllocator* alloc);
#endif
private:
    // Javascript number values (64-bit floats) have 53 bits excluding the sign bit to precisely represent integers. If we have
    // compounded uses in add/sub, such as:
    //     s1 = s0 + s0
    //     s2 = s1 + s1
    //     s3 = s2 + s2
    //     ...
    // And s0 has a 32-bit (signed or unsigned) int value, then we can do 53 - 32 such add/sub operations and guarantee that the
    // final result does not overflow the 53 bits. So long as that is the case, and the final result is only used in operations
    // that convert their srcs to int32s (such as bitwise operations), then overflow checks can be omitted on these adds/subs.
    // Once the result overflows 53 bits, the semantics of converting that imprecisely represented float value to int32 changes
    // and is no longer equivalent to a simple truncate of the precise int value.
    static const int MaxCompoundedUsesInAddSubForIgnoringIntOverflow = 53 - 32;

    Func * const func;
    GlobOpt * globOpt;
    JitArenaAllocator * tempAlloc;
    Js::Phase tag;
    Loop * currentPrePassLoop;
    BasicBlock * currentBlock;
    Region * currentRegion;
    IR::Instr *  currentInstr;
    IR::Instr * preOpBailOutInstrToProcess;
    BVSparse<JitArenaAllocator> * negativeZeroDoesNotMatterBySymId;
    BVSparse<JitArenaAllocator> * symUsedOnlyForBitOpsBySymId;
    BVSparse<JitArenaAllocator> * symUsedOnlyForNumberBySymId;
    BVSparse<JitArenaAllocator> * intOverflowDoesNotMatterBySymId;
    BVSparse<JitArenaAllocator> * intOverflowDoesNotMatterInRangeBySymId;
    BVSparse<JitArenaAllocator> * candidateSymsRequiredToBeInt;
    BVSparse<JitArenaAllocator> * candidateSymsRequiredToBeLossyInt;
    BVSparse<JitArenaAllocator> * considerSymsAsRealUsesInNoImplicitCallUses;
    bool intOverflowCurrentlyMattersInRange;
    bool isCollectionPass;
    enum class CollectionPassSubPhase
    {
        None,
        FirstPass,
        SecondPass
    } collectionPassSubPhase;
    bool isLoopPrepass;

    class FloatSymEquivalenceClass
    {
    private:
        BVSparse<JitArenaAllocator> bv;
        bool requiresBailOnNotNumber;

    public:
        FloatSymEquivalenceClass(JitArenaAllocator *const allocator) : bv(allocator), requiresBailOnNotNumber(false)
        {
        }

        BVSparse<JitArenaAllocator> *Bv()
        {
            return &bv;
        }

        bool RequiresBailOnNotNumber() const
        {
            return requiresBailOnNotNumber;
        }

        void Set(const StackSym *const sym)
        {
            bv.Set(sym->m_id);
            if(sym->m_requiresBailOnNotNumber)
            {
                requiresBailOnNotNumber = true;
            }
        }

        void Or(const FloatSymEquivalenceClass *const other)
        {
            bv.Or(&other->bv);
            if(other->requiresBailOnNotNumber)
            {
                requiresBailOnNotNumber = true;
            }
        }
    };

    typedef JsUtil::BaseDictionary<SymID, FloatSymEquivalenceClass *, JitArenaAllocator> FloatSymEquivalenceMap;
    FloatSymEquivalenceMap *floatSymEquivalenceMap = nullptr;

    // Use by numberTemp to keep track of the property sym  that is used to represent a property, since we don't trace aliasing
    typedef JsUtil::BaseDictionary<Js::PropertyId, SymID, JitArenaAllocator> NumberTempRepresentativePropertySymMap;
    NumberTempRepresentativePropertySymMap * numberTempRepresentativePropertySym;

#if DBG_DUMP
    uint32 numDeadStore;
    uint32 numMarkTempNumber;
    uint32 numMarkTempNumberTransferred;
    uint32 numMarkTempObject;
#endif

    uint32 implicitCallBailouts;
    uint32 fieldOpts;
};
