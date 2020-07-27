//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "JavascriptNativeOperators.h"

class Func;
class BasicBlock;
class Region;
class Lowerer;
class IRBuilder;
class IRBuilderAsmJs;
class FlowGraph;
class GlobOpt;
class BailOutInfo;
class GeneratorBailInInfo;
class SCCLiveness;

struct LazyBailOutRecord;

typedef JsUtil::KeyValuePair<StackSym *, BailoutConstantValue> ConstantStackSymValue;
typedef JsUtil::KeyValuePair<StackSym *, StackSym*> CopyPropSyms;


struct CapturedValues
{
    SListBase<ConstantStackSymValue> constantValues;           // Captured constant values during glob opt
    SListBase<CopyPropSyms> copyPropSyms;                      // Captured copy prop values during glob opt
    BVSparse<JitArenaAllocator> * argObjSyms;                  // Captured arg object symbols during glob opt
    uint refCount;

    CapturedValues() : argObjSyms(nullptr), refCount(0) {}
    ~CapturedValues()
    {
        // Reset SListBase to be exception safe. Captured values are from GlobOpt->func->alloc
        // in normal case the 2 SListBase are empty so no Clear needed, also no need to Clear in exception case
        constantValues.Reset();
        copyPropSyms.Reset();
        argObjSyms = nullptr;
        Assert(refCount == 0);
    }

    uint DecrementRefCount()
    {
        Assert(refCount != 0);
        return --refCount;
    }

    void IncrementRefCount()
    {
        Assert(refCount > 0);
        refCount++;
    }

    void CopyTo(JitArenaAllocator *allocator, CapturedValues *other) const
    {
        Assert(other != nullptr);
        this->constantValues.CopyTo(allocator, other->constantValues);
        this->copyPropSyms.CopyTo(allocator, other->copyPropSyms);

        if (other->argObjSyms != nullptr)
        {
            other->argObjSyms->ClearAll();
            JitAdelete(allocator, other->argObjSyms);
        }

        if (this->argObjSyms != nullptr)
        {
            other->argObjSyms = this->argObjSyms->CopyNew(allocator);
        }
        else
        {
            other->argObjSyms = nullptr;
        }

        // Ignore refCount because other objects might still reference it
    }
};

class LoweredBasicBlock;

class BranchJumpTableWrapper
{
public:

    BranchJumpTableWrapper(uint tableSize) : jmpTable(nullptr), defaultTarget(nullptr), labelInstr(nullptr), tableSize(tableSize)
    {
    }

    void** jmpTable;
    void* defaultTarget;
    IR::LabelInstr * labelInstr;
    int tableSize;

    static BranchJumpTableWrapper* New(JitArenaAllocator * allocator, uint tableSize)
    {
        BranchJumpTableWrapper * branchTargets = JitAnew(allocator, BranchJumpTableWrapper, tableSize);

        //Create the jump table for integers

        void* * jmpTable = JitAnewArrayZ(allocator, void*, tableSize);
        branchTargets->jmpTable = jmpTable;
        return branchTargets;
    }
};

namespace IR {

class EntryInstr;
class ExitInstr;
class BranchInstr;
class LabelInstr;
class JitProfilingInstr;
class ProfiledInstr;
class ProfiledLabelInstr;
class MultiBranchInstr;
class PragmaInstr;
class ByteCodeUsesInstr;
class GeneratorBailInInstr;

class Opnd;
class RegOpnd;
class IndirOpnd;
class SymOpnd;
class MemRefOpnd;
class PropertySymOpnd;
enum  AddrOpndKind : BYTE;

enum IRKind : BYTE {
    InstrKindInvalid,
    InstrKindInstr,
    InstrKindBranch,
    InstrKindLabel,
    InstrKindProfiled,
    InstrKindProfiledLabel,
    InstrKindEntry,
    InstrKindExit,
    InstrKindPragma,
    InstrKindByteCodeUses,
    InstrKindJitProfiling,
};

const int32 InvalidInstrLayout = -1;

///---------------------------------------------------------------------------
///
/// class Instr
///     BranchInstr
///     MultiBranchInstr
///     LabelInstr
///     JitProfilingInstr
///     ProfiledInstr
///     EntryInstr
///     ExitInstr
///     PragmaInstr
///     BailoutInstr
///     ByteCodeUsesInstr
///     GeneratorBailInInstr
///---------------------------------------------------------------------------

class Instr
{
protected:
    Instr(bool hasBailOutInfo = false) :
        m_next(nullptr),
        m_prev(nullptr),
        m_opcode(Js::OpCode::MaxByteSizedOpcodes),
        m_func(nullptr),
        m_number(Js::Constants::NoByteCodeOffset),
        m_dst(nullptr),
        m_src1(nullptr),
        m_src2(nullptr),
#if DBG_DUMP
        globOptInstrString(nullptr),
#endif
        dstIsTempNumber(false),
        dstIsTempNumberTransferred(false),
        dstIsTempObject(false),
        isCloned(false),
        hasBailOutInfo(hasBailOutInfo),
        hasAuxBailOut(false),
        forcePreOpBailOutIfNeeded(false),
        usesStackArgumentsObject(false),
        isInlineeEntryInstr(false),
        ignoreNegativeZero(false),
        dstIsAlwaysConvertedToInt32(false),
        dstIsAlwaysConvertedToNumber(false),
        ignoreIntOverflow(false),
        ignoreIntOverflowInRange(false),
        loadedArrayHeadSegment(false),
        loadedArrayHeadSegmentLength(false),
        extractedUpperBoundCheckWithoutHoisting(false),
        ignoreOverflowBitCount(32),
        isCtorCall(false),
        isCallInstrProtectedByNoProfileBailout(false),
        hasSideEffects(false),
        isNonFastPathFrameDisplay(false),
        isSafeToSpeculate(false)
#if DBG
        , highlight(0)
        , m_noLazyHelperAssert(false)
#endif
    {
    }
public:
    static Instr *  New(Js::OpCode opcode, Func *func);
    static Instr *  New(Js::OpCode opcode, Func *func, IR::Instr * bytecodeOffsetInstr);
    static Instr *  New(Js::OpCode opcode, Opnd *dstOpnd, Func *func);
    static Instr *  New(Js::OpCode opcode, Opnd *dstOpnd, Opnd *src1Opnd, Func *func);
    static Instr *  New(Js::OpCode opcode, Opnd *dstOpnd, Opnd *src1Opnd, Opnd *src2Opnd, Func *func);
    static Instr*   NewConstantLoad(IR::RegOpnd* dstOpnd, intptr_t varConst, ValueType type, Func *func, Js::Var varLocal = nullptr);

public:
    bool            IsPlainInstr() const;
    bool            IsEntryInstr() const;
    EntryInstr *    AsEntryInstr();
    bool            IsExitInstr() const;
    ExitInstr *     AsExitInstr();
    bool            IsBranchInstr() const;
    BranchInstr *   AsBranchInstr();
    bool            IsLabelInstr() const;
    LabelInstr *    AsLabelInstr();
    bool            IsGeneratorBailInInstr() const;
    GeneratorBailInInstr * AsGeneratorBailInInstr();

    bool            IsJitProfilingInstr() const;
    JitProfilingInstr * AsJitProfilingInstr();
    bool            IsProfiledInstr() const;
    ProfiledInstr * AsProfiledInstr();
    bool            IsProfiledLabelInstr() const;
    ProfiledLabelInstr * AsProfiledLabelInstr();
    bool            IsPragmaInstr() const;
    PragmaInstr *   AsPragmaInstr();
    bool            IsByteCodeUsesInstr() const;
    ByteCodeUsesInstr * AsByteCodeUsesInstr();
    bool            IsLowered() const;
    bool            IsRealInstr() const;
    bool            IsInlined() const;
    bool            IsNewScObjectInstr() const;
    bool            IsInvalidInstr() const;
    Instr*          GetInvalidInstr();

    bool            IsLinked() const { return this->m_prev != nullptr || this->m_next != nullptr; }

    bool            StartsBasicBlock() const;
    bool            EndsBasicBlock() const;
    bool            HasFallThrough() const;
    bool            DoStackArgsOpt() const;
    bool            HasAnyLoadHeapArgsOpCode();
    bool            IsEqual(IR::Instr *instr) const;

    bool            IsCloned() const { return isCloned; }
    void            SetIsCloned(bool isCloned) { this->isCloned = isCloned; }

    bool            IsSafeToSpeculate() const { return isSafeToSpeculate; }
    void            SetIsSafeToSpeculate(bool isSafe) { this->isSafeToSpeculate = isSafe; }

    bool            HasBailOutInfo() const { return hasBailOutInfo; }
    bool            HasAuxBailOut() const { return hasAuxBailOut; }
    bool            HasTypeCheckBailOut() const;
    bool            HasEquivalentTypeCheckBailOut() const;
    bool            HasBailOnNoProfile() const;
    void            ClearBailOutInfo();
    bool            IsDstNotAlwaysConvertedToInt32() const;
    bool            IsDstNotAlwaysConvertedToNumber() const;
    bool            ShouldCheckForNegativeZero() const;
    bool            ShouldCheckForIntOverflow() const;
    bool            ShouldCheckFor32BitOverflow() const;
    bool            ShouldCheckForNon32BitOverflow() const;
    static bool     OpndHasAnyImplicitCalls(IR::Opnd* opnd, bool isSrc);
    bool            HasAnyImplicitCalls() const;
    bool            HasAnySideEffects() const;
    bool            AreAllOpndInt64() const;

    IRKind          GetKind() const;
    Opnd *          GetDst() const;
    Opnd *          SetDst(Opnd * newDst);
    Opnd *          SetFakeDst(Opnd * newDst);
    Opnd *          UnlinkDst();
    void            FreeDst();
    Opnd *          Replace(Opnd *oldOpnd, Opnd *newOpnd);
    Opnd *          DeepReplace(Opnd *const oldOpnd, Opnd *const newOpnd);
    Opnd *          ReplaceDst(Opnd * newDst);
    Instr *         SinkDst(Js::OpCode assignOpcode, RegNum regNum = RegNOREG, IR::Instr *insertAfterInstr = nullptr);
    Instr *         SinkDst(Js::OpCode assignOpcode, StackSym * stackSym, RegNum regNum = RegNOREG, IR::Instr *insertAfterInstr = nullptr);
    Instr *         SinkInstrBefore(IR::Instr * instrTarget);
    Opnd *          GetSrc1() const;
    Opnd *          SetSrc1(Opnd * newSrc);
    Opnd *          UnlinkSrc1();
    void            FreeSrc1();
    Opnd *          ReplaceSrc1(Opnd * newSrc);
    Instr *         HoistSrc1(Js::OpCode assignOpcode, RegNum regNum = RegNOREG, StackSym *newSym = nullptr);
    Opnd *          GetSrc2() const;
    Opnd *          SetSrc2(Opnd * newSrc);
    Opnd *          UnlinkSrc2();
    void            FreeSrc2();
    Opnd *          ReplaceSrc2(Opnd * newSrc);
    Instr *         HoistSrc2(Js::OpCode assignOpcode, RegNum regNum = RegNOREG, StackSym *newSym = nullptr);
    IndirOpnd *     HoistMemRefAddress(MemRefOpnd *const memRefOpnd, const Js::OpCode loadOpCode);
    Opnd *          UnlinkSrc(Opnd *src);
    Opnd *          ReplaceSrc(Opnd *oldSrc, Opnd * newSrc);
    void            InsertBefore(Instr *instr);
    void            InsertAfter(Instr *instr);
    void            InsertRangeBefore(Instr *startInstr, Instr *endInstr);
    void            InsertMultipleBefore(Instr *endInstr);
    void            InsertRangeAfter(Instr *startInstr, Instr *endInstr);
    void            InsertMultipleAfter(Instr *endInstr);
    void            Unlink();
    void            Free();
    void            Remove();
    void            SwapOpnds();
    void            TransferTo(Instr * instr);
    void            TransferDstAttributesTo(Instr * instr);
    IR::Instr *     Copy(bool copyDst = true);
    IR::Instr *     CopyWithoutDst();
    IR::Instr *     Clone();
    IR::Instr *     ConvertToBailOutInstr(IR::Instr *bailOutTarget, BailOutKind kind, uint32 bailOutOffset = Js::Constants::NoByteCodeOffset);
    IR::Instr *     ConvertToBailOutInstr(BailOutInfo *bailOutInfo, BailOutKind kind, bool useAuxBailout = false);
    IR::Instr *     ConvertToBailOutInstrWithBailOutInfoCopy(BailOutInfo *bailOutInfo, IR::BailOutKind bailOutKind);
#if DBG
    IR::LabelInstr *GetNextNonEmptyLabel() const;
#endif
    IR::Instr *     GetNextRealInstr() const;
    IR::Instr *     GetNextRealInstrOrLabel() const;
    IR::Instr *     GetNextBranchOrLabel() const;
    IR::Instr *     GetNextByteCodeInstr() const;
    IR::Instr *     GetPrevRealInstr() const;
    IR::Instr *     GetPrevRealInstrOrLabel() const;
    IR::LabelInstr *GetPrevLabelInstr() const;
    IR::Instr *     GetBlockStartInstr() const;
    IR::Instr *     GetInsertBeforeByteCodeUsesInstr();
    bool            IsByteCodeUsesInstrFor(IR::Instr * instr) const;
    IR::LabelInstr *GetOrCreateContinueLabel(const bool isHelper = false);
    static bool     HasSymUseSrc(StackSym *sym, IR::Opnd*);
    static bool     HasSymUseDst(StackSym *sym, IR::Opnd*);
    bool            HasSymUse(StackSym *sym);
    static bool     HasSymUseInRange(StackSym *sym, Instr *instrBegin, Instr *instrEnd);
    RegOpnd *       FindRegDef(StackSym *sym);
    static Instr*   FindSingleDefInstr(Js::OpCode opCode, Opnd* src);
    bool            CanAggregateByteCodeUsesAcrossInstr(IR::Instr * instr);

    bool            DontHoistBailOnNoProfileAboveInGeneratorFunction() const;

    // LazyBailOut
    bool            AreAllOpndsTypeSpecialized() const;
    bool            IsStFldVariant() const;
    bool            IsStElemVariant() const;
    bool            CanChangeFieldValueWithoutImplicitCall() const;
    void            ClearLazyBailOut();
    bool            OnlyHasLazyBailOut() const;
    bool            HasLazyBailOut() const;
    bool            HasPreOpBailOut() const;
    bool            HasPostOpBailOut() const;
#if DBG
    bool            m_noLazyHelperAssert;
#endif

    BranchInstr *   ChangeCmCCToBranchInstr(LabelInstr *targetInstr);
    static void     MoveRangeAfter(Instr * instrStart, Instr * instrLast, Instr * instrAfter);
    static IR::Instr * CloneRange(Instr * instrStart, Instr * instrLast, Instr * instrInsert, Lowerer *lowerer, JitArenaAllocator *alloc, bool (*fMapTest)(IR::Instr*), bool clonedInstrGetOrigArgSlot);

    bool            CanHaveArgOutChain() const;
    bool            HasEmptyArgOutChain(IR::Instr** startCallInstrOut = nullptr);
    bool            HasFixedFunctionAddressTarget() const;
    // Return whether the instruction transfer value from the src to the dst for copy prop
    bool            TransfersSrcValue();

#if ENABLE_DEBUG_CONFIG_OPTIONS
    const char *    GetBailOutKindName() const;
#endif

#if DBG_DUMP
    virtual void    Dump(IRDumpFlags flags);
            void    Dump();
            void    DumpSimple();
    char16*        DumpString();
    void            DumpGlobOptInstrString();
    void            Dump(int window);
    void            DumpRange(Instr *instrEnd);
    void            DumpByteCodeOffset();
#endif
#if ENABLE_DEBUG_CONFIG_OPTIONS
    void            DumpTestTrace();
    void            DumpFieldCopyPropTestTrace(bool inLandingPad);
#endif
    uint32          GetByteCodeOffset() const;
    uint32          GetNumber() const;

    void            SetByteCodeOffset(IR::Instr * instr);
    void            ClearByteCodeOffset();

    BailOutInfo *   GetBailOutInfo() const;
    BailOutInfo *   UnlinkBailOutInfo();
    void            ReplaceBailOutInfo(BailOutInfo *newBailOutInfo);
    IR::Instr *     ShareBailOut();
    BailOutKind     GetBailOutKind() const;
    BailOutKind     GetBailOutKindNoBits() const;
    BailOutKind     GetAuxBailOutKind() const;
    void            SetBailOutKind(const IR::BailOutKind bailOutKind);
    void            SetAuxBailOutKind(const IR::BailOutKind bailOutKind);
    void            PromoteAuxBailOut();
    void            ResetAuxBailOut();
    void            UnlinkStartCallFromBailOutInfo(IR::Instr *endInstr) const;
    void            ChangeEquivalentToMonoTypeCheckBailOut();
    intptr_t        TryOptimizeInstrWithFixedDataProperty(IR::Instr ** pInstr, GlobOpt* globopt);
    Opnd *          FindCallArgumentOpnd(const Js::ArgSlot argSlot, IR::Instr * *const ownerInstrRef = nullptr);
    void            CopyNumber(IR::Instr *instr) { this->SetNumber(instr->GetNumber()); }

    bool            FetchOperands(_Out_writes_(argsOpndLength) IR::Opnd **argsOpnd, uint argsOpndLength);
    template <typename Fn>
    bool            ForEachCallDirectArgOutInstrBackward(Fn fn, uint argsOpndLength) const;
    bool            IsCmCC_A();
    bool            IsCmCC_R8();
    bool            IsCmCC_I4();
    bool            IsNeq();
    bool            BinaryCalculator(IntConstType src1Const, IntConstType src2Const, IntConstType *pResult, IRType type);
    template <typename T>     
    bool            BinaryCalculatorT(T src1Const, T src2Const, int64 *pResult, bool checkWouldTrap);
    bool            UnaryCalculator(IntConstType src1Const, IntConstType *pResult, IRType type);
    IR::Instr*      GetNextArg();
#if DBG
    bool            ShouldEmitIntRangeCheck();
#endif
    // Iterates argument chain
    template<class Fn>
    bool IterateArgInstrs(Fn callback)
    {
        StackSym* linkSym = this->GetSrc2()->GetStackSym();
        Assert(linkSym->IsSingleDef());
        IR::Instr *argInstr = linkSym->m_instrDef;
        IR::Instr* nextArg = nullptr;
        do
        {
            // Get the next instr before calling 'callback' since callback might modify the IR.
            if (argInstr->GetSrc2() && argInstr->GetSrc2()->IsSymOpnd())
            {
                linkSym = argInstr->GetSrc2()->AsSymOpnd()->m_sym->AsStackSym();
                Assert(linkSym->IsArgSlotSym());

                // Due to dead code elimination in FGPeeps, it is possible for the definitions of the
                // the instructions that we are visiting during FG to have been freed. In this case,
                // the ArgSlot, even though its was a single def, will report IsSingleDef() as false
                // since instrDef is reset to nullptr when the def instr is freed
                Assert(linkSym->IsSingleDef() ||
                    (m_func->IsInPhase(Js::Phase::FGPeepsPhase) || m_func->IsInPhase(Js::Phase::FGBuildPhase)));
                nextArg = linkSym->GetInstrDef();
            }
            else
            {
                nextArg = nullptr;
            }
            if(argInstr->m_opcode == Js::OpCode::ArgOut_A_InlineSpecialized)
            {
                argInstr = nextArg;
                // This is a fake ArgOut, skip it
                continue;
            }
            if (argInstr->m_opcode == Js::OpCode::StartCall)
            {
                Assert(nextArg == nullptr);
                break;
            }

            if(callback(argInstr))
            {
                return true;
            }
            argInstr = nextArg;
        } while(argInstr && !argInstr->IsInvalidInstr());

        // If an instr in the call sequence is invalid (0xFDFDFDFD), it must have been freed.
        // This is possible if some dead-code-removal/peeps code removed only part of the call sequence, while the whole sequence was dead (TH Bug 594245).
        // We allow this possibility here, while relying on the more involved dead-code-removal to remove the rest of the call sequence.
        // Inserting the opcode InvalidOpCode, with no lowering, here to safeguard against the possibility of a dead part of the call sequence not being removed. The lowerer would assert then.
        if (argInstr && argInstr->IsInvalidInstr())
        {
            this->InsertBefore(Instr::New(Js::OpCode::InvalidOpCode, this->m_func));
        }
        return false;
    }

    // Iterates all meta args for inlinee
    template<class Fn>
    bool IterateMetaArgs(Fn callback)
    {
        Assert(this->m_opcode == Js::OpCode::InlineeStart);
        Instr* currentInstr = this;
        while(currentInstr->m_opcode != Js::OpCode::InlineeMetaArg)
        {
            currentInstr = currentInstr->m_prev;
        }
        // backward iteration
        while (currentInstr->m_prev->m_opcode == Js::OpCode::InlineeMetaArg)
        {
            currentInstr = currentInstr->m_prev;
        }

        // forward iteration
        while(currentInstr->m_opcode == Js::OpCode::InlineeMetaArg)
        {
            // cache next instr as callback might move meta arg.
            IR::Instr* nextInstr = currentInstr->m_next;
            if(callback(currentInstr))
            {
                return true;
            }
            currentInstr = nextInstr;
        }
        return false;
    }

    IR::Instr* GetBytecodeArgOutCapture();
    void       GenerateBytecodeArgOutCapture();
    bool       HasByteCodeArgOutCapture();
    void       GenerateArgOutSnapshot();
    IR::Instr* GetArgOutSnapshot();
    FixedFieldInfo* GetFixedFunction() const;
    uint       GetArgOutCount(bool getInterpreterArgOutCount);
    uint       GetArgOutSize(bool getInterpreterArgOutCount);
    uint       GetAsmJsArgOutSize();
    IR::PropertySymOpnd *GetPropertySymOpnd() const;
    bool       CallsAccessor(IR::PropertySymOpnd * methodOpnd = nullptr);
    bool       CallsGetter();
    bool       CallsSetter();
    bool       UsesAllFields();
    void       MoveArgs(bool generateByteCodeCapture = false);
    void       Move(IR::Instr* insertInstr);
private:
    int             GetOpndCount() const;
    void            ClearNumber() { this->m_number = 0; }
    void            SetNumber(uint32 number);
    friend class ::Func;
    friend class ::Lowerer;
    friend class IR::ByteCodeUsesInstr;
    friend class ::SCCLiveness;

    void            SetByteCodeOffset(uint32 number);
    friend class ::IRBuilder;
    friend class ::IRBuilderAsmJs;
    friend class ::FlowGraph;

    void            SetBailOutKind_NoAssert(const IR::BailOutKind bailOutKind);

public:

#ifdef BAILOUT_INJECTION
    uint            bailOutByteCodeLocation;
#endif
    Instr *         m_next;
    Instr *         m_prev;
    Func *          m_func;
#if DBG_DUMP
    char16 *       globOptInstrString;
#endif
    // These should be together to pack into a uint32
    Js::OpCode      m_opcode;
    uint8           ignoreOverflowBitCount;      // Number of bits after which ovf matters. Currently used for MULs.

    // used only for SIMD Ld/St from typed arrays.
    // we keep these here to avoid increase in number of opcodes and to not use ExtendedArgs
    uint8           dataWidth;
#if DBG
    WORD            highlight;
#endif


    bool            isFsBased : 1; // TEMP : just for BS testing
    bool            dstIsTempNumber : 1;
    bool            dstIsTempNumberTransferred : 1;
    bool            dstIsTempObject : 1;
    bool            usesStackArgumentsObject: 1;
    // An inlinee entry instruction initializes the InlineeCallInfo on the frame.
    bool            isInlineeEntryInstr: 1;
    bool            ignoreNegativeZero: 1;
    bool            ignoreIntOverflow: 1;
    bool            ignoreIntOverflowInRange: 1;
    bool            forcePreOpBailOutIfNeeded: 1;
    bool            loadedArrayHeadSegment : 1;
    bool            loadedArrayHeadSegmentLength : 1;
    bool            extractedUpperBoundCheckWithoutHoisting : 1;
    bool            isCtorCall : 1;
    bool            dstIsAlwaysConvertedToInt32 : 1;
    bool            dstIsAlwaysConvertedToNumber : 1;
    bool            isCallInstrProtectedByNoProfileBailout : 1;
    bool            hasSideEffects : 1; // The instruction cannot be dead stored
    bool            isNonFastPathFrameDisplay : 1;
protected:
    bool            isCloned : 1;
    bool            hasBailOutInfo : 1;
    bool            isSafeToSpeculate : 1;

    // Used for aux bail out. We are using same bailOutInfo, just different boolean to hide regular bail out.
    // Refer to ConvertToBailOutInstr implementation for details.
    bool            hasAuxBailOut:1;
    IRKind          m_kind;

    uint32          m_number;
    Opnd *          m_dst;
    Opnd *          m_src1;
    Opnd *          m_src2;

    void Init(Js::OpCode opcode, IRKind kind, Func * func);
    IR::Instr *     CloneInstr() const;
};

class ByteCodeUsesInstr : public Instr
{
private:
    BVSparse<JitArenaAllocator> * byteCodeUpwardExposedUsed;
    
public:
    static ByteCodeUsesInstr * New(IR::Instr * originalBytecodeInstr);
    static ByteCodeUsesInstr * New(Func * containingFunction, uint32 offset);
    const BVSparse<JitArenaAllocator> * GetByteCodeUpwardExposedUsed() const;

    PropertySym *              propertySymUse;
    // In the case of instances where you would like to add a ByteCodeUses to some sym,
    // which doesn't have an operand associated with it (like a block closure sym), use
    // this to set it without needing to pass the check for JIT-Optimized registers.
    void SetNonOpndSymbol(uint symId);
    // In cases where the operand you're working on may be changed between when you get
    // access to it and when you determine that you can set it in the ByteCodeUsesInstr
    // set method, cache the values and use this caller.
    void SetRemovedOpndSymbol(bool isJITOptimizedReg, uint symId);
    void Set(IR::Opnd * originalOperand);
    void Clear(uint symId);
    // Set the byteCodeUpwardExposedUsed bitvector on a new ByteCodeUses instruction.
    void SetBV(BVSparse<JitArenaAllocator>* newbv);
    // If possible, we want to aggregate with subsequent ByteCodeUses Instructions, so
    // that we can do some optimizations in other places where we can simplify args in
    // a compare, but still need to generate them for bailouts. Without this, we cause
    // problems because we end up with an instruction losing atomicity in terms of its
    // bytecode use and generation lifetimes.
    void AggregateFollowingByteCodeUses();

private:
    void Aggregate(ByteCodeUsesInstr * byteCodeUsesInstr);
};

class JitProfilingInstr : public Instr
{
public:
    static JitProfilingInstr * New(Js::OpCode opcode, Opnd *dstOpnd, Opnd *src1Opnd, Func * func);
    static JitProfilingInstr * New(Js::OpCode opcode, Opnd *dstOpnd, Opnd *src1Opnd, Opnd *src2Opnd, Func * func);

    JitProfilingInstr* CloneJitProfiling() const;
    JitProfilingInstr* CopyJitProfiling() const;

    Js::ProfileId profileId;
    Js::ProfileId arrayProfileId;
    union
    {
        Js::InlineCacheIndex inlineCacheIndex;
        uint loopNumber;
    };
    bool isProfiledReturnCall : 1;
    bool isBeginSwitch : 1;
    bool isNewArray : 1;
    bool isLoopHelper: 1;
};

class ProfiledInstr: public Instr
{
protected:
    ProfiledInstr(bool hasBailOutInfo = false) : Instr(hasBailOutInfo) {}
public:
    static ProfiledInstr * New(Js::OpCode opcode, Opnd *dstOpnd, Opnd *src1Opnd, Func * func);
    static ProfiledInstr * New(Js::OpCode opcode, Opnd *dstOpnd, Opnd *src1Opnd, Opnd *src2Opnd, Func * func);

    ProfiledInstr *CloneProfiledInstr() const;
    ProfiledInstr *CopyProfiledInstr() const;

    union
    {
    public:
        uint                    profileId;
        const Js::LdElemInfo *  ldElemInfo;
        const Js::StElemInfo *  stElemInfo;
    private:
        struct
        {
            Js::FldInfo::TSize      fldInfoData;
            Js::LdLenInfo::TSize    ldLenInfoData;
        };

    public:
        Js::FldInfo &FldInfo()
        {
            return reinterpret_cast<Js::FldInfo &>(fldInfoData);
        }
        Js::LdLenInfo & LdLenInfo()
        {
            return reinterpret_cast<Js::LdLenInfo &>(ldLenInfoData);
        }
    } u;

    static const uint InvalidProfileId = (uint)-1;
};

#if TARGET_64
// Ensure that the size of the union doesn't exceed the size of a 64 bit pointer.
CompileAssert(sizeof(ProfiledInstr::u) <= sizeof(void*));
#endif

///---------------------------------------------------------------------------
///
/// class EntryInstr
///
///---------------------------------------------------------------------------

class EntryInstr: public Instr
{
public:
    static EntryInstr * New(Js::OpCode opcode, Func *func);
};

///---------------------------------------------------------------------------
///
/// class ExitInstr
///
///---------------------------------------------------------------------------

class ExitInstr: public Instr
{
public:
    static ExitInstr * New(Js::OpCode opcode, Func *func);
};


///---------------------------------------------------------------------------
///
/// class LabelInstr
///
///---------------------------------------------------------------------------

class LabelInstr : public Instr
{
    friend class BranchInstr;
    friend class IRBuilder;
    friend class IRBuilderAsmJs;
    friend class MultiBranchInstr;


public:
    LabelInstr(JitArenaAllocator * allocator) : Instr(), labelRefs(allocator), m_isLoopTop(false), m_block(nullptr), isOpHelper(false),
        m_hasNonBranchRef(false), m_region(nullptr), m_loweredBasicBlock(nullptr), m_isDataLabel(false), m_isForInExit(false)
#if DBG
        , m_noHelperAssert(false)
        , m_name(nullptr)
#endif
    {
#if DBG_DUMP
        m_id = 0;
#endif
        m_pc.pc = nullptr;
    }

    static LabelInstr *     New(Js::OpCode opcode, Func *func, bool isOpHelper = false);

public:
    SListCounted<BranchInstr *>  labelRefs;
    Lifetime **             m_regContent;

    BYTE                    m_isLoopTop : 1;
    BYTE                    isOpHelper : 1;
    BYTE                    m_hasNonBranchRef : 1;
    BYTE                    m_isDataLabel : 1;

    // Indicate whether the label is the target of a for in loop exit (BrOnEmpty or BrOnNotEmpty)
    // It is used by Inliner to track inlinee for in loop level to assign stack allocated for in 
    // This bit has unknown validity outside of inliner
    BYTE                    m_isForInExit : 1;
#if DBG
    BYTE                    m_noHelperAssert : 1;
#endif
    unsigned int            m_id;
    LoweredBasicBlock*      m_loweredBasicBlock;
#if DBG
    const char16*           m_name;
#endif
private:
    union labelLocation
    {
        BYTE *                  pc;     // Used by encoder and is the real pc offset
        uintptr_t               offset; // Used by preEncoder and is an estimation pc offset, not accurate
    } m_pc;

    BasicBlock *            m_block;
    Loop *                  m_loop;
    Region *                m_region;
public:

    inline void             SetPC(BYTE * pc);
    inline BYTE *           GetPC(void) const;
    inline void             SetOffset(uintptr_t offset);
    inline void             ResetOffset(uintptr_t offset);
    inline uintptr_t        GetOffset(void) const;
    inline void             SetBasicBlock(BasicBlock * block);
    inline BasicBlock *     GetBasicBlock(void) const;
    inline void             SetLoop(Loop *loop);
    inline Loop *           GetLoop(void) const;
    inline void             UnlinkBasicBlock(void);
    inline void             SetRegion(Region *);
    inline Region *         GetRegion(void) const;
    inline BOOL             IsUnreferenced(void) const;
    inline BOOL             IsGeneratorEpilogueLabel(void) const;

    LabelInstr *            CloneLabel(BOOL fCreate);

#if DBG_DUMP

    virtual void            Dump(IRDumpFlags flags) override;

#endif

private:
    void                    AddLabelRef(BranchInstr *branchRef);
    void                    RemoveLabelRef(BranchInstr *branchRef);

protected:
    void                    Init(Js::OpCode opcode, IRKind kind, Func *func, bool isOpHelper);
};

#if DBG
#define LABELNAMESET(label, name) do { label->m_name = _u(name); } while(false)
#define LABELNAME(label) do { label->m_name = _u(#label); } while(false)
#else
#define LABELNAMESET(label, name)
#define LABELNAME(label)
#endif

class ProfiledLabelInstr: public LabelInstr
{
private:
    ProfiledLabelInstr(JitArenaAllocator * allocator);

public:
    static ProfiledLabelInstr *     New(Js::OpCode opcode, Func *func, Js::ImplicitCallFlags flags, Js::LoopFlags loopFlags);

    Js::ImplicitCallFlags loopImplicitCallFlags;
    Js::LoopFlags loopFlags;
#if DBG_DUMP
    uint loopNum;
#endif
};
///---------------------------------------------------------------------------
///
/// class BranchInstr
///
///---------------------------------------------------------------------------

class BranchInstr : public Instr
{

public:
    bool                 m_isAirlock : 1;
    bool                 m_isSwitchBr : 1;
    bool                 m_isOrphanedLeave : 1; // A Leave in a loop body in a try, most likely generated because of a return statement.
    bool                 m_areCmpRegisterFlagsUsedLater : 1; // Indicate that this branch is not the only instr using the register flags set by cmp
    bool                 m_brFinallyToEarlyExit : 1; // BrOnException from finally to early exit, can be turned into BrOnNoException on break blocks removal
#if DBG
    bool                 m_isMultiBranch;
    bool                 m_isHelperToNonHelperBranch;
    bool                 m_leaveConvToBr;
#endif

public:
    static BranchInstr * New(Js::OpCode opcode, LabelInstr * branchTarget, Func *func);
    static BranchInstr * New(Js::OpCode opcode, LabelInstr * branchTarget, Opnd *srcOpnd, Func *func);
    static BranchInstr * New(Js::OpCode opcode, Opnd* destOpnd, LabelInstr * branchTarget, Opnd *srcOpnd, Func *func);
    static BranchInstr * New(Js::OpCode opcode, LabelInstr * branchTarget, Opnd *src1Opnd, Opnd *src2Opnd, Func *func);

    BranchInstr(bool hasBailOutInfo = false) : Instr(hasBailOutInfo), m_branchTarget(nullptr), m_isAirlock(false), m_isSwitchBr(false), m_isOrphanedLeave(false), m_areCmpRegisterFlagsUsedLater(false), m_brFinallyToEarlyExit(false)
    {
#if DBG
        m_isMultiBranch = false;
        m_isHelperToNonHelperBranch = false;
        m_leaveConvToBr = false;
#endif
    }

    void                SetTarget(LabelInstr *labelInstr);      // Only used for non-multi-branch

    bool                ReplaceTarget(LabelInstr * oldLabelInstr, LabelInstr * newLabelInstr);
    void                ClearTarget();

    LabelInstr *        GetTarget() const;
    bool                IsConditional() const;
    bool                IsUnconditional() const;
    void                Invert();
    void                RetargetClonedBranch();
    BranchInstr *       CloneBranchInstr() const;
    bool                IsMultiBranch() const;
    MultiBranchInstr *  AsMultiBrInstr();
    void                SetByteCodeReg(Js::RegSlot reg) { m_byteCodeReg = reg; }
    Js::RegSlot         GetByteCodeReg() { return m_byteCodeReg; }
    bool                HasByteCodeReg() { return m_byteCodeReg != Js::Constants::NoRegister; }
    bool                IsLoopTail(Func * func);

public:
    Lifetime **         m_regContent;

private:
    LabelInstr *        m_branchTarget;
    Js::RegSlot         m_byteCodeReg;
};

///---------------------------------------------------------------------------
///
/// class MultiBranchInstr
///
///---------------------------------------------------------------------------
class MultiBranchInstr : public BranchInstr
{
private:
    /*
        The value field in the dictionary has different semantics at different points of time. Hence the 'value' field is implemented as a void *.
    In IR Layer:
        Offset is stored in the dictionary until we generate the Labels in InsertLabels().
    LabelInstr is stored in the dictionary, after we generate the LabelInstrs.
    In Encoder:
        After the fixup, actual machine address corresponding to the LabelInstr is stored as the 'value'.
    */

private:
    typedef JITJavascriptString* TBranchKey;
    typedef Js::BranchDictionaryWrapper<TBranchKey> BranchDictionaryWrapper;
    typedef BranchDictionaryWrapper::BranchDictionary BranchDictionary;
    typedef BranchJumpTableWrapper BranchJumpTable;

    void * m_branchTargets;                     // can point to a dictionary or a jump table

public:
    static MultiBranchInstr * New(Js::OpCode opcode, IR::Opnd * srcOpnd, Func *func);
    static MultiBranchInstr * New(Js::OpCode opcode, Func *func);
    enum Kind
    {
        IntJumpTable,
        StrDictionary,
        SingleCharStrJumpTable,
    };
    Kind m_kind;
    IntConstType m_baseCaseValue;
    IntConstType m_lastCaseValue;

    MultiBranchInstr() :
        m_branchTargets(nullptr),
        m_kind(IntJumpTable),
        m_baseCaseValue(0),
        m_lastCaseValue(0)
    {
#if DBG
        m_isMultiBranch = true;
#endif
    }

    void                            AddtoDictionary(uint32 offset, TBranchKey key, void* remoteVar);
    void                            AddtoJumpTable(uint32 offset, uint32 jmpIndex);
    void                            CreateBranchTargetsAndSetDefaultTarget(int dictionarySize, Kind kind, uint defaultTargetOffset);
    void                            ChangeLabelRef(LabelInstr * oldTarget, LabelInstr * newTarget);
    bool                            ReplaceTarget(IR::LabelInstr * oldLabelInstr, IR::LabelInstr * newLabelInstr);
    void                            FixMultiBrDefaultTarget(uint32 targetOffset);
    void                            ClearTarget();
    BranchDictionaryWrapper *       GetBranchDictionary();
    BranchJumpTable *               GetBranchJumpTable();


///---------------------------------------------------------------------------
///
/// template MapMultiBrLabels
/// - Maps through the branchTargets dictionary for all the labelInstrs
///---------------------------------------------------------------------------
    template<class Fn>
    void MapMultiBrLabels(Fn fn)
    {
        MapMultiBrTargetByAddress([fn](void ** value) -> void
        {
            fn((LabelInstr*) *value);
        });
    }

///---------------------------------------------------------------------------
///
/// template MapUniqueMultiBrLabels
/// - Maps through the branchTargets dictionary for all unique labelInstrs
///---------------------------------------------------------------------------
    template<class Fn>
        void MapUniqueMultiBrLabels(Fn fn)
    {
        BVSparse<JitArenaAllocator> visitedTargets(m_func->m_alloc);
        MapMultiBrLabels([&](IR::LabelInstr *const targetLabel)
        {
            if(visitedTargets.Test(targetLabel->m_id))
            {
                return;
            }
            visitedTargets.Set(targetLabel->m_id);
            fn(targetLabel);
        });
    }

///--------------------------------------------------------------------------------------------
///
/// template UpdateMultiBrTargetOffsets
/// - Maps through the branchTargets dictionary for updating the target offset by returning the target offset.
///--------------------------------------------------------------------------------------------
    template<class Fn>
    void UpdateMultiBrTargetOffsets(Fn fn)
    {
        MapMultiBrTargetByAddress([fn](void ** value) -> void
        {
            *value = (void*)fn(::Math::PointerCastToIntegral<uint32>(*value));
        });
    }

///--------------------------------------------------------------------------------------------
///
/// template UpdateMultiBrLabels
/// - Maps through the branchDictionary for updating the labelInstr
///--------------------------------------------------------------------------------------------
    template<class Fn>
    void UpdateMultiBrLabels(Fn fn)
    {
        MapMultiBrTargetByAddress([fn](void ** value) -> void
        {
            IR::LabelInstr * oldLabelInstr = (LabelInstr*)*value;
            IR::LabelInstr * newLabelInstr = fn(oldLabelInstr);

            *value = (void*)newLabelInstr;
        });
    }

///-------------------------------------------------------------------------------------------------------------
///
/// template MapMultiBrTargetByAddress
/// - Maps through the branchDictionary accessing the address of the 'value'
///-------------------------------------------------------------------------------------------------------------
    template<class Fn>
    void MapMultiBrTargetByAddress(Fn fn)
    {
        if(!m_branchTargets)
        {
            return;
        }

        void ** defaultTarget = nullptr;

        switch (m_kind)
        {
        case StrDictionary:
        {
            BranchDictionary& branchDictionary = GetBranchDictionary()->dictionary;

            defaultTarget = &(((MultiBranchInstr::BranchDictionaryWrapper*)(m_branchTargets))->defaultTarget);

            branchDictionary.MapAddress([fn](TBranchKey key, void ** value)
            {
                fn(value);
            });
            break;
        }
        case IntJumpTable:
        case SingleCharStrJumpTable:
        {
            void ** branchJumpTable = GetBranchJumpTable()->jmpTable;
            defaultTarget = &(GetBranchJumpTable()->defaultTarget);

            for (IntConstType i = m_baseCaseValue; i <= m_lastCaseValue; i++)
            {
                fn(&branchJumpTable[i - m_baseCaseValue]);
            }
            break;
        }
        default:
            Assert(false);
        };
        fn(defaultTarget);
    }
};




///---------------------------------------------------------------------------
///
/// class PragmaInstr
///
///---------------------------------------------------------------------------

class PragmaInstr : public Instr
{
public:
    uint32               m_statementIndex;
    uint32               m_offsetInBuffer; // offset in the binary code buffer

public:
    static PragmaInstr * New(Js::OpCode opcode, uint32 index, Func *func);

    PragmaInstr() : Instr(), m_statementIndex(0)
    {
    }

#if DBG_DUMP

    virtual void            Dump(IRDumpFlags flags) override;

#endif
#if DBG_DUMP | defined(VTUNE_PROFILING)
    void Record(uint32 nativeBufferOffset);
#endif
    PragmaInstr * ClonePragma();
    PragmaInstr * CopyPragma();
};

class GeneratorBailInInstr : public LabelInstr
{
private:
    GeneratorBailInInstr(JitArenaAllocator* allocator, IR::Instr* yieldInstr) :
        LabelInstr(allocator),
        yieldInstr(yieldInstr),
        upwardExposedUses(allocator)
    {
        Assert(yieldInstr != nullptr && yieldInstr->m_opcode == Js::OpCode::Yield);
    }

public:
    IR::Instr* yieldInstr;
    CapturedValues capturedValues;
    BVSparse<JitArenaAllocator> upwardExposedUses;

    static GeneratorBailInInstr* New(IR::Instr* yieldInstr, Func* func);
};

template <typename InstrType>
class BailOutInstrTemplate : public InstrType
{
private:
    BailOutInstrTemplate() : InstrType(true) {}
public:
    static BailOutInstrTemplate * New(Js::OpCode opcode, BailOutKind kind, IR::Instr * bailOutTarget, Func * func);
    static BailOutInstrTemplate * New(Js::OpCode opcode, IR::Opnd *dst, BailOutKind kind, IR::Instr * bailOutTarget, Func * func);
    static BailOutInstrTemplate * New(Js::OpCode opcode, IR::Opnd *dst, IR::Opnd *src1, BailOutKind kind, IR::Instr * bailOutTarget, Func * func);
    static BailOutInstrTemplate * New(Js::OpCode opcode, IR::Opnd *dst, IR::Opnd *src1, IR::Opnd *src2, BailOutKind kind, IR::Instr * bailOutTarget, Func * func);
    static BailOutInstrTemplate * New(Js::OpCode opcode, BailOutKind kind, BailOutInfo * bailOutInfo, Func * func);

    BailOutInstrTemplate * CloneBailOut() const;
    BailOutInfo * bailOutInfo;
    BailOutKind bailOutKind;

    // Auxiliary bailout kind.
    // This is kind of a decoration on top of main bail out kind and is not used for runtime bail out logic (in globopt, etc).
    // It's added when we convert instr to bailout instr for which there is already bailout,
    // and is not used/just preserved until lowerer, in the beginning of lowerer we split it out.
    // Currently used for debugger bailout when it is shared with main bailout.
    BailOutKind auxBailOutKind;
};

typedef BailOutInstrTemplate<Instr> BailOutInstr;
typedef BailOutInstrTemplate<ProfiledInstr> ProfiledBailOutInstr;
typedef BailOutInstrTemplate<BranchInstr> BranchBailOutInstr;

//
// FOREACH_INSTR iterators
//

#ifdef DBG
# define INIT_PREV   IR::Instr * __prevInstrCheck = nullptr
# define CHECK_PREV(instr)\
    AssertMsg(__prevInstrCheck == nullptr || __prevInstrCheck->m_next == instr, \
        "Modifying instr list but not using EDITING iterator!"); \
    __prevInstrCheck = instr;
#else
# define INIT_PREV
# define CHECK_PREV(instr)
#endif

#ifdef DBG
# define INIT_NEXT   IR::Instr * __nextInstrCheck = nullptr
# define CHECK_NEXT(instr)\
    AssertMsg(__nextInstrCheck == nullptr || __nextInstrCheck->m_prev == instr, \
        "Modifying instr list but not using EDITING iterator!"); \
    __nextInstrCheck = instr;
#else
# define INIT_NEXT
# define CHECK_NEXT(instr)
#endif

#define FOREACH_INSTR_IN_RANGE(instr, instrList, instrLast)\
    {\
        INIT_PREV;\
        IR::Instr *instr##Stop = instrLast ? ((IR::Instr*)instrLast)->m_next : nullptr; \
        for ( IR::Instr *instr = instrList;\
            instr != instr##Stop;\
            instr = instr->m_next)\
        {\
            CHECK_PREV(instr);
#define NEXT_INSTR_IN_RANGE                     }}

#define FOREACH_REAL_INSTR_IN_RANGE(instr, instrList, instrLast)\
    FOREACH_INSTR_IN_RANGE(instr, instrList, instrLast)\
    {\
        if (!instr->IsRealInstr())\
        {\
            continue;\
        }
#define NEXT_REAL_INSTR_IN_RANGE    NEXT_INSTR_IN_RANGE }

#define FOREACH_INSTR_BACKWARD_IN_RANGE(instr, instrList, instrLast)\
    {\
        INIT_NEXT;\
        IR::Instr *instr##Stop = instrLast ? ((IR::Instr*)instrLast)->m_prev : nullptr; \
        for ( IR::Instr *instr = instrList;\
            instr != instr##Stop;\
            instr = instr->m_prev)\
        {\
            CHECK_NEXT(instr);
#define NEXT_INSTR_BACKWARD_IN_RANGE            }}

#define FOREACH_INSTR_EDITING_IN_RANGE(instr, instrNext, instrList, instrLast)\
    {\
        IR::Instr * instrNext;\
        IR::Instr *instr##Stop = instrLast ? ((IR::Instr*)instrLast)->m_next : nullptr; \
        for ( IR::Instr *instr = instrList;\
            instr != instr##Stop;\
            instr = instrNext)\
        {\
            instrNext = instr->m_next;
#define NEXT_INSTR_EDITING_IN_RANGE            }}

#define FOREACH_REAL_INSTR_EDITING_IN_RANGE(instr, instrNext, instrList, instrLast)\
    FOREACH_INSTR_EDITING_IN_RANGE(instr, instrNext, instrList, instrLast)\
    {\
        if (!instr->IsRealInstr())\
        {\
            continue;\
        }
#define NEXT_REAL_INSTR_EDITING_IN_RANGE NEXT_INSTR_EDITING_IN_RANGE }

#define FOREACH_INSTR_BACKWARD_EDITING_IN_RANGE(instr, instrPrev, instrList, instrLast)\
    {\
        IR::Instr * instrPrev;\
        IR::Instr *instr##Stop = instrLast ? ((IR::Instr*)instrLast)->m_prev : nullptr; \
        for ( IR::Instr *instr = instrList;\
            instr != instr##Stop;\
            instr = instrPrev)\
        {\
            instrPrev = instr->m_prev;
#define NEXT_INSTR_BACKWARD_EDITING_IN_RANGE   }}

#define FOREACH_INSTR_EDITING(instr, instrNext, instrList)\
    FOREACH_INSTR_EDITING_IN_RANGE(instr, instrNext, instrList, nullptr)
#define NEXT_INSTR_EDITING              NEXT_INSTR_EDITING_IN_RANGE

#define FOREACH_INSTR(instr, instrList)\
    FOREACH_INSTR_IN_RANGE(instr, instrList, nullptr)
#define NEXT_INSTR                      NEXT_INSTR_IN_RANGE

#define FOREACH_REAL_INSTR(instr, instrList)\
    FOREACH_REAL_INSTR_IN_RANGE(instr, instrList, nullptr)
#define NEXT_REAL_INSTR                 NEXT_REAL_INSTR_IN_RANGE

#define FOREACH_INSTR_BACKWARD(instr, instrList)\
    FOREACH_INSTR_BACKWARD_IN_RANGE(instr, instrList, nullptr)
#define NEXT_INSTR_BACKWARD             NEXT_INSTR_BACKWARD_IN_RANGE

#define FOREACH_INSTR_EDITING(instr, instrNext, instrList)\
    FOREACH_INSTR_EDITING_IN_RANGE(instr, instrNext, instrList, nullptr)
#define NEXT_INSTR_EDITING             NEXT_INSTR_EDITING_IN_RANGE

#define FOREACH_REAL_INSTR_EDITING(instr, instrNext, instrList)\
    FOREACH_REAL_INSTR_EDITING_IN_RANGE(instr, instrNext, instrList, nullptr)
#define NEXT_REAL_INSTR_EDITING         NEXT_REAL_INSTR_EDITING_IN_RANGE

#define FOREACH_INSTR_BACKWARD_EDITING(instr, instrPrev, instrList)\
    FOREACH_INSTR_BACKWARD_EDITING_IN_RANGE(instr, instrPrev, instrList, nullptr)
#define NEXT_INSTR_BACKWARD_EDITING     NEXT_INSTR_BACKWARD_EDITING_IN_RANGE

#define FOREACH_INSTR_IN_FUNC(instr, func)\
    FOREACH_INSTR(instr, func->m_headInstr)
#define NEXT_INSTR_IN_FUNC              NEXT_INSTR

#define FOREACH_REAL_INSTR_IN_FUNC(instr, func)\
    FOREACH_REAL_INSTR(instr, func->m_headInstr)
#define NEXT_REAL_INSTR_IN_FUNC         NEXT_REAL_INSTR

#define FOREACH_INSTR_IN_FUNC_BACKWARD(instr, func)\
    FOREACH_INSTR_BACKWARD(instr, func->m_tailInstr)
#define NEXT_INSTR_IN_FUNC_BACKWARD     NEXT_INSTR_BACKWARD

#define FOREACH_INSTR_IN_FUNC_EDITING(instr, instrNext, func)\
    FOREACH_INSTR_EDITING(instr, instrNext, func->m_headInstr)
#define NEXT_INSTR_IN_FUNC_EDITING      NEXT_INSTR_EDITING

#define FOREACH_REAL_INSTR_IN_FUNC_EDITING(instr, instrNext, func)\
    FOREACH_REAL_INSTR_EDITING(instr, instrNext, func->m_headInstr)
#define NEXT_REAL_INSTR_IN_FUNC_EDITING NEXT_REAL_INSTR_EDITING

#define FOREACH_INSTR_IN_FUNC_BACKWARD_EDITING(instr, instrPrev, func)\
    FOREACH_INSTR_BACKWARD_EDITING(instr, instrPrev, func->m_tailInstr)
#define NEXT_INSTR_IN_FUNC_BACKWARD_EDITING NEXT_INSTR_BACKWARD_EDITING

#define FOREACH_INSTR_IN_BLOCK(instr, block)\
    FOREACH_INSTR_IN_RANGE(instr, block->GetFirstInstr(), block->GetLastInstr())
#define NEXT_INSTR_IN_BLOCK\
    NEXT_INSTR_IN_RANGE

#define FOREACH_INSTR_IN_BLOCK_EDITING(instr, instrNext, block)\
    FOREACH_INSTR_EDITING_IN_RANGE(instr, instrNext, block->GetFirstInstr(), block->GetLastInstr())
#define NEXT_INSTR_IN_BLOCK_EDITING \
    NEXT_INSTR_EDITING_IN_RANGE

#define FOREACH_INSTR_BACKWARD_IN_BLOCK(instr, block)\
    FOREACH_INSTR_BACKWARD_IN_RANGE(instr, block->GetLastInstr(), block->GetFirstInstr())
#define NEXT_INSTR_BACKWARD_IN_BLOCK\
    NEXT_INSTR_BACKWARD_IN_RANGE

#define FOREACH_INSTR_BACKWARD_IN_BLOCK_EDITING(instr, instrPrev, block)\
    FOREACH_INSTR_BACKWARD_EDITING_IN_RANGE(instr, instrPrev, block->GetLastInstr(), block->GetFirstInstr())
#define NEXT_INSTR_BACKWARD_IN_BLOCK_EDITING\
    NEXT_INSTR_BACKWARD_EDITING_IN_RANGE

} // namespace IR

typedef JsUtil::BaseDictionary<IR::Instr*, IR::Instr*, JitArenaAllocator, PrimeSizePolicy> InstrMap;
