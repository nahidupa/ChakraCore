//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#if defined(_M_ARM32_OR_ARM64) || defined(_M_X64)
const int32 AstBytecodeRatioEstimate = 4;
#else
const int32 AstBytecodeRatioEstimate = 5;
#endif


enum DynamicLoadKind
{
    Invalid,
    Local,
    Env,
    LocalWith,
    EnvWith
};

struct DynamicLoadRecord
{
    DynamicLoadRecord();
    DynamicLoadKind kind;
    Js::ByteCodeLabel label;
    union
    {
        uint32 index;
        Js::RegSlot instance;
    };
};

struct JumpCleanupInfo
{
    // Used for loop nodes
    ParseNode* loopNode;
    uint loopId;

    // Used for try and finally nodes
    Js::OpCode tryOp;
    Js::ByteCodeLabel label;
    Js::RegSlot regSlot1;
    Js::RegSlot regSlot2;
};

class ByteCodeGenerator
{
private:
    Js::ScriptContext* scriptContext;
    ArenaAllocator *alloc;
    uint32 flags;
    SList<FuncInfo*> *funcInfoStack;
    ParseNodeBlock *currentBlock;
    ParseNode *currentTopStatement;
    Scope *currentScope;
    Scope *globalScope; // the global members will be in this scope
    Js::ScopeInfo* parentScopeInfo;
    Js::ByteCodeWriter  m_writer;

    // pointer to the root function wrapper that will be invoked by the caller
    Js::ParseableFunctionInfo * pRootFunc;

    SList<FuncInfo*> * funcInfosToFinalize;

    using JumpCleanupList = DList<JumpCleanupInfo, ArenaAllocator>;
    JumpCleanupList* jumpCleanupList;

    int32 maxAstSize;
    uint16 envDepth;
    uint sourceIndex;
    uint dynamicScopeCount;
    uint loopDepth;
    uint16 m_callSiteId;
    uint16 m_callApplyCallSiteCount;
    bool isBinding;
    bool trackEnvDepth;
    bool funcEscapes;
    bool inPrologue;
    bool inDestructuredPattern;
    Parser* parser; // currently active parser (used for AST transformation)

    Js::Utf8SourceInfo *m_utf8SourceInfo;

    // The stack walker won't be able to find the current function being defer parse, pass in
    // The address so we can patch it up if it is a stack function and we need to box it.
    Js::ScriptFunction ** functionRef;
public:
    // This points to the current function body which can be reused when parsing a subtree (called due to deferred parsing logic).
    Js::FunctionBody * pCurrentFunction;

    bool InDestructuredPattern() const { return inDestructuredPattern; }
    void SetInDestructuredPattern(bool in) { inDestructuredPattern = in; }

    bool InPrologue() const { return inPrologue; }
    void SetInPrologue(bool val) { inPrologue = val; }
    Parser* GetParser() { return parser; }
    Js::ParseableFunctionInfo * GetRootFunc(){return pRootFunc;}
    void SetRootFuncInfo(FuncInfo* funcInfo);
    // Treat the return value register like a constant register so that the byte code writer maps it to the bottom
    // of the register range.
    static const Js::RegSlot ReturnRegister = REGSLOT_TO_CONSTREG(Js::FunctionBody::ReturnValueRegSlot);
    static const Js::RegSlot RootObjectRegister = REGSLOT_TO_CONSTREG(Js::FunctionBody::RootObjectRegSlot);
    static const unsigned int DefaultArraySize = 0;  // This __must__ be '0' so that "(new Array()).length == 0"
    static const unsigned int MinArgumentsForCallOptimization = 16;
    bool forceNoNative;

    // A flag that when set will force bytecode opcodes to be emitted in strict mode when avaliable.
    // This flag is set outside of emit calls under the condition that the bytecode being emitted
    // corresponds to computed property names within classes. This fixes a bug where computed property
    // names would not enforce strict mode when inside a class even though the spec requires that
    // all code within a class must be strict.
    bool forceStrictModeForClassComputedPropertyName = false;

    ByteCodeGenerator(Js::ScriptContext* scriptContext, Js::ScopeInfo* parentScopeInfo);

#if DBG_DUMP
    bool Trace() const
    {
        return Js::Configuration::Global.flags.Trace.IsEnabled(Js::ByteCodePhase);
    }
#else
    bool Trace() const
    {
        return false;
    }
#endif

    Js::ScriptContext* GetScriptContext() { return scriptContext; }

    Scope *GetCurrentScope() const { return currentScope; }

    void SetCurrentBlock(ParseNodeBlock *pnode) { currentBlock = pnode; }
    ParseNodeBlock *GetCurrentBlock() const { return currentBlock; }

    void SetCurrentTopStatement(ParseNode *pnode) { currentTopStatement = pnode; }
    ParseNode *GetCurrentTopStatement() const { return currentTopStatement; }

    Js::ModuleID GetModuleID() const
    {
        return m_utf8SourceInfo->GetSrcInfo()->moduleID;
    }

    void SetFlags(uint32 grfscr)
    {
        flags = grfscr;
    }

    uint32 GetFlags(void)
    {
        return flags;
    }

    bool IsConsoleScopeEval(void)
    {
        return (flags & fscrConsoleScopeEval) == fscrConsoleScopeEval;
    }

    bool IsModuleCode()
    {
        return (flags & fscrIsModuleCode) == fscrIsModuleCode;
    }

    bool IsBinding() const {
        return isBinding;
    }

    Js::ByteCodeWriter *Writer() {
        return &m_writer;
    }

    ArenaAllocator *GetAllocator() {
        return alloc;
    }

    bool IsEvalWithNoParentScopeInfo()
    {
        return (flags & fscrEvalCode) && !HasParentScopeInfo();
    }

    Js::ProfileId GetNextCallSiteId(Js::OpCode op)
    {
        if (m_writer.ShouldIncrementCallSiteId(op))
        {
            if (m_callSiteId != Js::Constants::NoProfileId)
            {
                return m_callSiteId++;
            }
        }
        return m_callSiteId;
    }
    Js::ProfileId GetCurrentCallSiteId() { return m_callSiteId; }

    Js::ProfileId GetNextCallApplyCallSiteId(Js::OpCode op)
    {
        if (m_writer.ShouldIncrementCallSiteId(op))
        {
            if (m_callApplyCallSiteCount != Js::Constants::NoProfileId)
            {
                return m_callApplyCallSiteCount++;
            }
        }
        return m_callApplyCallSiteCount;
    }

    Js::RegSlot NextVarRegister();
    Js::RegSlot NextConstRegister();
    FuncInfo *TopFuncInfo() const;

    void EnterLoop();
    void ExitLoop() { loopDepth--; }
    BOOL IsInLoop() const { return loopDepth > 0; }
    // TODO: per-function register assignment for env and global symbols
    void AssignRegister(Symbol *sym);
    void AddTargetStmt(ParseNodeStmt *pnodeStmt);
    Js::RegSlot AssignNullConstRegister();
    Js::RegSlot AssignUndefinedConstRegister();
    Js::RegSlot AssignTrueConstRegister();
    Js::RegSlot AssignFalseConstRegister();
    Js::RegSlot AssignThisConstRegister();
    void SetNeedEnvRegister();
    void AssignFrameObjRegister();
    void AssignFrameSlotsRegister();
    void AssignParamSlotsRegister();
    void AssignFrameDisplayRegister();

    void ProcessCapturedSym(Symbol *sym);
    void ProcessScopeWithCapturedSym(Scope *scope);
    void InitScopeSlotArray(FuncInfo * funcInfo);
    void FinalizeRegisters(FuncInfo * funcInfo, Js::FunctionBody * byteCodeFunction);
    void SetClosureRegisters(FuncInfo * funcInfo, Js::FunctionBody * byteCodeFunction);
    void SetHasTry(bool has);
    void SetHasFinally(bool has);
    void SetNumberOfInArgs(Js::ArgSlot argCount);
    Js::RegSlot EnregisterConstant(unsigned int constant);
    Js::RegSlot EnregisterStringConstant(IdentPtr pid);
    Js::RegSlot EnregisterDoubleConstant(double d);
    Js::RegSlot EnregisterBigIntConstant(ParseNodePtr pid);
    Js::RegSlot EnregisterStringTemplateCallsiteConstant(ParseNode* pnode);

    static Js::JavascriptArray* BuildArrayFromStringList(ParseNode* stringNodeList, uint arrayLength, Js::ScriptContext* scriptContext);

    bool HasParentScopeInfo() const
    {
        return this->parentScopeInfo != nullptr;
    }

    Js::RegSlot EmitLdObjProto(Js::OpCode op, Js::RegSlot objReg, FuncInfo *funcInfo)
    {
        // LdHomeObjProto protoReg, objReg
        // LdFuncObjProto protoReg, objReg
        Js::RegSlot protoReg = funcInfo->AcquireTmpRegister();
        this->Writer()->Reg2(op, protoReg, objReg);
        funcInfo->ReleaseTmpRegister(protoReg);
        return protoReg;
    }

    void RestoreScopeInfo(Js::ScopeInfo *scopeInfo, FuncInfo * func);
    void RestoreOneScope(Js::ScopeInfo * scopeInfo, FuncInfo * func);

    FuncInfo *StartBindGlobalStatements(ParseNodeProg *pnode);
    void AssignPropertyId(Symbol *sym, Js::ParseableFunctionInfo* functionInfo);
    void AssignPropertyId(IdentPtr pid);

    void ProcessCapturedSyms(ParseNode *pnodeFnc);

    void RecordAllIntConstants(FuncInfo * funcInfo);
    void RecordAllStrConstants(FuncInfo * funcInfo);
    void RecordAllBigIntConstants(FuncInfo * funcInfo);
    void RecordAllStringTemplateCallsiteConstants(FuncInfo* funcInfo);

    // For now, this just assigns field ids for the current script.
    // Later, we will combine this information with the global field ID map.
    // This temporary code will not work if a global member is accessed both with and without a LHS.
    void AssignPropertyIds(Js::ParseableFunctionInfo* functionInfo);
    void MapCacheIdsToPropertyIds(FuncInfo *funcInfo);
    void MapReferencedPropertyIds(FuncInfo *funcInfo);
#if ENABLE_NATIVE_CODEGEN
    void MapCallSiteToCallApplyCallSiteMap(FuncInfo * funcInfo);
#endif
    FuncInfo *StartBindFunction(const char16 *name, uint nameLength, uint shortNameOffset, bool* pfuncExprWithName, ParseNodeFnc *pnodeFnc, Js::ParseableFunctionInfo * reuseNestedFunc);
    void EndBindFunction(bool funcExprWithName);
    void StartBindCatch(ParseNode *pnode);

    // Block scopes related functions
    template<class Fn> void IterateBlockScopedVariables(ParseNodeBlock *pnodeBlock, Fn fn);
    void InitBlockScopedContent(ParseNodeBlock *pnodeBlock, Js::DebuggerScope *debuggerScope, FuncInfo *funcInfo);

    Js::DebuggerScope* RecordStartScopeObject(ParseNode *pnodeBlock, Js::DiagExtraScopesType scopeType, Js::RegSlot scopeLocation = Js::Constants::NoRegister, int* index = nullptr);
    void RecordEndScopeObject(ParseNode *pnodeBlock);

    void EndBindCatch();
    void StartEmitFunction(ParseNodeFnc *pnodeFnc);
    void EndEmitFunction(ParseNodeFnc *pnodeFnc);
    void StartEmitBlock(ParseNodeBlock *pnodeBlock);
    void EndEmitBlock(ParseNodeBlock *pnodeBlock);
    void StartEmitCatch(ParseNodeCatch *pnodeCatch);
    void EndEmitCatch(ParseNodeCatch *pnodeCatch);
    void StartEmitWith(ParseNode *pnodeWith);
    void EndEmitWith(ParseNode *pnodeWith);
    void EnsureFncScopeSlots(ParseNode *pnode, FuncInfo *funcInfo);
    void EnsureLetConstScopeSlots(ParseNodeBlock *pnodeBlock, FuncInfo *funcInfo);
    bool EnsureSymbolModuleSlots(Symbol* sym, FuncInfo* funcInfo);
    void EmitAssignmentToDefaultModuleExport(ParseNode* pnode, FuncInfo* funcInfo);
    void EmitModuleExportAccess(Symbol* sym, Js::OpCode opcode, Js::RegSlot location, FuncInfo* funcInfo);

    void PushScope(Scope *innerScope);
    void PopScope();
    void PushBlock(ParseNodeBlock *pnode);
    void PopBlock();

    void PushFuncInfo(char16 const * location, FuncInfo* funcInfo);
    void PopFuncInfo(char16 const * location);

    Js::RegSlot PrependLocalScopes(Js::RegSlot evalEnv, Js::RegSlot tempLoc, FuncInfo *funcInfo);
    Symbol *FindSymbol(Symbol **symRef, IdentPtr pid, bool forReference = false);
    Symbol *AddSymbolToScope(Scope *scope, const char16 *key, int keyLength, ParseNode *varDecl, SymbolType symbolType);
    Symbol *AddSymbolToFunctionScope(const char16 *key, int keyLength, ParseNode *varDecl, SymbolType symbolType);
    void FuncEscapes(Scope *scope);
    void EmitTopLevelStatement(ParseNode *stmt, FuncInfo *funcInfo, BOOL fReturnValue);
    void EmitInvertedLoop(ParseNodeStmt* outerLoop,ParseNodeFor* invertedLoop,FuncInfo* funcInfo);
    void DefineFunctions(FuncInfo *funcInfoParent);
    Js::RegSlot DefineOneFunction(ParseNodeFnc *pnodeFnc, FuncInfo *funcInfoParent, bool generateAssignment=true, Js::RegSlot regEnv = Js::Constants::NoRegister, Js::RegSlot frameDisplayTemp = Js::Constants::NoRegister);
    void DefineCachedFunctions(FuncInfo *funcInfoParent);
    void DefineUncachedFunctions(FuncInfo *funcInfoParent);
    void DefineUserVars(FuncInfo *funcInfo);
    void InitBlockScopedNonTemps(ParseNode *pnode, FuncInfo *funcInfo);
    // temporarily load all constants and special registers in a single block
    void LoadAllConstants(FuncInfo *funcInfo);
    void LoadHeapArguments(FuncInfo *funcInfo);
    void LoadUncachedHeapArguments(FuncInfo *funcInfo);
    void LoadCachedHeapArguments(FuncInfo *funcInfo);
    void LoadThisObject(FuncInfo *funcInfo, bool thisLoadedFromParams = false);
    void EmitThis(FuncInfo *funcInfo, Js::RegSlot lhsLocation, Js::RegSlot fromRegister);
    void LoadNewTargetObject(FuncInfo *funcInfo);
    void LoadImportMetaObject(FuncInfo* funcInfo);
    void LoadSuperObject(FuncInfo *funcInfo);
    void LoadSuperConstructorObject(FuncInfo *funcInfo);
    void EmitSuperCall(FuncInfo* funcInfo, ParseNodeSuperCall * pnodeSuperCall, BOOL fReturnValue, BOOL fEvaluateComponents);
    void EmitClassConstructorEndCode(FuncInfo *funcInfo);

    // TODO: home the 'this' argument
    void EmitLoadFormalIntoRegister(ParseNode *pnodeFormal, Js::RegSlot pos, FuncInfo *funcInfo);
    void HomeArguments(FuncInfo *funcInfo);

    void EnsureNoRedeclarations(ParseNodeBlock *pnodeBlock, FuncInfo *funcInfo);

    void DefineLabels(FuncInfo *funcInfo);
    void EmitProgram(ParseNodeProg *pnodeProg);
    void EmitScopeList(ParseNode *pnode, ParseNode *breakOnBodyScopeNode = nullptr);
    void EmitDefaultArgs(FuncInfo *funcInfo, ParseNodeFnc *pnode);
    void EmitOneFunction(ParseNodeFnc *pnodeFnc);
    void EmitGlobalFncDeclInit(Js::RegSlot rhsLocation, Js::PropertyId propertyId, FuncInfo * funcInfo);
    void EmitLocalPropInit(Js::RegSlot rhsLocation, Symbol *sym, FuncInfo *funcInfo);
    void EmitPropStore(Js::RegSlot rhsLocation, Symbol *sym, IdentPtr pid, FuncInfo *funcInfo, bool isLet = false, bool isConst = false, bool isFncDeclVar = false, bool skipUseBeforeDeclarationCheck = false);
    void EmitPropLoad(Js::RegSlot lhsLocation, Symbol *sym, IdentPtr pid, FuncInfo *funcInfo, bool skipUseBeforeDeclarationCheck = false);
    void EmitPropDelete(Js::RegSlot lhsLocation, Symbol *sym, IdentPtr pid, FuncInfo *funcInfo);
    void EmitPropTypeof(Js::RegSlot lhsLocation, Symbol *sym, IdentPtr pid, FuncInfo *funcInfo);
    void EmitTypeOfFld(FuncInfo * funcInfo, Js::PropertyId propertyId, Js::RegSlot value, Js::RegSlot instance, Js::OpCode op1, bool reuseLoc = false);

    bool ShouldLoadConstThis(FuncInfo* funcInfo);

    void EmitPropLoadThis(Js::RegSlot lhsLocation, ParseNodeSpecialName *pnode, FuncInfo *funcInfo, bool chkUndecl);
    void EmitPropStoreForSpecialSymbol(Js::RegSlot rhsLocation, Symbol *sym, IdentPtr pid, FuncInfo *funcInfo, bool init);

    void EmitLoadInstance(Symbol *sym, IdentPtr pid, Js::RegSlot *pThisLocation, Js::RegSlot *pTargetLocation, FuncInfo *funcInfo);
    void EmitGlobalBody(FuncInfo *funcInfo);
    void EmitFunctionBody(FuncInfo *funcInfo);
    void EmitAsmFunctionBody(FuncInfo *funcInfo);
    void EmitScopeObjectInit(FuncInfo *funcInfo);

    void EmitPatchableRootProperty(Js::OpCode opcode, Js::RegSlot regSlot, Js::PropertyId propertyId, bool isLoadMethod, bool isStore, FuncInfo *funcInfo);

    struct TryScopeRecord;
    JsUtil::DoublyLinkedList<TryScopeRecord> tryScopeRecordsList;
    void EmitLeaveOpCodesBeforeYield();
    void EmitTryBlockHeadersAfterYield();

    void InvalidateCachedOuterScopes(FuncInfo *funcInfo);

    bool InDynamicScope() const { return dynamicScopeCount != 0; }

    Scope * FindScopeForSym(Scope *symScope, Scope *scope, Js::PropertyId *envIndex, FuncInfo *funcInfo) const;

    static Js::OpCode GetStFldOpCode(bool isStrictMode, bool isRoot, bool isLetDecl, bool isConstDecl, bool isClassMemberInit)
    {
        return isClassMemberInit ? Js::OpCode::InitClassMember :
            isConstDecl ? (isRoot ? Js::OpCode::InitRootConstFld : Js::OpCode::InitConstFld) :
            isLetDecl ? (isRoot ? Js::OpCode::InitRootLetFld : Js::OpCode::InitLetFld) :
            isStrictMode ? (isRoot ? Js::OpCode::StRootFldStrict : Js::OpCode::StFldStrict) :
            isRoot ? Js::OpCode::StRootFld : Js::OpCode::StFld;
    }
    static Js::OpCode GetStFldOpCode(FuncInfo* funcInfo, bool isRoot, bool isLetDecl, bool isConstDecl, bool isClassMemberInit, bool forceStrictModeForClassComputedPropertyName = false);
    static Js::OpCode GetScopedStFldOpCode(bool isStrictMode, bool isConsoleScope = false)
    {
        return isStrictMode ? 
            (isConsoleScope ? Js::OpCode::ConsoleScopedStFldStrict : Js::OpCode::ScopedStFldStrict) :
            (isConsoleScope ? Js::OpCode::ConsoleScopedStFld : Js::OpCode::ScopedStFld);
    }
    static Js::OpCode GetScopedStFldOpCode(FuncInfo* funcInfo, bool isConsoleScopeLetConst = false);
    static Js::OpCode GetStElemIOpCode(bool isStrictMode)
    {
        return isStrictMode ? Js::OpCode::StElemI_A_Strict : Js::OpCode::StElemI_A;
    }
    static Js::OpCode GetStElemIOpCode(FuncInfo* funcInfo);

    bool DoJitLoopBodies(FuncInfo *funcInfo) const;

    static void Generate(__in ParseNodeProg *pnode, uint32 grfscr, __in ByteCodeGenerator* byteCodeGenerator, __inout Js::ParseableFunctionInfo ** ppRootFunc, __in uint sourceIndex, __in bool forceNoNative, __in Parser* parser, Js::ScriptFunction ** functionRef);
    void Begin(
        __in ArenaAllocator *alloc,
        __in uint32 grfscr,
        __in Js::ParseableFunctionInfo* pRootFunc);

    void SetCurrentSourceIndex(uint sourceIndex) { this->sourceIndex = sourceIndex; }
    uint GetCurrentSourceIndex() { return sourceIndex; }

    static bool IsFalse(ParseNode* node);

    static bool IsThis(ParseNode* pnode);
    static bool IsSuper(ParseNode* pnode);

    void StartStatement(ParseNode* node);
    void EndStatement(ParseNode* node);
    void StartSubexpression(ParseNode* node);
    void EndSubexpression(ParseNode* node);

    bool IsES6DestructuringEnabled() const;
    bool IsES6ForLoopSemanticsEnabled() const;

    // Debugger methods.
    bool IsInDebugMode() const;
    bool IsInNonDebugMode() const;
    bool ShouldTrackDebuggerMetadata() const;
    void TrackRegisterPropertyForDebugger(Js::DebuggerScope *debuggerScope, Symbol *symbol, FuncInfo *funcInfo, Js::DebuggerScopePropertyFlags flags = Js::DebuggerScopePropertyFlags_None, bool isFunctionDeclaration = false);
    void TrackActivationObjectPropertyForDebugger(Js::DebuggerScope *debuggerScope, Symbol *symbol, Js::DebuggerScopePropertyFlags flags = Js::DebuggerScopePropertyFlags_None, bool isFunctionDeclaration = false);
    void TrackSlotArrayPropertyForDebugger(Js::DebuggerScope *debuggerScope, Symbol* symbol, Js::PropertyId propertyId, Js::DebuggerScopePropertyFlags flags = Js::DebuggerScopePropertyFlags_None, bool isFunctionDeclaration = false);
    void TrackFunctionDeclarationPropertyForDebugger(Symbol *functionDeclarationSymbol, FuncInfo *funcInfoParent);
    void UpdateDebuggerPropertyInitializationOffset(Js::RegSlot location, Js::PropertyId propertyId, bool shouldConsumeRegister = true);

    void PopulateFormalsScope(uint beginOffset, FuncInfo *funcInfo, ParseNodeFnc *pnodeFnc);
    void InsertPropertyToDebuggerScope(FuncInfo* funcInfo, Js::DebuggerScope* debuggerScope, Symbol* sym);
    FuncInfo *FindEnclosingNonLambda();
    static FuncInfo* GetParentFuncInfo(FuncInfo* child);
    FuncInfo* GetEnclosingFuncInfo();

    bool CanStackNestedFunc(FuncInfo * funcInfo, bool trace = false);
    void CheckDeferParseHasMaybeEscapedNestedFunc();
    bool NeedObjectAsFunctionScope(FuncInfo * funcInfo, ParseNodeFnc * pnodeFnc) const;
    bool HasInterleavingDynamicScope(Symbol * sym) const;

    Js::FunctionBody *EnsureFakeGlobalFuncForUndefer(ParseNode *pnode);
    Js::FunctionBody *MakeGlobalFunctionBody(ParseNode *pnode);

    bool NeedScopeObjectForArguments(FuncInfo *funcInfo, ParseNodeFnc *pnodeFnc) const;

    void AddFuncInfoToFinalizationSet(FuncInfo *funcInfo);
    void FinalizeFuncInfos();
    void CheckFncDeclScopeSlot(ParseNodeFnc *pnodeFnc, FuncInfo *funcInfo);
    void EnsureFncDeclScopeSlot(ParseNodeFnc *pnodeFnc, FuncInfo *funcInfo);

    Js::OpCode GetStSlotOp(Scope *scope, int envIndex, Js::RegSlot scopeLocation, bool chkBlockVar, FuncInfo *funcInfo);
    Js::OpCode GetLdSlotOp(Scope *scope, int envIndex, Js::RegSlot scopeLocation, FuncInfo *funcInfo);
    Js::OpCode GetInitFldOp(Scope *scope, Js::RegSlot scopeLocation, FuncInfo *funcInfo, bool letDecl = false);
    
    void PushJumpCleanupForLoop(ParseNode* loopNode, uint loopId)
    {
        this->jumpCleanupList->Prepend({
            loopNode,
            loopId,
            Js::OpCode::Nop,
            0,
            Js::Constants::NoRegister,
            Js::Constants::NoRegister
        });
    }

    void PushJumpCleanupForTry(
        Js::OpCode tryOp,
        Js::ByteCodeLabel label = 0,
        Js::RegSlot regSlot1 = Js::Constants::NoRegister,
        Js::RegSlot regSlot2 = Js::Constants::NoRegister)
    {
        this->jumpCleanupList->Prepend({nullptr, 0, tryOp, label, regSlot1, regSlot2});
    }

    void PopJumpCleanup() { this->jumpCleanupList->RemoveHead(); }
    bool HasJumpCleanup() { return !this->jumpCleanupList->Empty(); }
    void EmitJumpCleanup(ParseNode* target, FuncInfo* funcInfo);

private:
    bool NeedCheckBlockVar(Symbol* sym, Scope* scope, FuncInfo* funcInfo) const;

    Js::OpCode ToChkUndeclOp(Js::OpCode op) const;
};

template<class Fn> void ByteCodeGenerator::IterateBlockScopedVariables(ParseNodeBlock *pnodeBlock, Fn fn)
{
    Assert(pnodeBlock->nop == knopBlock);
    for (auto lexvar = pnodeBlock->pnodeLexVars; lexvar; lexvar = lexvar->AsParseNodeVar()->pnodeNext)
    {
        fn(lexvar);
    }
}

struct ApplyCheck
{
    bool matches;
    bool insideApplyCall;
    bool sawApply;
};
