//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class Lowerer;

enum LegalForms : uint8
{
    L_None  = 0,
    L_Reg   = 1 << 0,
    L_Mem   = 1 << 1,
    L_Imm32 = 1 << 2,  // supports 8-bit, 16-bit, and 32-bit immediate values
    L_Ptr   = 1 << 3,  // supports 8-bit, 16-bit, 32-bit, and 64-bit immediate values on 64-bit architectures

    L_FormMask = (L_Ptr << 1) - 1,

    // Start flags for common behavior
    LF_Custom   = 1 << 4,   // Legal Form Flag: Custom Legal forms, must be handled in the Legalizer Switch-Case
#if DBG
    LF_Optional = 1 << 5,   // Legal Form Flag: legal for the opnd to be missing
#else
    LF_Optional = 0,        // Legal Form Flag: legal for the opnd to be missing
#endif
};
ENUM_CLASS_HELPERS(LegalForms, uint8);


#include "LowererMDArch.h"

///---------------------------------------------------------------------------
///
/// class LowererMD
///
///---------------------------------------------------------------------------

class LowererMD
{
public:
    LowererMD(Func *func) :
        m_func(func),
        lowererMDArch(func),
        floatTmpMap(nullptr),
        bvFloatTmpInits(nullptr),
        m_simd128OpCodesMap(nullptr)
    {
    }

    friend class LowererMDArch;

    static  bool            IsAssign(IR::Instr *instr);
    static  bool            IsCall(IR::Instr *instr);
    static  bool            IsUnconditionalBranch(const IR::Instr *instr);
    static  void            InvertBranch(IR::BranchInstr *instr);
    static  void            ReverseBranch(IR::BranchInstr *branchInstr);
    static  Js::OpCode      MDBranchOpcode(Js::OpCode opcode);
    static Js::OpCode       MDUnsignedBranchOpcode(Js::OpCode opcode);
    static Js::OpCode       MDCompareWithZeroBranchOpcode(Js::OpCode opcode);
    static Js::OpCode       MDConvertFloat64ToInt32Opcode(const RoundMode roundMode);
    static void             ChangeToAdd(IR::Instr *const instr, const bool needFlags);
    static void             ChangeToSub(IR::Instr *const instr, const bool needFlags);
    static void             ChangeToShift(IR::Instr *const instr, const bool needFlags);
    static void             ChangeToIMul(IR::Instr *const instr, const bool hasOverflowCheck = false);
    static uint16           GetFormalParamOffset();
    static const Js::OpCode MDUncondBranchOpcode;
    static const Js::OpCode MDMultiBranchOpcode;
    static const Js::OpCode MDExtend32Opcode;
    static const Js::OpCode MDTestOpcode;
    static const Js::OpCode MDOrOpcode;
    static const Js::OpCode MDXorOpcode;
#if _M_X64
    static const Js::OpCode MDMovUint64ToFloat64Opcode;
#endif
    static const Js::OpCode MDOverflowBranchOpcode;
    static const Js::OpCode MDNotOverflowBranchOpcode;
    static const Js::OpCode MDConvertFloat32ToFloat64Opcode;
    static const Js::OpCode MDConvertFloat64ToFloat32Opcode;
    static const Js::OpCode MDCallOpcode;
    static const Js::OpCode MDImulOpcode;
    static const Js::OpCode MDLea;
    static const Js::OpCode MDSpecBlockNEOpcode;
    static const Js::OpCode MDSpecBlockFNEOpcode;

    UINT FloatPrefThreshold;

            void            Init(Lowerer *lowerer);
            IR::Opnd *      GenerateMemRef(intptr_t addr, IRType type, IR::Instr *instr, bool dontEncode = false);
            void            GenerateMemInit(IR::RegOpnd * opnd, int32 offset, size_t value, IR::Instr * insertBeforeInstr, bool isZeroed = false);
            IR::Instr *     ChangeToHelperCall(IR::Instr * instr, IR::JnHelperMethod helperMethod, IR::LabelInstr *labelBailOut = nullptr,
                                               IR::Opnd *opndInstance = nullptr, IR::PropertySymOpnd * propSymOpnd = nullptr, bool isHelperContinuation = false);
            void            FinalLower();
#ifdef _M_X64
            void            FlipHelperCallArgsOrder()
            {
                lowererMDArch.FlipHelperCallArgsOrder();
            }
#endif
            IR::Instr *     ChangeToHelperCallMem(IR::Instr * instr, IR::JnHelperMethod helperMethod);

    static  IR::Instr *     ChangeToAssign(IR::Instr * instr);
    static  IR::Instr *     ChangeToAssignNoBarrierCheck(IR::Instr * instr);
    static  IR::Instr *     ChangeToAssign(IR::Instr * instr, IRType type);
    static  void            ImmedSrcToReg(IR::Instr * instr, IR::Opnd * newOpnd, int srcNum);
    static IR::Opnd *       CreateStackArgumentsSlotOpnd(Func *func);

            IR::Instr *     LoadInputParamCount(IR::Instr * instr, int adjust = 0, bool needFlags = false);
            IR::Instr *     LoadStackArgPtr(IR::Instr * instr);
            IR::Instr *     LoadArgumentsFromFrame(IR::Instr * instr);
            IR::Instr *     LoadArgumentCount(IR::Instr * instr);
            IR::Instr *     LoadHeapArguments(IR::Instr * instr);
            IR::Instr *     LoadHeapArgsCached(IR::Instr * instr);
            IR::Instr *     LowerRet(IR::Instr * instr);
            IR::Instr *     LowerCondBranch(IR::Instr * instr);
            IR::Instr *     LoadFunctionObjectOpnd(IR::Instr *instr, IR::Opnd *&functionObjOpnd);
            IR::Instr *     LowerNewScObject(IR::Instr *newObjInstr);
            IR::Instr *     LowerWasmArrayBoundsCheck(IR::Instr *instr, IR::Opnd *addrOpnd);
            void            LowerAtomicStore(IR::Opnd * dst, IR::Opnd * src1, IR::Instr * insertBeforeInstr);
            void            LowerAtomicLoad(IR::Opnd* dst, IR::Opnd* src1, IR::Instr* insertBeforeInstr);
            void            ForceDstToReg(IR::Instr *instr);

public:
            template <bool verify = false>
            static void     Legalize(IR::Instr *const instr, bool fPostRegAlloc = false);
private:
            template <bool verify>
            static void     LegalizeOpnds(IR::Instr *const instr, const LegalForms dstForms, LegalForms src1Forms, LegalForms src2Forms);
            template <bool verify>
            static void     LegalizeDst(IR::Instr *const instr, const LegalForms forms);
            template <bool verify>
            static void     LegalizeSrc(IR::Instr *const instr, IR::Opnd *src, const LegalForms forms);
            template <bool verify = false>
            static void     MakeDstEquSrc1(IR::Instr *const instr);
            static bool     HoistLargeConstant(IR::IndirOpnd *indirOpnd, IR::Opnd *src, IR::Instr *instr);
public:
            IR::Instr *     GenerateSmIntPairTest(IR::Instr * instrInsert, IR::Opnd * opndSrc1, IR::Opnd * opndSrc2, IR::LabelInstr * labelFail);
            void            GenerateSmIntTest(IR::Opnd *opndSrc, IR::Instr *instrInsert, IR::LabelInstr *labelHelper, IR::Instr **instrFirst = nullptr, bool fContinueLabel = false);
            void            GenerateTaggedZeroTest( IR::Opnd * opndSrc, IR::Instr * instrInsert, IR::LabelInstr * labelHelper = nullptr);
            bool            GenerateObjectTest(IR::Opnd * opndSrc, IR::Instr * insertInstr, IR::LabelInstr * labelInstr, bool fContinueLabel = false);
            void            GenerateInt32ToVarConversion( IR::Opnd * opndSrc, IR::Instr * insertInstr );
            void            GenerateFloatTest( IR::RegOpnd * opndSrc, IR::Instr * insertInstr, IR::LabelInstr* labelHelper, const bool checkForNullInLoopBody = false);
#if FLOATVAR
            IR::RegOpnd*    CheckFloatAndUntag(IR::RegOpnd * opndSrc, IR::Instr * insertInstr, IR::LabelInstr* labelHelper);
#endif
            bool            GenerateFastCmSrXxConst(IR::Instr *instr);
            bool            GenerateFastCmXxTaggedInt(IR::Instr *instr, bool isInHelper = false);
            void            GenerateFastCmXxI4(IR::Instr *instr);
            void            GenerateFastCmXxR8(IR::Instr *instr);
            void            GenerateFastCmXx(IR::Instr *instr);
            IR::Instr *     GenerateConvBool(IR::Instr *instr);
            void            GenerateFastDivByPow2(IR::Instr *instrDiv);
            bool            GenerateFastAdd(IR::Instr * instrAdd);
            bool            GenerateFastDivAndRem(IR::Instr* instr, IR::LabelInstr* bailoutLabel = nullptr);
#if DBG
            static void     GenerateDebugBreak( IR::Instr * insertInstr );
#endif
            bool            GenerateFastSub(IR::Instr * instrSub);
            bool            GenerateFastMul(IR::Instr * instrMul);
            bool            GenerateFastAnd(IR::Instr * instrAnd);
            bool            GenerateFastXor(IR::Instr * instrXor);
            bool            GenerateFastOr(IR::Instr * instrOr);
            bool            GenerateFastNot(IR::Instr * instrNot);

            bool            GenerateFastShiftLeft(IR::Instr * instrShift);
            bool            GenerateFastShiftRight(IR::Instr * instrShift);
            bool            GenerateFastNeg(IR::Instr * instrNeg);
            void            GenerateFastBrS(IR::BranchInstr *brInstr);
            IR::IndirOpnd*  GetArgsIndirOpndForTopFunction(IR::Instr* ldElem, IR::Opnd* valueOpnd);
            IR::IndirOpnd*  GetArgsIndirOpndForInlinee(IR::Instr* ldElem, IR::Opnd* valueOpnd);
            void            GenerateCheckForArgumentsLength(IR::Instr* ldElem, IR::LabelInstr* labelCreateHeapArgs, IR::Opnd* actualParamOpnd, IR::Opnd* valueOpnd, Js::OpCode);
            IR::RegOpnd *   LoadNonnegativeIndex(IR::RegOpnd *indexOpnd, const bool skipNegativeCheck, IR::LabelInstr *const notTaggedIntLabel, IR::LabelInstr *const negativeLabel, IR::Instr *const insertBeforeInstr);
            IR::RegOpnd *   GenerateUntagVar(IR::RegOpnd * opnd, IR::LabelInstr * labelFail, IR::Instr * insertBeforeInstr, bool generateTagCheck = true);
            bool            GenerateFastLdMethodFromFlags(IR::Instr * instrLdFld);
            IR::Instr *     GenerateFastScopedLdFld(IR::Instr * instrLdFld);
            IR::Instr *     GenerateFastScopedStFld(IR::Instr * instrStFld);
            void            GenerateFastAbs(IR::Opnd *dst, IR::Opnd *src, IR::Instr *callInstr, IR::Instr *insertInstr, IR::LabelInstr *labelHelper, IR::LabelInstr *doneLabel);
            IR::Instr *     GenerateFloatAbs(IR::RegOpnd * regOpnd, IR::Instr * insertInstr);
            void            GenerateClz(IR::Instr * instr);
            void            GenerateCtz(IR::Instr * instr);
            void            GeneratePopCnt(IR::Instr * instr);
            template <bool Saturate> void GenerateTruncWithCheck(_In_ IR::Instr * instr);
            template <bool Saturate> IR::Opnd* GenerateTruncChecks(_In_ IR::Instr* instr, _In_opt_ IR::LabelInstr* doneLabel);
            IR::RegOpnd*    MaterializeDoubleConstFromInt(intptr_t constAddr, IR::Instr* instr);
            IR::RegOpnd*    MaterializeConstFromBits(int intConst, IRType type, IR::Instr* instr);
            IR::Opnd*       Subtract2To31(IR::Opnd* src1, IR::Opnd* intMinFP, IRType type, IR::Instr* instr);
            bool            TryGenerateFastMulAdd(IR::Instr * instrAdd, IR::Instr ** pInstrPrev);
            void            GenerateFastInlineBuiltInCall(IR::Instr* instr, IR::JnHelperMethod helperMethod);
            void            HelperCallForAsmMathBuiltin(IR::Instr* instr, IR::JnHelperMethod helperMethodFloat, IR::JnHelperMethod helperMethodDouble);
            void            GenerateFastInlineBuiltInMathAbs(IR::Instr* instr);
            void            GenerateFastInlineBuiltInMathPow(IR::Instr* instr);
            IR::Instr *     EnsureAdjacentArgs(IR::Instr * instrArg);
            void            SaveDoubleToVar(IR::RegOpnd * dstOpnd, IR::RegOpnd *opndFloat, IR::Instr *instrOrig, IR::Instr *instrInsert, bool isHelper = false);
#if !FLOATVAR
            void            GenerateNumberAllocation(IR::RegOpnd * opndDst, IR::Instr * instrInsert, bool isHelper);
#endif
            void            GenerateFastRecyclerAlloc(size_t allocSize, IR::RegOpnd* newObjDst, IR::Instr* insertionPointInstr, IR::LabelInstr* allocHelperLabel, IR::LabelInstr* allocDoneLabel);
#ifdef _CONTROL_FLOW_GUARD
            void            GenerateCFGCheck(IR::Opnd * entryPointOpnd, IR::Instr * insertBeforeInstr);
#endif

            void            GenerateCopysign(IR::Instr * instr);

            static IR::Instr *LoadFloatZero(IR::Opnd * opndDst, IR::Instr * instrInsert);
            template <typename T>
            static IR::Instr *LoadFloatValue(IR::Opnd * opndDst, T value, IR::Instr * instrInsert);
            void            EmitInt64Instr(IR::Instr * instr);
     static void            EmitInt4Instr(IR::Instr *instr);
            void            EmitLoadVar(IR::Instr *instr, bool isFromUint32 = false, bool isHelper = false);
            void            EmitLoadVarNoCheck(IR::RegOpnd * dst, IR::RegOpnd * src, IR::Instr *instrLoad, bool isFromUint32, bool isHelper);
            bool            EmitLoadInt32(IR::Instr *instr, bool conversionFromObjectAllowed, bool bailOutOnHelper = false, IR::LabelInstr * labelBailOut = nullptr);
            void            EmitIntToFloat(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert);
            void            EmitUIntToFloat(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert);
            void            EmitIntToLong(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert);
            void            EmitUIntToLong(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert);
            void            EmitLongToInt(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert);
            void            EmitFloatToInt(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert, IR::Instr * instrBailOut = nullptr, IR::LabelInstr * labelBailOut = nullptr);
            void            EmitInt64toFloat(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert);
            void            EmitFloat32ToFloat64(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert);
            void            EmitSignExtend(IR::Instr * instr);
     static IR::Instr *     InsertConvertFloat64ToInt32(const RoundMode roundMode, IR::Opnd *const dst, IR::Opnd *const src, IR::Instr *const insertBeforeInstr);
            void            ConvertFloatToInt32(IR::Opnd* intOpnd, IR::Opnd* floatOpnd, IR::LabelInstr * labelHelper, IR::LabelInstr * labelDone, IR::Instr * instInsert);
            void            EmitReinterpretPrimitive(IR::Opnd* dst, IR::Opnd* src, IR::Instr* insertBeforeInstr);
            void            EmitLoadFloatFromNumber(IR::Opnd *dst, IR::Opnd *src, IR::Instr *insertInstr);
            void            EmitLoadFloat(IR::Opnd *dst, IR::Opnd *src, IR::Instr *insertInstr, IR::Instr * instrBailOut = nullptr, IR::LabelInstr * labelBailOut = nullptr);
            static void     EmitNon32BitOvfCheck(IR::Instr *instr, IR::Instr *insertInstr, IR::LabelInstr* bailOutLabel);
            IR::BranchInstr* InsertMissingItemCompareBranch(IR::Opnd* compareSrc, IR::Opnd* missingItemOpnd, Js::OpCode opcode, IR::LabelInstr* target, IR::Instr* insertBeforeInstr);

            static void     LowerInt4NegWithBailOut(IR::Instr *const instr, const IR::BailOutKind bailOutKind, IR::LabelInstr *const bailOutLabel, IR::LabelInstr *const skipBailOutLabel);
            static void     LowerInt4AddWithBailOut(IR::Instr *const instr, const IR::BailOutKind bailOutKind, IR::LabelInstr *const bailOutLabel, IR::LabelInstr *const skipBailOutLabel);
            static void     LowerInt4SubWithBailOut(IR::Instr *const instr, const IR::BailOutKind bailOutKind, IR::LabelInstr *const bailOutLabel, IR::LabelInstr *const skipBailOutLabel);
            static void     LowerInt4MulWithBailOut(IR::Instr *const instr, const IR::BailOutKind bailOutKind, IR::LabelInstr *const bailOutLabel, IR::LabelInstr *const skipBailOutLabel);
            void     LowerInt4RemWithBailOut(IR::Instr *const instr, const IR::BailOutKind bailOutKind, IR::LabelInstr *const bailOutLabel, IR::LabelInstr *const skipBailOutLabel) const;

            static bool     GenerateSimplifiedInt4Mul(IR::Instr *const mulInstr, const IR::BailOutKind bailOutKind = IR::BailOutInvalid, IR::LabelInstr *const bailOutLabel = nullptr);
            static bool     GenerateSimplifiedInt4Rem(IR::Instr *const remInstr, IR::LabelInstr *const skipBailOutLabel = nullptr);

            IR::Instr *     LowerTry(IR::Instr *instr, IR::JnHelperMethod helperMethod);
            IR::Instr *     LowerEHRegionReturn(IR::Instr * insertBeforeInstr, IR::Opnd * targetOpnd);
            IR::Instr *     LowerLeaveNull(IR::Instr *instr);
            IR::Instr *     LowerCallHelper(IR::Instr *instrCall);

            IR::LabelInstr *GetBailOutStackRestoreLabel(BailOutInfo * bailOutInfo, IR::LabelInstr * exitTargetInstr);
            StackSym *      GetImplicitParamSlotSym(Js::ArgSlot argSlot);
     static StackSym *      GetImplicitParamSlotSym(Js::ArgSlot argSlot, Func * func);

            Lowerer*        GetLowerer() { return m_lowerer; }

            void            GenerateIsJsObjectTest(IR::RegOpnd* instanceReg, IR::Instr* insertInstr, IR::LabelInstr* labelHelper);
            void            LowerTypeof(IR::Instr * typeOfInstr);

     static void            InsertObjectPoison(IR::Opnd* poisonedOpnd, IR::BranchInstr* branchInstr, IR::Instr* insertInstr, bool isForStore);
public:
            //
            // These methods are simply forwarded to lowererMDArch
            //
            IR::Instr *         LowerAsmJsCallI(IR::Instr * callInstr);
            IR::Instr *         LowerAsmJsCallE(IR::Instr * callInstr);
            IR::Instr *         LowerAsmJsLdElemHelper(IR::Instr * callInstr);
            IR::Instr *         LowerAsmJsStElemHelper(IR::Instr * callInstr);
            IR::Instr *         LowerCall(IR::Instr * callInstr, Js::ArgSlot argCount);
            IR::Instr *         LowerCallI(IR::Instr * callInstr, ushort callFlags, bool isHelper = false, IR::Instr * insertBeforeInstrForCFG = nullptr);
            IR::Instr *         LoadInt64HelperArgument(IR::Instr * instr, IR::Opnd* opnd);
            IR::Instr *         LoadHelperArgument(IR::Instr * instr, IR::Opnd * opndArg);
            IR::MemRefOpnd *    LoadSimdHelperArgument(IR::Instr * instr, uint8 index);
            IR::Instr *         LoadDoubleHelperArgument(IR::Instr * instr, IR::Opnd * opndArg);
            IR::Instr *         LoadFloatHelperArgument(IR::Instr * instr, IR::Opnd * opndArg);
            IR::Instr *         LowerEntryInstr(IR::EntryInstr * entryInstr);
            IR::Instr *         LowerExitInstr(IR::ExitInstr * exitInstr);
            IR::Instr *         LowerExitInstrAsmJs(IR::ExitInstr * exitInstr);
            IR::Instr *         LoadNewScObjFirstArg(IR::Instr * instr, IR::Opnd * dst, ushort extraArgs = 0);
            IR::Instr *         LowerToFloat(IR::Instr *instr);
     static IR::BranchInstr *   LowerFloatCondBranch(IR::BranchInstr *instrBranch, bool ignoreNan = false);

     static Js::OpCode          GetLoadOp(IRType type) { return LowererMDArch::GetAssignOp(type); }
     static Js::OpCode          GetStoreOp(IRType type) { return LowererMDArch::GetAssignOp(type); }
     static RegNum              GetRegStackPointer() { return LowererMDArch::GetRegStackPointer(); }
     static RegNum              GetRegArgI4(int32 argNum) { return LowererMDArch::GetRegArgI4(argNum); }
     static RegNum              GetRegArgR8(int32 argNum) { return LowererMDArch::GetRegArgR8(argNum); }
     static RegNum              GetRegReturn(IRType type) { return LowererMDArch::GetRegReturn(type); }

            //All the following functions delegate to lowererMDArch
            IR::Instr *         LowerCallIDynamic(IR::Instr * callInstr, IR::Instr* saveThis, IR::Opnd* argsLengthOpnd, ushort callFlags, IR::Instr * insertBeforeInstrForCFG = nullptr)
            {
                return this->lowererMDArch.LowerCallIDynamic(callInstr, saveThis, argsLengthOpnd, callFlags, insertBeforeInstrForCFG);
            }

            IR::Instr *         LoadDynamicArgument(IR::Instr * instr, uint argNumber = 1) { return this->lowererMDArch.LoadDynamicArgument(instr, argNumber);}
            IR::Opnd*           GenerateArgOutForStackArgs(IR::Instr* callInstr, IR::Instr* stackArgsInstr) { return lowererMDArch.GenerateArgOutForStackArgs(callInstr, stackArgsInstr);}
     static RegNum              GetRegFramePointer() { return LowererMDArch::GetRegFramePointer(); }
     static BYTE                GetDefaultIndirScale() { return LowererMDArch::GetDefaultIndirScale(); }
            IR::Instr *         LoadDynamicArgumentUsingLength(IR::Instr *instr) { return this->lowererMDArch.LoadDynamicArgumentUsingLength(instr); }
            void                GenerateFunctionObjectTest(IR::Instr * callInstr, IR::RegOpnd  *functionObjOpnd, bool isHelper, IR::LabelInstr* afterCallLabel = nullptr) { this->lowererMDArch.GenerateFunctionObjectTest(callInstr, functionObjOpnd, isHelper, afterCallLabel); }
            int32               LowerCallArgs(IR::Instr *callInstr, ushort callFlags, ushort extraArgsCount = 1 /* for function object */) { return this->lowererMDArch.LowerCallArgs(callInstr, callFlags, extraArgsCount); }

            static void GenerateLoadTaggedType(IR::Instr * instrLdSt, IR::RegOpnd * opndType, IR::RegOpnd * opndTaggedType);
            static void GenerateLoadPolymorphicInlineCacheSlot(IR::Instr * instrLdSt, IR::RegOpnd * opndInlineCache, IR::RegOpnd * opndType, uint polymorphicInlineCacheSize);

            void GenerateWriteBarrierAssign(IR::IndirOpnd * opndDst, IR::Opnd * opndSrc, IR::Instr * insertBeforeInstr);
            void GenerateWriteBarrierAssign(IR::MemRefOpnd * opndDst, IR::Opnd * opndSrc, IR::Instr * insertBeforeInstr);
            static IR::Instr * ChangeToWriteBarrierAssign(IR::Instr * assignInstr, const Func* func);

            void GenerateStFldFromLocalInlineCache(IR::Instr * instrStFld, IR::RegOpnd * opndBase, IR::Opnd * opndSrc, IR::RegOpnd * opndInlineCache, IR::LabelInstr * labelFallThru, bool isInlineSlot);

            IR::Instr *         LowerDivI4AndBailOnReminder(IR::Instr * instr, IR::LabelInstr * bailOutLabel);

            void                LowerInlineSpreadArgOutLoop(IR::Instr *callInstr, IR::RegOpnd *indexOpnd, IR::RegOpnd *arrayElementsStartOpnd)
            {
                this->lowererMDArch.LowerInlineSpreadArgOutLoop(callInstr, indexOpnd, arrayElementsStartOpnd);
            }

    static IR::Instr * InsertCmovCC(const Js::OpCode opCode, IR::Opnd * dst, IR::Opnd* src1, IR::Instr* insertBeforeInstr, bool postRegAlloc = false);
    
#ifdef ENABLE_WASM_SIMD
    void                Simd128InitOpcodeMap();
    IR::Instr*          Simd128Instruction(IR::Instr* instr);
    IR::Instr*          Simd128LoadConst(IR::Instr* instr);
    IR::Instr*          LowerSimd128BitSelect(IR::Instr* instr);
    bool                Simd128TryLowerMappedInstruction(IR::Instr *instr);
    IR::Instr*          Simd128LowerUnMappedInstruction(IR::Instr *instr);
    IR::Instr*          Simd128LowerLdLane(IR::Instr *instr);
    IR::Instr*          SIMD128LowerReplaceLane_2(IR::Instr *instr);
    void                EmitExtractInt64(IR::Opnd* dst, IR::Opnd* src, uint index, IR::Instr *instr);
    void                EmitInsertInt64(IR::Opnd* dst, uint index, IR::Instr *instr);
    void                EmitShiftByScalarI2(IR::Instr *instr, IR::JnHelperMethod helper);
    IR::Instr*          EmitSimdConversion(IR::Instr *instr, IR::JnHelperMethod helper);
    IR::Instr*          SIMD128LowerReplaceLane_4(IR::Instr *instr);
    IR::Instr*          SIMD128LowerReplaceLane_8(IR::Instr *instr);
    IR::Instr*          SIMD128LowerReplaceLane_16(IR::Instr *instr);
    IR::Instr*          Simd128LowerSplat(IR::Instr *instr);
    IR::Instr*          Simd128LowerSqrt(IR::Instr *instr);
    IR::Instr*          Simd128LowerNeg(IR::Instr *instr);
    IR::Instr*          Simd128LowerMulI4(IR::Instr *instr);
    IR::Instr*          Simd128LowerShift(IR::Instr *instr);
    IR::Instr*          Simd128LowerMulI16(IR::Instr *instr);
    IR::Instr*          Simd128LowerInt32x4FromFloat32x4(IR::Instr *instr);
    IR::Instr*          Simd128LowerUint32x4FromFloat32x4(IR::Instr *instr);
    IR::Instr*          Simd128LowerFloat32x4FromUint32x4(IR::Instr *instr);
    IR::Instr*          Simd128AsmJsLowerLoadElem(IR::Instr *instr);
    IR::Instr*          Simd128LowerLoadElem(IR::Instr *instr);
    IR::Instr*          Simd128ConvertToLoad(IR::Opnd *dst, IR::Opnd *src1, uint8 dataWidth, IR::Instr* instr, byte scaleFactor = 0);
    IR::Instr*          Simd128AsmJsLowerStoreElem(IR::Instr *instr);
    IR::Instr*          Simd128LowerStoreElem(IR::Instr *instr);
    IR::Instr*          Simd128ConvertToStore(IR::Opnd *dst, IR::Opnd *src1, uint8 dataWidth, IR::Instr* instr, byte scaleFactor = 0);
    void                Simd128LoadHeadSegment(IR::IndirOpnd *indirOpnd, ValueType arrType, IR::Instr *instr);
    void                Simd128GenerateUpperBoundCheck(IR::RegOpnd *indexOpnd, IR::IndirOpnd *indirOpnd, ValueType arrType, IR::Instr *instr);
    IR::Instr*          Simd128LowerSwizzle_4(IR::Instr *instr);
    IR::Instr*          Simd128LowerShuffle_4(IR::Instr *instr);
    IR::Instr*          Simd128LowerShuffle(IR::Instr *instr);
    IR::Instr*          Simd128LowerNotEqual(IR::Instr* instr);
    IR::Instr*          Simd128LowerLessThan(IR::Instr* instr);
    IR::Instr*          Simd128LowerLessThanOrEqual(IR::Instr* instr);
    IR::Instr*          Simd128LowerGreaterThanOrEqual(IR::Instr* instr);
    IR::Instr*          Simd128LowerMinMax_F4(IR::Instr* instr);
    IR::Instr*          Simd128LowerAnyTrue(IR::Instr* instr);
    IR::Instr*          Simd128LowerAllTrue(IR::Instr* instr);
    IR::Opnd*           Simd128CanonicalizeToBoolsBeforeReduction(IR::Instr* instr);
    BYTE                Simd128GetTypedArrBytesPerElem(ValueType arrType);
    IR::Instr*          Simd128CanonicalizeToBools(IR::Instr* instr, const Js::OpCode& cmpOpcode, IR::Opnd& dstOpnd);
    IR::Opnd*           EnregisterIntConst(IR::Instr* instr, IR::Opnd *constOpnd, IRType type = TyInt32);
    IR::Opnd*           EnregisterBoolConst(IR::Instr* instr, IR::Opnd *opnd, IRType type);
    SList<IR::Opnd*>  * Simd128GetExtendedArgs(IR::Instr *instr);
    void                CheckShuffleLanes_4(uint8 lanes[], uint8 lanesSrc[], uint *fromSrc1, uint *fromSrc2);
    void                InsertShufps(uint8 lanes[], IR::Opnd *dst, IR::Opnd *src1, IR::Opnd *src2, IR::Instr *insertBeforeInstr);
#endif

private:
    void EmitReinterpretFloatToInt(IR::Opnd* dst, IR::Opnd* src, IR::Instr* insertBeforeInstr);
    void EmitReinterpretIntToFloat(IR::Opnd* dst, IR::Opnd* src, IR::Instr* insertBeforeInstr);
    IR::Instr * NegZeroBranching(IR::Opnd* opnd, IR::Instr* instr, IR::LabelInstr* isNeg0Label, IR::LabelInstr* isNotNeg0Label);

    IR::LabelInstr*   EmitLoadFloatCommon(IR::Opnd *dst, IR::Opnd *src, IR::Instr *insertInstr, bool needLabelHelper);
#ifdef RECYCLER_WRITE_BARRIER
    static IR::Instr* GenerateWriteBarrier(IR::Instr * assignInstr);
#endif

    // Data
protected:
    Func                     *m_func;
    Lowerer                  *m_lowerer;
    LowererMDArch             lowererMDArch;
    StackSymMap              *floatTmpMap;
    BVSparse<JitArenaAllocator> *bvFloatTmpInits;
    Js::OpCode              *m_simd128OpCodesMap; // used to map single-opcode SIMD operations
};
