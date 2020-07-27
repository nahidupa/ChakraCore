//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"
#include "RuntimeMathPch.h"

namespace IR
{
void
Instr::Init(Js::OpCode opcode, IRKind kind, Func * func)
{
    Assert(!OpCodeAttr::ByteCodeOnly(opcode));
    this->m_opcode = opcode;
    this->m_kind = kind;
    this->m_func = func;
#ifdef BAILOUT_INJECTION
    this->bailOutByteCodeLocation = (uint)-1;
#endif
}

uint32
Instr::GetByteCodeOffset() const
{
    Assert(m_func->HasByteCodeOffset());
    return m_number;
}

void
Instr::SetByteCodeOffset(uint32 offset)
{
    Assert(m_func->HasByteCodeOffset());
    Assert(m_number == Js::Constants::NoByteCodeOffset);
    m_number = offset;
}

void
Instr::SetByteCodeOffset(IR::Instr * instr)
{
    SetByteCodeOffset(instr->GetByteCodeOffset());
}

void
Instr::ClearByteCodeOffset()
{
    Assert(m_func->HasByteCodeOffset());
    m_number = Js::Constants::NoByteCodeOffset;
}

uint32
Instr::GetNumber() const
{
    Assert(m_func->HasInstrNumber());
    return m_number;
}

void
Instr::SetNumber(uint32 number)
{
    Assert(m_func->HasInstrNumber());
    m_number = number;
}

bool
Instr::IsPlainInstr() const
{
    return this->GetKind() == IR::InstrKindInstr;
}

bool
Instr::DoStackArgsOpt() const
{
    return this->usesStackArgumentsObject && m_func->IsStackArgsEnabled();
}

bool
Instr::HasTypeCheckBailOut() const
{
    return this->HasBailOutInfo() && IR::IsTypeCheckBailOutKind(this->GetBailOutKind());
}

bool
Instr::HasEquivalentTypeCheckBailOut() const
{
    return this->HasBailOutInfo() && IR::IsEquivalentTypeCheckBailOutKind(this->GetBailOutKind());
}

bool
Instr::HasBailOnNoProfile() const
{
    return this->HasBailOutInfo() && this->GetBailOutKind() == IR::BailOutOnNoProfile;
}

void
Instr::ChangeEquivalentToMonoTypeCheckBailOut()
{
    Assert(this->HasEquivalentTypeCheckBailOut());

    this->SetBailOutKind(IR::EquivalentToMonoTypeCheckBailOutKind(this->GetBailOutKind()));
}

intptr_t
Instr::TryOptimizeInstrWithFixedDataProperty(IR::Instr **pInstr, GlobOpt * globopt)
{
    IR::Instr *&instr = *pInstr;
    Assert(OpCodeAttr::CanLoadFixedFields(instr->m_opcode));
    IR::Opnd * src1 = instr->GetSrc1();
    Assert(src1 && src1->IsSymOpnd() && src1->AsSymOpnd()->IsPropertySymOpnd());

    IR::PropertySymOpnd * propSymOpnd = src1->AsSymOpnd()->AsPropertySymOpnd();
    if (propSymOpnd->HasFixedValue() && !propSymOpnd->IsPoly())
    {
        intptr_t fixedValue = propSymOpnd->GetFieldValueAsFixedData();
        Assert(instr->IsProfiledInstr());
        ValueType valType = instr->AsProfiledInstr()->u.FldInfo().valueType;
        if (fixedValue && ((Js::TaggedInt::Is(fixedValue) && (valType.IsUninitialized() || valType.IsLikelyInt())) || PHASE_ON1(Js::FixDataVarPropsPhase)))
        {
            // Change Ld[Root]Fld to CheckFixedFld, which doesn't need a dst.
            instr->m_opcode = Js::OpCode::CheckFixedFld;
            IR::RegOpnd* dataValueDstOpnd = instr->UnlinkDst()->AsRegOpnd();
            if (globopt)
            {
                globopt->GenerateBailAtOperation(&instr, !propSymOpnd->HasEquivalentTypeSet() ? IR::BailOutFailedFixedFieldTypeCheck : IR::BailOutFailedEquivalentFixedFieldTypeCheck);
            }
            else
            {
                instr = instr->ConvertToBailOutInstr(instr, !propSymOpnd->HasEquivalentTypeSet() ? IR::BailOutFailedFixedFieldTypeCheck : IR::BailOutFailedEquivalentFixedFieldTypeCheck);
            }

            IR::Instr* loadInstr = IR::Instr::NewConstantLoad(dataValueDstOpnd, (intptr_t)fixedValue, valType, instr->m_func);

            OUTPUT_VERBOSE_TRACE(Js::UseFixedDataPropsPhase,
                _u("FixedFields: Replacing the source (fixed Data prop) with property id %u with 0x%x .\n"),
                propSymOpnd->GetPropertyId(), fixedValue);

            instr->InsertAfter(loadInstr);
            propSymOpnd->SetUsesFixedValue(true);
            return fixedValue;
        }
    }
    return 0;
}

///----------------------------------------------------------------------------
///
/// Instr::IsEqual
///     Check if this instruction is the same instruction as compareInstr. Two
/// instructions are equal if kind, opcode, dst, src1 and src2 from both instrs
/// are the same.
///
///----------------------------------------------------------------------------
bool
Instr::IsEqual(IR::Instr *compareInstr) const
{
    Assert(compareInstr);
    if (this->GetKind() == compareInstr->GetKind()
        && this->m_opcode == compareInstr->m_opcode)
    {
        IR::Opnd *dst = this->GetDst();
        IR::Opnd *src1 = this->GetSrc1();
        IR::Opnd *src2 = this->GetSrc2();
        IR::Opnd *compareDst = compareInstr->GetDst();
        IR::Opnd *compareSrc1 = compareInstr->GetSrc1();
        IR::Opnd *compareSrc2 = compareInstr->GetSrc2();

        // when both dst and compareDst are null, they are equal, same applies to src1, src2
        if ((dst != compareDst) && (!dst || !compareDst || !dst->IsEqual(compareDst)))
        {
            return false;
        }
        if ((src1 != compareSrc1) && (!src1 || !compareSrc1 || !src1->IsEqual(compareSrc1)))
        {
            return false;
        }
        if ((src2 != compareSrc2) && (!src2 || !compareSrc2 || !src2->IsEqual(compareSrc2)))
        {
            return false;
        }

        return true;
    }
    else
    {
        return false;
    }
}

///----------------------------------------------------------------------------
///
/// Instr::InsertBefore
///
///     Insert 'instr' before 'this' instruction.
///
///----------------------------------------------------------------------------

void
Instr::InsertBefore(Instr *instr)
{
    Assert(!instr->IsLinked());

    Instr * prevInstr = this->m_prev;
    instr->m_prev = prevInstr;
    this->m_prev = instr;

    if (prevInstr)
    {
        prevInstr->m_next = instr;
    }
    instr->m_next = this;
}

///----------------------------------------------------------------------------
///
/// Instr::InsertAfter
///
///     Insert 'instr' after 'this' instruction.
///
///----------------------------------------------------------------------------

void
Instr::InsertAfter(Instr *instr)
{
    Assert(!instr->IsLinked());

    Instr * nextInstr = this->m_next;
    instr->m_next = nextInstr;
    this->m_next = instr;

    if (nextInstr)
    {
        nextInstr->m_prev = instr;
    }
    instr->m_prev = this;
}

///----------------------------------------------------------------------------
///
/// Instr::InsertRangeBefore
///
///----------------------------------------------------------------------------
void
Instr::InsertRangeBefore(Instr *startInstr, Instr *endInstr)
{
    Instr * prevInstr = this->m_prev;

    startInstr->m_prev = prevInstr;
    this->m_prev = endInstr;

    if (prevInstr)
    {
        prevInstr->m_next = startInstr;
    }
    endInstr->m_next = this;
}

///----------------------------------------------------------------------------
///
/// Instr::InsertMultipleBefore - Inserts multiple instr
///
///----------------------------------------------------------------------------
void
Instr::InsertMultipleBefore(Instr *endInstr)
{
    Instr *startInstr = endInstr->m_prev;

    if (startInstr) // more than one instruction to insert
    {
        while (startInstr->m_prev)
        {
            startInstr = startInstr->m_prev;
        }
        return this->InsertRangeBefore(startInstr, endInstr);
    }
    return this->InsertBefore(endInstr);
}

///----------------------------------------------------------------------------
///
/// Instr::InsertRangeAfter
///
///----------------------------------------------------------------------------
void
Instr::InsertRangeAfter(Instr *startInstr, Instr *endInstr)
{
    Instr * nextInstr = this->m_next;

    endInstr->m_next = nextInstr;
    this->m_next = startInstr;

    if (nextInstr)
    {
        nextInstr->m_prev = endInstr;
    }
    startInstr->m_prev = this;
}

///----------------------------------------------------------------------------
///
/// Instr::InsertMultipleAfter - Inserts multiple instr
///
///----------------------------------------------------------------------------
void
Instr::InsertMultipleAfter(Instr *endInstr)
{
    Instr *startInstr = endInstr->m_prev;

    if (startInstr) //more than one instruction to insert
    {
        while (startInstr->m_prev)
        {
            startInstr = startInstr->m_prev;
        }
        return this->InsertRangeAfter(startInstr, endInstr);
    }
    return this->InsertAfter(endInstr);
}


///----------------------------------------------------------------------------
///
/// Instr::Free
///
///     Free this instruction by putting it on a free list.
///
///----------------------------------------------------------------------------

void
Instr::Free()
{
    AssertMsg(!this->IsLabelInstr() || !this->AsLabelInstr()->m_hasNonBranchRef,
        "Cannot free label with non-branch reference");

    switch (this->GetKind())
    {
    case InstrKindBranch:
        {
            IR::BranchInstr *branchInstr = this->AsBranchInstr();
            branchInstr->ClearTarget();
            break;
        }
    }

    IR::Opnd * dstOpnd = this->GetDst();
    if (dstOpnd)
    {
        StackSym * stackSym = dstOpnd->GetStackSym();
        if (stackSym)
        {
            if (stackSym->m_isSingleDef)
            {
                Assert(!stackSym->m_isEncodedConstant);
                if (stackSym->m_instrDef == this)
                {
                    Assert(!dstOpnd->isFakeDst);
                    if (stackSym->IsConst())
                    {
                        // keep the instruction around so we can get the constant value
                        // from the symbol
                        return;
                    }
                    Assert(this->m_func->GetTopFunc()->allowRemoveBailOutArgInstr || !stackSym->m_isBailOutReferenced);
                }
                else
                {
                    Assert(dstOpnd->isFakeDst);
                }
            }
            else
            {
                // Encoded constants are not single-defs anymore, and therefore not isConst.
                Assert((!stackSym->m_isConst && stackSym->constantValue == 0)
                    || (stackSym->m_isEncodedConstant && stackSym->constantValue != 0));
            }
        }
        this->FreeDst();
    }
    if (this->GetSrc1())
    {
        this->FreeSrc1();
    }
    if (this->GetSrc2())
    {
        // This pattern isn't so unusual:
        //     src = instr->UnlinkSrc1();
        //     instr->Remove();
        this->FreeSrc2();
    }

    ClearBailOutInfo();
    JitAdelete(this->m_func->m_alloc, this);
}

///----------------------------------------------------------------------------
///
/// Instr::Unlink
///
///     Unlink this instr from the instr list.
///
///----------------------------------------------------------------------------

void
Instr::Unlink()
{
    m_prev->m_next = m_next;
    if (m_next)
    {
        m_next->m_prev = m_prev;
    }
    else
    {
        Assert(this == this->m_func->m_tailInstr);
    }

#if DBG_DUMP
    // Transferring the globOptInstrString to the next non-Label Instruction
    if(this->globOptInstrString != nullptr && m_next && m_next->globOptInstrString == nullptr && !m_next->IsLabelInstr())
    {
        m_next->globOptInstrString = this->globOptInstrString;
    }
#endif

#if DBG
    m_prev = nullptr;
    m_next = nullptr;
#endif
}

///----------------------------------------------------------------------------
///
/// Instr::Remove
///
///     Unlink and free this instr.
///
///----------------------------------------------------------------------------

void
Instr::Remove()
{
    this->Unlink();
    this->Free();
}

void
Instr::SwapOpnds()
{
    IR::Opnd *opndTemp = m_src1;
    m_src1 = m_src2;
    m_src2 = opndTemp;
}

Instr *
Instr::CopyWithoutDst()
{
    return Copy(false /*copyDst*/);
}

// Copy a vanilla instruction.
Instr *
Instr::Copy(bool copyDst)
{
    Instr * instrCopy;

    if (this->HasBailOutInfo() || this->HasAuxBailOut())
    {
        instrCopy = BailOutInstr::New(this->m_opcode, this->GetBailOutKind(), this->GetBailOutInfo(), this->m_func);
        instrCopy->SetByteCodeOffset(this->GetByteCodeOffset());
        if (this->HasAuxBailOut())
        {
            instrCopy->hasAuxBailOut = true;
            instrCopy->SetAuxBailOutKind(this->GetAuxBailOutKind());
        }
    }
    else
    {
        switch (this->GetKind())
        {
        case InstrKindInstr:
            instrCopy = Instr::New(this->m_opcode, this->m_func);
            break;

        case InstrKindProfiled:
            instrCopy = this->AsProfiledInstr()->CopyProfiledInstr();
            break;

        case InstrKindJitProfiling:
            instrCopy = this->AsJitProfilingInstr()->CopyJitProfiling();
            break;

        case InstrKindPragma:
            instrCopy = this->AsPragmaInstr()->CopyPragma();
            break;

        default:
            instrCopy = nullptr;
            AnalysisAssertMsg(UNREACHED, "Copy of other instr kinds NYI");
        }
    }

    Opnd * opnd = this->GetDst();
    if (copyDst && opnd)
    {
        instrCopy->SetDst(opnd->Copy(this->m_func));
    }
    opnd = this->GetSrc1();
    if (opnd)
    {
        instrCopy->SetSrc1(opnd->Copy(this->m_func));
        opnd = this->GetSrc2();
        if (opnd)
        {
            instrCopy->SetSrc2(opnd->Copy(this->m_func));
        }
    }

    instrCopy->isInlineeEntryInstr = this->isInlineeEntryInstr;

    if (this->m_func->DoMaintainByteCodeOffset())
    {
        instrCopy->SetByteCodeOffset(this->GetByteCodeOffset());
    }
    instrCopy->usesStackArgumentsObject = this->usesStackArgumentsObject;
    return instrCopy;
}

LabelInstr *
LabelInstr::CloneLabel(BOOL fCreate)
{
    Func * func = this->m_func;
    Cloner * cloner = func->GetCloner();
    IR::LabelInstr * instrLabel = nullptr;

    AssertMsg(cloner, "Use Func::BeginClone to initialize cloner");

    if (cloner->labelMap == nullptr)
    {
        if (!fCreate)
        {
            return nullptr;
        }
        cloner->labelMap = HashTable<LabelInstr*>::New(cloner->alloc, 7);
    }
    else
    {
        IR::LabelInstr ** map = cloner->labelMap->Get(this->m_id);
        if (map)
        {
            instrLabel = *map;
        }
    }

    if (instrLabel == nullptr)
    {
        if (!fCreate)
        {
            return nullptr;
        }
        if (this->IsProfiledLabelInstr())
        {
            instrLabel = IR::ProfiledLabelInstr::New(this->m_opcode, func, this->AsProfiledLabelInstr()->loopImplicitCallFlags, this->AsProfiledLabelInstr()->loopFlags);
#if DBG
            instrLabel->AsProfiledLabelInstr()->loopNum = this->AsProfiledLabelInstr()->loopNum;
#endif
        }
        else
        {
            instrLabel = IR::LabelInstr::New(this->m_opcode, func, this->isOpHelper);
        }
        instrLabel->m_isLoopTop = this->m_isLoopTop;
        cloner->labelMap->FindOrInsert(instrLabel, this->m_id);
    }

    return instrLabel;
}

ProfiledLabelInstr::ProfiledLabelInstr(JitArenaAllocator * allocator)
    : LabelInstr(allocator)
{
}

ProfiledLabelInstr *
ProfiledLabelInstr::New(Js::OpCode opcode, Func *func, Js::ImplicitCallFlags flags, Js::LoopFlags loopFlags)
{
    ProfiledLabelInstr * profiledLabelInstr = JitAnew(func->m_alloc, ProfiledLabelInstr, func->m_alloc);
    profiledLabelInstr->Init(opcode, InstrKindProfiledLabel, func, false);
    profiledLabelInstr->loopImplicitCallFlags = flags;
    profiledLabelInstr->loopFlags = loopFlags;
    return profiledLabelInstr;
}

void
BranchInstr::RetargetClonedBranch()
{
    IR::LabelInstr * instrLabel = this->m_branchTarget->CloneLabel(false);
    if (instrLabel == nullptr)
    {
        // Jumping outside the cloned range. No retarget.
        return;
    }

    this->SetTarget(instrLabel);
}

PragmaInstr *
PragmaInstr::ClonePragma()
{
    return this->CopyPragma();
}

PragmaInstr *
PragmaInstr::CopyPragma()
{
    IR::PragmaInstr * instrPragma = IR::PragmaInstr::New(this->m_opcode, 0, this->m_func);
    return instrPragma;
}

Instr *
Instr::CloneInstr() const
{
    if (this->HasBailOutInfo() || this->HasAuxBailOut())
    {
        return ((BailOutInstr *)this)->CloneBailOut();
    }

    IR::Instr *clone = IR::Instr::New(this->m_opcode, this->m_func);
    clone->isInlineeEntryInstr = this->isInlineeEntryInstr;

    return clone;
}

// Clone a vanilla instruction, replacing single-def StackSym's with new syms where appropriate.
Instr *
Instr::Clone()
{
    Func * func = this->m_func;
    Cloner *cloner = func->GetCloner();
    IR::Instr * instrClone;
    IR::Opnd * opnd;

    switch (this->GetKind())
    {
    case InstrKindInstr:
        instrClone = this->CloneInstr();
        break;
    case InstrKindBranch:
        instrClone = this->AsBranchInstr()->CloneBranchInstr();
        break;
    case InstrKindProfiled:
        instrClone = this->AsProfiledInstr()->CloneProfiledInstr();
        break;
    case InstrKindLabel:
    case InstrKindProfiledLabel:
        instrClone = this->AsLabelInstr()->CloneLabel(true);
        break;
    case InstrKindPragma:
        instrClone = this->AsPragmaInstr()->ClonePragma();
        break;
    case InstrKindJitProfiling:
        instrClone = this->AsJitProfilingInstr()->CloneJitProfiling();
        break;
    default:
        AssertMsg(0, "Clone of this instr kind NYI");
        return nullptr;
    }

    opnd = this->GetDst();
    if (opnd)
    {
        instrClone->SetDst(opnd->CloneDef(func));
    }
    opnd = this->GetSrc1();
    if (opnd)
    {
        instrClone->SetSrc1(opnd->CloneUse(func));
        opnd = this->GetSrc2();
        if (opnd)
        {
            instrClone->SetSrc2(opnd->CloneUse(func));
        }
    }
    if (this->m_func->DoMaintainByteCodeOffset())
    {
        instrClone->SetByteCodeOffset(this->GetByteCodeOffset());
    }
    instrClone->usesStackArgumentsObject = this->usesStackArgumentsObject;
    cloner->AddInstr(this, instrClone);

    return instrClone;
}

// Clone a range of instructions.
Instr *
Instr::CloneRange(
    Instr * instrStart, Instr * instrLast, Instr * instrAfter, Lowerer *lowerer, JitArenaAllocator * alloc, bool (*fMapTest)(IR::Instr*), bool clonedInstrGetOrigArgSlotSym)
{
    IR::Instr * instrReturn = instrAfter;

    Func * topFunc = instrStart->m_func->GetTopFunc();
    topFunc->BeginClone(lowerer, alloc);
    topFunc->GetCloner()->clonedInstrGetOrigArgSlotSym = clonedInstrGetOrigArgSlotSym;

    FOREACH_INSTR_IN_RANGE(instr, instrStart, instrLast)
    {
        Instr * instrClone = instr->Clone();
        instrAfter->InsertAfter(instrClone);
        instrAfter = instrClone;
        instr->isCloned = true;
        if (fMapTest(instrClone))
        {
            IR::LabelInstr *instrLabel = IR::LabelInstr::New(Js::OpCode::Label, instr->m_func);
            instrClone->InsertBefore(instrLabel);
            topFunc->GetCloneMap()->Item(instr, instrLabel);
        }
    }
    NEXT_INSTR_IN_RANGE;

    topFunc->EndClone();

    return instrReturn;
}

///----------------------------------------------------------------------------
///
/// Instr::MoveRangeAfter
///
///     Move a range of instruction after another instruction
///
///----------------------------------------------------------------------------

void
Instr::MoveRangeAfter(Instr * instrStart, Instr * instrLast, Instr * instrAfter)
{
    if (instrLast->m_next != nullptr)
    {
        instrLast->m_next->m_prev = instrStart->m_prev;
    }
    else
    {
        instrLast->m_func->m_tailInstr = instrStart->m_prev;
    }

    if (instrStart->m_prev != nullptr)
    {
        instrStart->m_prev->m_next = instrLast->m_next;
    }
    else
    {
        instrStart->m_func->m_headInstr = instrLast->m_next;
    }

    instrStart->m_prev = instrAfter;
    instrLast->m_next = instrAfter->m_next;
    if (instrAfter->m_next != nullptr)
    {
        instrAfter->m_next->m_prev = instrLast;
    }
    else
    {
        instrAfter->m_func->m_tailInstr = instrLast;
    }
    instrAfter->m_next = instrStart;
}

JitProfilingInstr *
JitProfilingInstr::New(Js::OpCode opcode, Opnd *dstOpnd, Opnd *src1Opnd, Opnd *src2Opnd, Func * func)
{
    JitProfilingInstr * profiledInstr = JitProfilingInstr::New(opcode, dstOpnd, src1Opnd, func);
    profiledInstr->SetSrc2(src2Opnd);

    return profiledInstr;
}

JitProfilingInstr *
JitProfilingInstr::New(Js::OpCode opcode, Opnd *dstOpnd, Opnd *src1Opnd, Func * func)
{
    Assert(func->DoSimpleJitDynamicProfile());

    JitProfilingInstr * profiledInstr = JitAnew(func->m_alloc, IR::JitProfilingInstr);
    profiledInstr->Init(opcode, InstrKindJitProfiling, func);

    if (dstOpnd)
    {
        profiledInstr->SetDst(dstOpnd);
    }
    if (src1Opnd)
    {
        profiledInstr->SetSrc1(src1Opnd);
    }

#if DBG
    profiledInstr->profileId = Js::Constants::NoProfileId;
    profiledInstr->arrayProfileId = Js::Constants::NoProfileId;
    profiledInstr->inlineCacheIndex = Js::Constants::NoInlineCacheIndex;
    Assert(profiledInstr->loopNumber == 0u - 1);
#endif

    // default these to false.
    profiledInstr->isProfiledReturnCall = false;
    profiledInstr->isBeginSwitch = false;
    profiledInstr->isNewArray = false;
    profiledInstr->isLoopHelper = false;

    return profiledInstr;
}

JitProfilingInstr*
JitProfilingInstr::CloneJitProfiling() const
{
    // Adapted from Profiled::CloneProfiledInstr. Note that the dst and srcs are not set.

    Assert(!(this->HasBailOutInfo() || this->HasAuxBailOut())); // Shouldn't have bailout info in a jitprofiling instr

    return this->CopyJitProfiling();
}


JitProfilingInstr*
JitProfilingInstr::CopyJitProfiling() const
{
    // Adapted from Profiled::CopyProfiledInstr. Note that the dst and srcs are not set.

    IR::JitProfilingInstr * jitProfInstr;

    jitProfInstr = JitAnew(this->m_func->m_alloc, IR::JitProfilingInstr);
    jitProfInstr->Init(this->m_opcode, InstrKindProfiled, this->m_func);

    jitProfInstr->isProfiledReturnCall = this->isProfiledReturnCall;
    jitProfInstr->isBeginSwitch = this->isBeginSwitch;
    jitProfInstr->isNewArray = this->isNewArray;
    jitProfInstr->isLoopHelper = this->isLoopHelper;

    jitProfInstr->profileId = this->profileId;
    jitProfInstr->arrayProfileId = this->arrayProfileId;
    jitProfInstr->inlineCacheIndex = this->inlineCacheIndex;
    Assert(jitProfInstr->loopNumber == this->loopNumber);

    return jitProfInstr;
}

ProfiledInstr *
ProfiledInstr::New(Js::OpCode opcode, Opnd *dstOpnd, Opnd *src1Opnd, Opnd *src2Opnd, Func * func)
{
    ProfiledInstr * profiledInstr = ProfiledInstr::New(opcode, dstOpnd, src1Opnd, func);
    profiledInstr->SetSrc2(src2Opnd);

    return profiledInstr;
}

ProfiledInstr *
ProfiledInstr::New(Js::OpCode opcode, Opnd *dstOpnd, Opnd *src1Opnd, Func * func)
{
    ProfiledInstr * profiledInstr = JitAnew(func->m_alloc, IR::ProfiledInstr);
    profiledInstr->Init(opcode, InstrKindProfiled, func);

    if (dstOpnd)
    {
        profiledInstr->SetDst(dstOpnd);
    }
    if (src1Opnd)
    {
        profiledInstr->SetSrc1(src1Opnd);
    }

    profiledInstr->u.ldElemInfo = nullptr;
    return profiledInstr;
}

ProfiledInstr *
ProfiledInstr::CloneProfiledInstr() const
{
    IR::ProfiledInstr * profiledInstr;
    if (this->HasBailOutInfo() || this->HasAuxBailOut())
    {
        profiledInstr = ((ProfiledBailOutInstr *)this)->CloneBailOut();
        profiledInstr->u = this->u;
    }
    else
    {
        profiledInstr = this->CopyProfiledInstr();
    }

    return profiledInstr;
}

ProfiledInstr *
ProfiledInstr::CopyProfiledInstr() const
{
    IR::ProfiledInstr * profiledInstr;

    profiledInstr = JitAnew(this->m_func->m_alloc, IR::ProfiledInstr);
    profiledInstr->Init(this->m_opcode, InstrKindProfiled, this->m_func);
    profiledInstr->u = this->u;

    return profiledInstr;
}

ByteCodeUsesInstr *
ByteCodeUsesInstr::New(IR::Instr * originalBytecodeInstr)
{
    Func* func = originalBytecodeInstr->m_func;
    ByteCodeUsesInstr * byteCodeUses = JitAnew(func->m_alloc, IR::ByteCodeUsesInstr);
    byteCodeUses->Init(Js::OpCode::ByteCodeUses, InstrKindByteCodeUses, func);
    byteCodeUses->byteCodeUpwardExposedUsed = nullptr;
    byteCodeUses->propertySymUse = nullptr;
    byteCodeUses->SetByteCodeOffset(originalBytecodeInstr);
    return byteCodeUses;
}

ByteCodeUsesInstr *
ByteCodeUsesInstr::New(Func * func, uint32 offset)
{
    ByteCodeUsesInstr * byteCodeUses = JitAnew(func->m_alloc, IR::ByteCodeUsesInstr);
    byteCodeUses->Init(Js::OpCode::ByteCodeUses, InstrKindByteCodeUses, func);
    byteCodeUses->byteCodeUpwardExposedUsed = nullptr;
    byteCodeUses->propertySymUse = nullptr;
    byteCodeUses->SetByteCodeOffset(offset);
    return byteCodeUses;
}

const BVSparse<JitArenaAllocator> * ByteCodeUsesInstr::GetByteCodeUpwardExposedUsed() const
{
    return this->byteCodeUpwardExposedUsed;
}

// In the case of instances where you would like to add a ByteCodeUses to some sym,
// which doesn't have an operand associated with it (like a block closure sym), use
// this to set it without needing to pass the check for JIT-Optimized registers.
void ByteCodeUsesInstr::SetNonOpndSymbol(uint symId)
{
    if (!this->byteCodeUpwardExposedUsed)
    {
        this->byteCodeUpwardExposedUsed = JitAnew(m_func->m_alloc, BVSparse<JitArenaAllocator>, m_func->m_alloc);
    }
    this->byteCodeUpwardExposedUsed->Set(symId);
}

// In cases where the operand you're working on may be changed between when you get
// access to it and when you determine that you can set it in the ByteCodeUsesInstr
// set method, cache the values and use this caller.
void ByteCodeUsesInstr::SetRemovedOpndSymbol(bool isJITOptimizedReg, uint symId)
{
    if (isJITOptimizedReg)
    {
        AssertMsg(false, "Tried to add a jit-optimized register to a ByteCodeUses instruction!");
        // Although we assert on debug builds, we should actually be ok with release builds
        // if we ignore the operand; not ignoring it, however, can cause us to introduce an
        // inconsistency in bytecode register lifetimes.
        return;
    }
    if(!this->byteCodeUpwardExposedUsed)
    {
        this->byteCodeUpwardExposedUsed = JitAnew(m_func->m_alloc, BVSparse<JitArenaAllocator>, m_func->m_alloc);
    }
    this->byteCodeUpwardExposedUsed->Set(symId);
}

void ByteCodeUsesInstr::Set(IR::Opnd * originalOperand)
{
    Assert(originalOperand && originalOperand->GetStackSym());
    bool isJITOptimizedReg = originalOperand->GetIsJITOptimizedReg();
    SymID symId = originalOperand->GetStackSym()->m_id;
    if (isJITOptimizedReg)
    {
        AssertMsg(false, "Tried to add a jit-optimized register to a ByteCodeUses instruction!");
        // Although we assert on debug builds, we should actually be ok with release builds
        // if we ignore the operand; not ignoring it, however, can cause us to introduce an
        // inconsistency in bytecode register lifetimes.
        return;
    }
    if (!this->byteCodeUpwardExposedUsed)
    {
        this->byteCodeUpwardExposedUsed = JitAnew(m_func->m_alloc, BVSparse<JitArenaAllocator>, m_func->m_alloc);
    }
    this->byteCodeUpwardExposedUsed->Set(symId);
}

void ByteCodeUsesInstr::Clear(uint symId)
{
    Assert(byteCodeUpwardExposedUsed != nullptr);
    this->byteCodeUpwardExposedUsed->Clear(symId);
}

void ByteCodeUsesInstr::SetBV(BVSparse<JitArenaAllocator>* newbv)
{
    Assert(byteCodeUpwardExposedUsed == nullptr && newbv != nullptr);
    byteCodeUpwardExposedUsed = newbv;
}

// If possible, we want to aggregate with subsequent ByteCodeUses Instructions, so
// that we can do some optimizations in other places where we can simplify args in
// a compare, but still need to generate them for bailouts. Without this, we cause
// problems because we end up with an instruction losing atomicity in terms of its
// bytecode use and generation lifetimes.
void ByteCodeUsesInstr::AggregateFollowingByteCodeUses()
{
    IR::Instr* scanner = this->m_next;
    while (scanner && scanner->m_opcode == Js::OpCode::ByteCodeUses && scanner->GetByteCodeOffset() == this->GetByteCodeOffset() && scanner->GetDst() == nullptr)
    {
        IR::ByteCodeUsesInstr* target = scanner->AsByteCodeUsesInstr();
        this->Aggregate(target);
        scanner = scanner->m_next;
    }
}

void ByteCodeUsesInstr::Aggregate(ByteCodeUsesInstr * byteCodeUsesInstr)
{
    Assert(this->m_func == byteCodeUsesInstr->m_func);
    if (byteCodeUsesInstr->byteCodeUpwardExposedUsed)
    {
        Assert(byteCodeUsesInstr->GetDst() == nullptr);
        if (this->byteCodeUpwardExposedUsed)
        {
            this->byteCodeUpwardExposedUsed->Or(byteCodeUsesInstr->byteCodeUpwardExposedUsed);
            JitAdelete(byteCodeUsesInstr->byteCodeUpwardExposedUsed->GetAllocator(), byteCodeUsesInstr->byteCodeUpwardExposedUsed);
            byteCodeUsesInstr->byteCodeUpwardExposedUsed = nullptr;
        }
        else
        {
            this->byteCodeUpwardExposedUsed = byteCodeUsesInstr->byteCodeUpwardExposedUsed;
            byteCodeUsesInstr->byteCodeUpwardExposedUsed = nullptr;
        }
    }
}

bool Instr::CanAggregateByteCodeUsesAcrossInstr(Instr * instr)
{
    return !instr->StartsBasicBlock() && 
        instr->m_func == this->m_func &&
        ((instr->GetByteCodeOffset() == Js::Constants::NoByteCodeOffset) ||
        (instr->GetByteCodeOffset() == this->GetByteCodeOffset()));
}

bool IR::Instr::IsStFldVariant() const
{
    return this->m_opcode == Js::OpCode::StFld ||
        this->m_opcode == Js::OpCode::StFldStrict ||
        this->m_opcode == Js::OpCode::StLocalFld ||
        this->m_opcode == Js::OpCode::StRootFld ||
        this->m_opcode == Js::OpCode::StRootFldStrict ||
        this->m_opcode == Js::OpCode::StSuperFld ||
        this->m_opcode == Js::OpCode::StSuperFldStrict;
}

bool IR::Instr::IsStElemVariant() const
{
    return this->m_opcode == Js::OpCode::StElemI_A ||
        this->m_opcode == Js::OpCode::StElemI_A_Strict ||
        this->m_opcode == Js::OpCode::StElemC;
}

bool IR::Instr::DontHoistBailOnNoProfileAboveInGeneratorFunction() const
{
    return this->m_opcode == Js::OpCode::GeneratorResumeYield ||
        this->m_opcode == Js::OpCode::GeneratorCreateInterpreterStackFrame;
}

bool IR::Instr::CanChangeFieldValueWithoutImplicitCall() const
{
    // TODO: Why is InitFld necessary?
    return this->IsStFldVariant() || this->IsStElemVariant();
}

// If LazyBailOut is the only BailOutKind on the instruction, the BailOutInfo is cleared.
// Otherwise, we remove the LazyBailOut kind from the instruction and still keep the BailOutInfo.
void IR::Instr::ClearLazyBailOut()
{
    if (!this->HasBailOutInfo())
    {
        return;
    }

    if (this->OnlyHasLazyBailOut())
    {
        this->ClearBailOutInfo();
    }
    else
    {
        this->GetBailOutInfo()->RestoreUseOfDst();
        this->SetBailOutKind(BailOutInfo::WithoutLazyBailOut(this->GetBailOutKind()));
    }

    Assert(!this->HasLazyBailOut());
}

int IR::Instr::GetOpndCount() const
{
    return (this->m_src1 ? 1 : 0) + (this->m_src2 ? 1 : 0) + (this->m_dst ? 1 : 0);
}

bool IR::Instr::AreAllOpndsTypeSpecialized() const
{
    bool src1TypeSpec = !this->m_src1 || (this->m_src1->GetStackSym() && this->m_src1->GetStackSym()->IsTypeSpec());
    bool src2TypeSpec = !this->m_src2 || (this->m_src2->GetStackSym() && this->m_src2->GetStackSym()->IsTypeSpec());
    bool dstTypeSpec = !this->m_dst || (this->m_dst->GetStackSym() && this->m_dst->GetStackSym()->IsTypeSpec());
    return src1TypeSpec && src2TypeSpec && dstTypeSpec && this->GetOpndCount() > 0;
}

bool IR::Instr::OnlyHasLazyBailOut() const
{
    return this->HasBailOutInfo() && BailOutInfo::OnlyHasLazyBailOut(this->GetBailOutKind());
}

bool IR::Instr::HasLazyBailOut() const
{
    return this->HasBailOutInfo() && BailOutInfo::HasLazyBailOut(this->GetBailOutKind());
}

bool IR::Instr::HasPreOpBailOut() const
{
    return this->HasBailOutInfo() && this->GetBailOutInfo()->bailOutOffset == this->GetByteCodeOffset();
}

bool IR::Instr::HasPostOpBailOut() const
{
    return this->HasBailOutInfo() && this->GetBailOutInfo()->bailOutOffset > this->GetByteCodeOffset();
}

BailOutInfo *
Instr::GetBailOutInfo() const
{
    Assert(this->HasBailOutInfo() || this->HasAuxBailOut());
    switch (this->m_kind)
    {
    case InstrKindInstr:
        return ((BailOutInstr const *)this)->bailOutInfo;
    case InstrKindProfiled:
        return ((ProfiledBailOutInstr const *)this)->bailOutInfo;
    case InstrKindBranch:
        return ((BranchBailOutInstr const *)this)->bailOutInfo;
    default:
        Assert(false);
        __assume(false);
    }
}

BailOutKind
Instr::GetBailOutKind() const
{
    Assert(this->HasBailOutInfo());
    switch (this->m_kind)
    {
    case InstrKindInstr:
        return ((BailOutInstr const *)this)->bailOutKind;
    case InstrKindProfiled:
        return ((ProfiledBailOutInstr const *)this)->bailOutKind;
    case InstrKindBranch:
        return ((BranchBailOutInstr const *)this)->bailOutKind;
    default:
        Assert(false);
        return BailOutInvalid;
    }
}

BailOutKind
Instr::GetBailOutKindNoBits() const
{
    return GetBailOutKind() & ~IR::BailOutKindBits;
}

BailOutKind
Instr::GetAuxBailOutKind() const
{
    Assert(this->HasAuxBailOut());
    switch (this->m_kind)
    {
    case InstrKindInstr:
        return ((BailOutInstr const *)this)->auxBailOutKind;
    case InstrKindProfiled:
        return ((ProfiledBailOutInstr const *)this)->auxBailOutKind;
    case InstrKindBranch:
        return ((BranchBailOutInstr const *)this)->auxBailOutKind;
    default:
        Assert(false);
        return BailOutInvalid;
    }
}

void Instr::SetBailOutKind(const IR::BailOutKind bailOutKind)
{
    Assert(this->HasBailOutInfo());
    Assert(bailOutKind != IR::BailOutInvalid);
    this->SetBailOutKind_NoAssert(bailOutKind);
}

// Helper to set bail out kind, doesn't assert.
void Instr::SetBailOutKind_NoAssert(const IR::BailOutKind bailOutKind)
{
    Assert(IsValidBailOutKindAndBits(bailOutKind));
    switch (this->m_kind)
    {
    case InstrKindInstr:
        ((BailOutInstr *)this)->bailOutKind = bailOutKind;
        break;
    case InstrKindProfiled:
        ((ProfiledBailOutInstr *)this)->bailOutKind = bailOutKind;
        break;
    case InstrKindBranch:
        ((BranchBailOutInstr *)this)->bailOutKind = bailOutKind;
        break;
    default:
        Assert(false);
        __assume(false);
    }
}

void Instr::SetAuxBailOutKind(const IR::BailOutKind bailOutKind)
{
    switch (this->m_kind)
    {
    case InstrKindInstr:
        ((BailOutInstr *)this)->auxBailOutKind = bailOutKind;
        break;
    case InstrKindProfiled:
        ((ProfiledBailOutInstr *)this)->auxBailOutKind = bailOutKind;
        break;
    case InstrKindBranch:
        ((BranchBailOutInstr *)this)->auxBailOutKind = bailOutKind;
        break;
    default:
        Assert(false);
        __assume(false);
    }
}

BailOutInfo *
Instr::UnlinkBailOutInfo()
{
    BailOutInfo *bailOutInfo;
    Assert(this->HasBailOutInfo() || this->HasAuxBailOut());

    switch (this->m_kind)
    {
    case InstrKindInstr:
        bailOutInfo = ((BailOutInstr const *)this)->bailOutInfo;
        ((BailOutInstr *)this)->bailOutInfo = nullptr;
        break;
    case InstrKindProfiled:
        bailOutInfo = ((ProfiledBailOutInstr const *)this)->bailOutInfo;
        ((ProfiledBailOutInstr *)this)->bailOutInfo = nullptr;
        break;
    case InstrKindBranch:
        bailOutInfo = ((BranchBailOutInstr const *)this)->bailOutInfo;
        ((BranchBailOutInstr *)this)->bailOutInfo = nullptr;
        break;
    default:
        Assert(false);
        return nullptr;
    }
    Assert(bailOutInfo);
#if 0
    if (bailOutInfo->bailOutInstr == this)
    {
        bailOutInfo->bailOutInstr = nullptr;
    }
#endif
    this->hasBailOutInfo = false;
    this->hasAuxBailOut = false;

    return bailOutInfo;
}

void
Instr::ReplaceBailOutInfo(BailOutInfo *newBailOutInfo)
{
    BailOutInfo *oldBailOutInfo = nullptr;

#if DBG
    newBailOutInfo->wasCopied = true;
#endif

    Assert(this->HasBailOutInfo() || this->HasAuxBailOut());
    switch (this->m_kind)
    {
    case InstrKindInstr:
        oldBailOutInfo = ((BailOutInstr *)this)->bailOutInfo;
        ((BailOutInstr *)this)->bailOutInfo = newBailOutInfo;
        break;
    case InstrKindProfiled:
        oldBailOutInfo = ((ProfiledBailOutInstr *)this)->bailOutInfo;
        ((ProfiledBailOutInstr *)this)->bailOutInfo = newBailOutInfo;
        break;
    case InstrKindBranch:
        AssertMsg(!this->HasBailOutInfo() && this->HasAuxBailOut(), "ReplaceBailOutInfo is not used with InstrKindBranch for non-aux bailout");
        oldBailOutInfo = ((BranchBailOutInstr *)this)->bailOutInfo;
        ((BranchBailOutInstr *)this)->bailOutInfo = newBailOutInfo;
        break;
    default:
        Assert(false);
        __assume(UNREACHED);
    }
    
    if (oldBailOutInfo->bailOutInstr == this && !oldBailOutInfo->sharedBailOutKind)
    {
        Assert(!oldBailOutInfo->wasCloned && !oldBailOutInfo->wasCopied);
        JitArenaAllocator * alloc = this->m_func->m_alloc;
        oldBailOutInfo->Clear(alloc);
        JitAdelete(alloc, oldBailOutInfo);
    }

    return;
}

IR::Instr *Instr::ShareBailOut()
{
    BailOutInfo *const bailOutInfo = GetBailOutInfo();
    bailOutInfo->bailOutInstr = nullptr;
#if DBG
    bailOutInfo->wasCopied = true;
#endif
    IR::Instr *const sharedBail =
        IR::BailOutInstr::New(Js::OpCode::BailTarget, IR::BailOutShared, bailOutInfo, bailOutInfo->bailOutFunc);
    sharedBail->SetByteCodeOffset(this);
    InsertAfter(sharedBail);
    Assert(bailOutInfo->bailOutInstr == sharedBail);
    return sharedBail;
}

void
Instr::UnlinkStartCallFromBailOutInfo(IR::Instr *endInstr) const
{
#ifdef _M_IX86
    // The StartCall instruction is being deleted, or is being moved and may later be deleted,
    // so remove its references from bailouts in the given range.
    // This only happens during cloning, which is rare, and only across the range of instructions
    // that evaluate outgoing arguments, which is long only in synthetic cases.

    Assert(this->m_opcode == Js::OpCode::StartCall);

    if (!this->m_func->hasBailout)
    {
        return;
    }

    FOREACH_INSTR_IN_RANGE(instr, this->m_next, endInstr)
    {
        if (instr->HasBailOutInfo())
        {
            BailOutInfo *bailOutInfo = instr->GetBailOutInfo();
            bailOutInfo->UnlinkStartCall(this);
        }
    }
    NEXT_INSTR_IN_RANGE;
#endif
}

Opnd *Instr::FindCallArgumentOpnd(const Js::ArgSlot argSlot, IR::Instr * *const ownerInstrRef)
{
    Assert(OpCodeAttr::CallInstr(m_opcode));
    Assert(argSlot != static_cast<Js::ArgSlot>(0));

    IR::Instr *argInstr = this;
    Assert(argInstr->GetSrc2());
    Assert(argInstr->GetSrc2()->IsSymOpnd());
    do
    {
        StackSym *const linkSym = argInstr->GetSrc2()->AsSymOpnd()->m_sym->AsStackSym();
        Assert(linkSym->IsSingleDef());
        Assert(linkSym->IsArgSlotSym());

        argInstr = linkSym->m_instrDef;
        Assert(argInstr->GetSrc2());
        if(argInstr->m_opcode == Js::OpCode::ArgOut_A_InlineSpecialized)
        {
            // This is a fake ArgOut, skip it
            continue;
        }

        if(linkSym->GetArgSlotNum() == argSlot)
        {
            if(ownerInstrRef)
            {
                *ownerInstrRef = argInstr;
            }
            return argInstr->GetSrc1();
        }
    } while(argInstr->GetSrc2()->IsSymOpnd());
    return nullptr;
}


bool
Instr::FetchOperands(_Out_writes_(argsOpndLength) IR::Opnd **argsOpnd, uint argsOpndLength)
{
    return this->ForEachCallDirectArgOutInstrBackward([&](IR::Instr *argOutInstr, uint argNum)
    {
        argsOpnd[argNum] = argOutInstr->GetSrc1();
        return argNum == 0;
    }, argsOpndLength);
}

bool Instr::ShouldCheckForNegativeZero() const
{
    return !ignoreNegativeZero;
}

bool Instr::IsDstNotAlwaysConvertedToInt32() const
{
    return !dstIsAlwaysConvertedToInt32;
}

bool Instr::IsDstNotAlwaysConvertedToNumber() const
{
    return !dstIsAlwaysConvertedToNumber;
}

bool Instr::ShouldCheckForIntOverflow() const
{
    return ShouldCheckFor32BitOverflow() || ShouldCheckForNon32BitOverflow();
}

bool Instr::ShouldCheckFor32BitOverflow() const
{
    return !(ignoreIntOverflow || ignoreIntOverflowInRange);
}

bool Instr::ShouldCheckForNon32BitOverflow() const
{
    return ignoreOverflowBitCount != 32;
}

template <typename InstrType> struct IRKindMap;

template <> struct IRKindMap<IR::Instr> { static const IRKind InstrKind = InstrKindInstr; };
template <> struct IRKindMap<IR::ProfiledInstr> { static const IRKind InstrKind = InstrKindProfiled; };
template <> struct IRKindMap<IR::BranchInstr> { static const IRKind InstrKind = InstrKindBranch; };

template <typename InstrType>
BailOutInstrTemplate<InstrType> *
BailOutInstrTemplate<InstrType>::New(Js::OpCode opcode, BailOutKind kind, IR::Instr * bailOutTarget, Func * func)
{
    Assert(func == bailOutTarget->m_func);
    BailOutInfo * bailOutInfo = JitAnew(func->m_alloc, BailOutInfo, bailOutTarget->GetByteCodeOffset(), func);
#if ENABLE_DEBUG_CONFIG_OPTIONS
    bailOutInfo->bailOutOpcode = opcode;
#endif
    return BailOutInstrTemplate::New(opcode, kind, bailOutInfo, func);
}

template <typename InstrType>
BailOutInstrTemplate<InstrType> *
BailOutInstrTemplate<InstrType>::New(Js::OpCode opcode, IR::Opnd *dst, BailOutKind kind, IR::Instr * bailOutTarget, Func * func)
{
    BailOutInstrTemplate *instr = BailOutInstrTemplate::New(opcode, kind, bailOutTarget, func);
    instr->SetDst(dst);

    return instr;
}

template <typename InstrType>
BailOutInstrTemplate<InstrType> *
BailOutInstrTemplate<InstrType>::New(Js::OpCode opcode, IR::Opnd *dst, IR::Opnd *src1, BailOutKind kind, IR::Instr * bailOutTarget, Func * func)
{
    BailOutInstrTemplate *instr = BailOutInstrTemplate::New(opcode, dst, kind, bailOutTarget, func);
    instr->SetSrc1(src1);

    return instr;
}

template <typename InstrType>
BailOutInstrTemplate<InstrType> *
BailOutInstrTemplate<InstrType>::New(Js::OpCode opcode, IR::Opnd *dst, IR::Opnd *src1, IR::Opnd *src2, BailOutKind kind, IR::Instr * bailOutTarget, Func * func)
{
    BailOutInstrTemplate *instr = BailOutInstrTemplate::New(opcode, dst, src1, kind, bailOutTarget, func);
    instr->SetSrc2(src2);

    return instr;
}

template <typename InstrType>
BailOutInstrTemplate<InstrType> *
BailOutInstrTemplate<InstrType>::New(Js::OpCode opcode, BailOutKind kind, BailOutInfo * bailOutInfo, Func * func)
{
    Assert(func == bailOutInfo->bailOutFunc);
    Assert(IsValidBailOutKindAndBits(kind));
    BailOutInstrTemplate * bailOutInstr = JitAnew(func->m_alloc, BailOutInstrTemplate);
    bailOutInstr->Init(opcode, IRKindMap<InstrType>::InstrKind, func);
#if ENABLE_DEBUG_CONFIG_OPTIONS
    bailOutInfo->bailOutOpcode = opcode;
#endif
    bailOutInstr->bailOutInfo = bailOutInfo;
    bailOutInstr->bailOutKind = kind;
    bailOutInstr->auxBailOutKind = BailOutInvalid;

    if (bailOutInfo->bailOutInstr == nullptr)
    {
        bailOutInfo->bailOutInstr = bailOutInstr;
    }
    else if (bailOutInfo->sharedBailOutKind)
    {
        if (bailOutInfo->bailOutInstr->HasBailOutInfo())
        {
            bailOutInfo->sharedBailOutKind = bailOutInfo->bailOutInstr->GetBailOutKind() ==  kind;
        }
        else
        {
            // Rare cases where we have already generated the bailout record. Unlikely they share the same bailout kind as this is hit only when we try to
            // share bailout in lowerer. See Instr::ShareBailOut.
            bailOutInfo->sharedBailOutKind = false;
        }
    }

    func->hasBailout = true;

    // Indicate that the function has bailout instructions
    // This information is used to determine whether to free jitted loop bodies
    // If the function has bailout instructions, we keep the loop bodies alive
    // in case we bail out to the interpreter, so that we can reuse the jitted
    // loop bodies
    func->GetJITOutput()->SetHasBailoutInstr(true);

    return bailOutInstr;
}

template <typename InstrType>
BailOutInstrTemplate<InstrType> *
BailOutInstrTemplate<InstrType>::CloneBailOut() const
{
    Assert(this->m_func->hasBailout);
    Assert(!this->bailOutInfo->wasCloned);

    BailOutInstrTemplate * bailOutInstr = BailOutInstrTemplate::New(this->m_opcode, this->bailOutKind, this->bailOutInfo, this->bailOutInfo->bailOutFunc);
    bailOutInstr->hasAuxBailOut = this->hasAuxBailOut;
    bailOutInstr->auxBailOutKind = this->auxBailOutKind;
    bailOutInstr->bailOutInfo->wasCloned = true;

    // the new copy is in the slow path and generate the real bailout
    bailOutInstr->bailOutInfo->bailOutInstr = bailOutInstr;

    return bailOutInstr;
}

template class BailOutInstrTemplate<IR::Instr>;

///----------------------------------------------------------------------------
///
/// EntryInstr::New
///
///     Create an EntryInstr.
///
///----------------------------------------------------------------------------

EntryInstr *
EntryInstr::New(Js::OpCode opcode, Func *func)
{
    EntryInstr * entryInstr;

    entryInstr = JitAnew(func->m_alloc, IR::EntryInstr);
    entryInstr->Init(opcode, InstrKindEntry, func);
    return entryInstr;
}

///----------------------------------------------------------------------------
///
/// ExitInstr::New
///
///     Create an ExitInstr.
///
///----------------------------------------------------------------------------

ExitInstr *
ExitInstr::New(Js::OpCode opcode, Func *func)
{
    ExitInstr * exitInstr;

    exitInstr = JitAnew(func->m_alloc, IR::ExitInstr);
    exitInstr->Init(opcode, InstrKindExit, func);
    return exitInstr;
}

///----------------------------------------------------------------------------
///
/// LabelInstr::New
///
///     Create a label.
///
///----------------------------------------------------------------------------

LabelInstr *
LabelInstr::New(Js::OpCode opcode, Func *func, bool isOpHelper)
{
    LabelInstr * labelInstr;

    labelInstr = JitAnew(func->m_alloc, IR::LabelInstr, func->m_alloc);
    labelInstr->Init(opcode, InstrKindLabel, func, isOpHelper);
    return labelInstr;
}

void
LabelInstr::Init(Js::OpCode opcode, IRKind kind, Func *func, bool isOpHelper)
{
    // Pass in the region when this is called from anywhere between the Lowerer and EHBailoutPatchUp code?
    __super::Init(opcode, kind, func);
    this->isOpHelper = isOpHelper;

    this->m_pc.pc = nullptr;
    this->m_id = ++(func->GetTopFunc()->m_labelCount);
    AssertMsg(this->m_id != 0, "Label numbers wrapped around?");
}

///----------------------------------------------------------------------------
///
/// LabelInstr::AddLabelRef
///
///     Add a branch to the list of label references.
///
///----------------------------------------------------------------------------

void
LabelInstr::AddLabelRef(BranchInstr *branchRef)
{
    this->labelRefs.Prepend(branchRef);
}

///----------------------------------------------------------------------------
///
/// LabelInstr::RemoveLabelRef
///
///     Remove a branch from the list of label references.
///
///----------------------------------------------------------------------------

void
LabelInstr::RemoveLabelRef(BranchInstr *branchRef)
{
    FOREACH_SLISTCOUNTED_ENTRY_EDITING(BranchInstr*, branchEntry, &this->labelRefs, iter)
    {
        if (branchEntry == branchRef)
        {
            iter.RemoveCurrent();
            return;
        }
    } NEXT_SLISTCOUNTED_ENTRY_EDITING;

    AssertMsg(UNREACHED, "Branch not found on labelRef list");
}

///----------------------------------------------------------------------------
///
/// BranchInstr::New
///
///     Create a Br (unconditional) BranchInstr.
///
///----------------------------------------------------------------------------

BranchInstr *
BranchInstr::New(Js::OpCode opcode, LabelInstr * branchTarget, Func *func)
{
    BranchInstr * branchInstr;

    branchInstr = JitAnew(func->m_alloc, IR::BranchInstr);
    branchInstr->Init(opcode, InstrKindBranch, func);
    branchInstr->SetTarget(branchTarget);
    branchInstr->m_dst = nullptr;
    branchInstr->m_src1 = nullptr;
    branchInstr->m_src2 = nullptr;
    branchInstr->m_byteCodeReg = Js::Constants::NoRegister;

    return branchInstr;
}

///----------------------------------------------------------------------------
///
/// BranchInstr::New
///
///     Create a BrB BranchInstr (1-operand conditional branch).
///
///----------------------------------------------------------------------------

BranchInstr *
BranchInstr::New(Js::OpCode opcode, LabelInstr * branchTarget, Opnd *srcOpnd, Func *func)
{
    BranchInstr * branchInstr;

    branchInstr = BranchInstr::New(opcode, branchTarget, func);

    branchInstr->SetSrc1(srcOpnd);

    return branchInstr;
}


///----------------------------------------------------------------------------
///
/// BranchInstr::New
///
///     Create a BrBReturn BranchInstr (1-operand conditional branch. If condition fails return the result of the condition).
///
///----------------------------------------------------------------------------

BranchInstr *
BranchInstr::New(Js::OpCode opcode, Opnd* destOpnd, LabelInstr * branchTarget, Opnd *srcOpnd, Func *func)
{
    BranchInstr * branchInstr;

    branchInstr = BranchInstr::New(opcode, branchTarget, func);

    branchInstr->SetSrc1(srcOpnd);
    branchInstr->SetDst(destOpnd);

    return branchInstr;
}

///----------------------------------------------------------------------------
///
/// BranchInstr::New
///
///     Create a BrReg2 BranchInstr (2-operand conditional branch).
///
///----------------------------------------------------------------------------

BranchInstr *
BranchInstr::New(Js::OpCode opcode, LabelInstr * branchTarget, Opnd *src1Opnd, Opnd *src2Opnd, Func *func)
{
    BranchInstr * branchInstr;

    branchInstr = BranchInstr::New(opcode, branchTarget, src1Opnd, func);

    branchInstr->SetSrc2(src2Opnd);

    return branchInstr;
}

///----------------------------------------------------------------------------
///
/// MultiBranchInstr::New
///
///     Create a MultiBr BranchInstr (unconditional multi branch).
///
///----------------------------------------------------------------------------

MultiBranchInstr *
MultiBranchInstr::New(Js::OpCode opcode, IR::Opnd * srcOpnd, Func * func)
{
   MultiBranchInstr * multiBranchInstr;

   multiBranchInstr = MultiBranchInstr::New(opcode, func);

   multiBranchInstr->SetSrc1(srcOpnd);

   return multiBranchInstr;
}

MultiBranchInstr *
MultiBranchInstr::New(Js::OpCode opcode, Func * func)
{
   JitArenaAllocator * m_funcAlloc = func->m_alloc;

   MultiBranchInstr * multiBranchInstr;

   multiBranchInstr = JitAnew(m_funcAlloc, IR::MultiBranchInstr);

   multiBranchInstr->Init(opcode, InstrKindBranch, func);

   return multiBranchInstr;
}

bool
BranchInstr::ReplaceTarget(IR::LabelInstr * oldLabelInstr, IR::LabelInstr * newLabelInstr)
{
    if (this->IsMultiBranch())
    {
        return this->AsMultiBrInstr()->ReplaceTarget(oldLabelInstr, newLabelInstr);
    }
    if (this->GetTarget() == oldLabelInstr)
    {
        this->SetTarget(newLabelInstr);
        return true;
    }
    return false;
}

bool
MultiBranchInstr::ReplaceTarget(IR::LabelInstr * oldLabelInstr, IR::LabelInstr * newLabelInstr)
{
    Assert(this->IsMultiBranch());
    bool remapped = false;
    this->UpdateMultiBrLabels([=, &remapped](IR::LabelInstr * targetLabel) -> IR::LabelInstr *
    {
        if (targetLabel == oldLabelInstr)
        {
            this->ChangeLabelRef(targetLabel, newLabelInstr);
            remapped = true;
            return newLabelInstr;
        }
        return targetLabel;
    });
    return remapped;
}

void
MultiBranchInstr::ClearTarget()
{
    Assert(IsMultiBranch());

    MapMultiBrLabels([&](LabelInstr *const targetLabel)
    {
        ChangeLabelRef(targetLabel, nullptr);
    });
    m_branchTargets = nullptr;
}

BranchInstr *
BranchInstr::CloneBranchInstr() const
{
    AssertMsg(!this->IsMultiBranch(),"Cloning Not supported for MultiBranchInstr");
    Func * func = this->m_func;
    // See if the target has already been cloned.
    IR::LabelInstr * instrLabel = this->GetTarget()->CloneLabel(false);
    if (instrLabel == nullptr)
    {
        // We didn't find a clone for this label.
        // We'll go back and retarget the cloned branch if the target turns up in the cloned range.
        instrLabel = this->GetTarget();
        func->GetCloner()->fRetargetClonedBranch = TRUE;
    }
    return IR::BranchInstr::New(this->m_opcode, instrLabel, func);
}

void
BranchInstr::Invert()
{
    /*
     * If one of the operands to a relational operator is 'undefined', the result
     * is always false. Don't invert such branches as they result in a jump to
     * the wrong target.
     */

    switch (this->m_opcode)
    {
    case Js::OpCode::BrGt_A:
        this->m_opcode = Js::OpCode::BrNotGt_A;
        break;

    case Js::OpCode::BrNotGt_A:
        this->m_opcode = Js::OpCode::BrGt_A;
        break;

    case Js::OpCode::BrGe_A:
        this->m_opcode = Js::OpCode::BrNotGe_A;
        break;

    case Js::OpCode::BrNotGe_A:
        this->m_opcode = Js::OpCode::BrGe_A;
        break;

    case Js::OpCode::BrLt_A:
        this->m_opcode = Js::OpCode::BrNotLt_A;
        break;

    case Js::OpCode::BrNotLt_A:
        this->m_opcode = Js::OpCode::BrLt_A;
        break;

    case Js::OpCode::BrLe_A:
        this->m_opcode = Js::OpCode::BrNotLe_A;
        break;

    case Js::OpCode::BrNotLe_A:
        this->m_opcode = Js::OpCode::BrLe_A;
        break;

    case Js::OpCode::BrEq_A:
        this->m_opcode = Js::OpCode::BrNotEq_A;
        break;

    case Js::OpCode::BrNotEq_A:
        this->m_opcode = Js::OpCode::BrEq_A;
        break;

    case Js::OpCode::BrNeq_A:
        this->m_opcode = Js::OpCode::BrNotNeq_A;
        break;

    case Js::OpCode::BrNotNeq_A:
        this->m_opcode = Js::OpCode::BrNeq_A;
        break;

    case Js::OpCode::Br:
        break;

    case Js::OpCode::BrFalse_A:
        this->m_opcode = Js::OpCode::BrTrue_A;
        break;

    case Js::OpCode::BrTrue_A:
        this->m_opcode = Js::OpCode::BrFalse_A;
        break;

    case Js::OpCode::BrSrEq_A:
        this->m_opcode = Js::OpCode::BrSrNotEq_A;
        break;

    case Js::OpCode::BrSrNotEq_A:
        this->m_opcode = Js::OpCode::BrSrEq_A;
        break;

    case Js::OpCode::BrSrNeq_A:
        this->m_opcode = Js::OpCode::BrSrNotNeq_A;
        break;

    case Js::OpCode::BrSrNotNeq_A:
        this->m_opcode = Js::OpCode::BrSrNeq_A;
        break;

    case Js::OpCode::BrOnHasProperty:
        this->m_opcode = Js::OpCode::BrOnNoProperty;
        break;

    case Js::OpCode::BrOnHasLocalProperty:
        this->m_opcode = Js::OpCode::BrOnNoLocalProperty;
        break;

    case Js::OpCode::BrOnNoLocalProperty:
        this->m_opcode = Js::OpCode::BrOnHasLocalProperty;
        break;

    case Js::OpCode::BrOnNoProperty:
        this->m_opcode = Js::OpCode::BrOnHasProperty;
        break;

    case Js::OpCode::BrTrue_I4:
        this->m_opcode = Js::OpCode::BrFalse_I4;
        break;

    case Js::OpCode::BrFalse_I4:
        this->m_opcode = Js::OpCode::BrTrue_I4;
        break;

    case Js::OpCode::BrEq_I4:
        this->m_opcode = Js::OpCode::BrNeq_I4;
        break;

    case Js::OpCode::BrNeq_I4:
        this->m_opcode = Js::OpCode::BrEq_I4;
        break;

    case Js::OpCode::BrGe_I4:
        this->m_opcode = Js::OpCode::BrLt_I4;
        break;

    case Js::OpCode::BrGt_I4:
        this->m_opcode = Js::OpCode::BrLe_I4;
        break;

    case Js::OpCode::BrLe_I4:
        this->m_opcode = Js::OpCode::BrGt_I4;
        break;

    case Js::OpCode::BrLt_I4:
        this->m_opcode = Js::OpCode::BrGe_I4;
        break;

    case Js::OpCode::BrUnGe_A:
        this->m_opcode = Js::OpCode::BrUnLt_A;
        break;

    case Js::OpCode::BrUnGt_A:
        this->m_opcode = Js::OpCode::BrUnLe_A;
        break;

    case Js::OpCode::BrUnLe_A:
        this->m_opcode = Js::OpCode::BrUnGt_A;
        break;

    case Js::OpCode::BrUnLt_A:
        this->m_opcode = Js::OpCode::BrUnGe_A;
        break;
    case Js::OpCode::BrUnGe_I4:
        this->m_opcode = Js::OpCode::BrUnLt_I4;
        break;

    case Js::OpCode::BrUnGt_I4:
        this->m_opcode = Js::OpCode::BrUnLe_I4;
        break;

    case Js::OpCode::BrUnLe_I4:
        this->m_opcode = Js::OpCode::BrUnGt_I4;
        break;

    case Js::OpCode::BrUnLt_I4:
        this->m_opcode = Js::OpCode::BrUnGe_I4;
        break;
    case Js::OpCode::BrOnEmpty:
        this->m_opcode = Js::OpCode::BrOnNotEmpty;
        break;

    case Js::OpCode::BrOnNotEmpty:
        this->m_opcode = Js::OpCode::BrOnEmpty;
        break;
    case Js::OpCode::BrHasSideEffects:
        this->m_opcode = Js::OpCode::BrNotHasSideEffects;
        break;
    case Js::OpCode::BrFncEqApply:
        this->m_opcode = Js::OpCode::BrFncNeqApply;
        break;
    case Js::OpCode::BrFncNeqApply:
        this->m_opcode = Js::OpCode::BrFncEqApply;
        break;
    case Js::OpCode::BrNotHasSideEffects:
        this->m_opcode = Js::OpCode::BrHasSideEffects;
        break;
    case Js::OpCode::BrNotAddr_A:
        this->m_opcode = Js::OpCode::BrAddr_A;
        break;
    case Js::OpCode::BrAddr_A:
        this->m_opcode = Js::OpCode::BrNotAddr_A;
        break;
    case Js::OpCode::BrFncCachedScopeEq:
        this->m_opcode = Js::OpCode::BrFncCachedScopeNeq;
        break;
    case Js::OpCode::BrFncCachedScopeNeq:
        this->m_opcode = Js::OpCode::BrFncCachedScopeEq;
        break;
    case Js::OpCode::BrOnException:
        this->m_opcode = Js::OpCode::BrOnNoException;
        break;

    default:
        AssertMsg(UNREACHED, "Unhandled branch");
    }
}

bool
BranchInstr::IsLoopTail(Func * func)
{
    Assert(func->isPostLower);
    IR::LabelInstr * target = this->GetTarget();
    if (!target->m_isLoopTop)
    {
        return false;
    }

    IR::BranchInstr * lastBranchInstr = nullptr;
    uint32 lastBranchNum = 0;
    FOREACH_SLISTCOUNTED_ENTRY(IR::BranchInstr *, ref, &target->labelRefs)
    {
        if (ref->GetNumber() > lastBranchNum)
        {
            lastBranchInstr = ref;
            lastBranchNum = lastBranchInstr->GetNumber();
        }
    }
    NEXT_SLISTCOUNTED_ENTRY;

    if (this == lastBranchInstr)
    {
        return true;
    }
    return false;
}

///----------------------------------------------------------------------------
///
/// PragmaInstr::New
///
///     Create a PragmaInstr.
///
///----------------------------------------------------------------------------

PragmaInstr *
PragmaInstr::New(Js::OpCode opcode, uint32 index, Func *func)
{
    PragmaInstr * pragmaInstr;

    pragmaInstr = JitAnew(func->m_alloc, IR::PragmaInstr);
    pragmaInstr->Init(opcode, InstrKindPragma, func);
    pragmaInstr->m_statementIndex = index;

    return pragmaInstr;
}

///----------------------------------------------------------------------------
///
/// PragmaInstr::Instr
///
///     Record the information encoded in the pragma
///
///----------------------------------------------------------------------------

#if DBG_DUMP | defined(VTUNE_PROFILING)
void
PragmaInstr::Record(uint32 nativeBufferOffset)
{
    // Currently the only pragma instructions are for Source Info
    Assert(this->m_func->GetTopFunc()->DoRecordNativeMap());
    if (!m_func->IsOOPJIT())
    {
        m_func->GetTopFunc()->GetInProcJITEntryPointInfo()->RecordNativeMap(nativeBufferOffset, m_statementIndex);
    }
}
#endif

///----------------------------------------------------------------------------
///
/// Instr::New
///
///     Create an Instr.
///
///----------------------------------------------------------------------------

Instr *
Instr::New(Js::OpCode opcode, Func *func)
{
    Instr * instr;

    instr = JitAnew(func->m_alloc, IR::Instr);
    instr->Init(opcode, InstrKindInstr, func);
    return instr;
}

///----------------------------------------------------------------------------
///
/// Instr::New
///
///     Create an Instr with a byte code offset.
///
///----------------------------------------------------------------------------

Instr *
Instr::New(Js::OpCode opcode, Func *func, IR::Instr * bytecodeOffsetInstr)
{
    Instr * instr = Instr::New(opcode, func);
    instr->SetByteCodeOffset(bytecodeOffsetInstr);
    return instr;
}

///----------------------------------------------------------------------------
///
/// Instr::New
///
///     Create an Instr with dst.
///
///----------------------------------------------------------------------------

Instr *
Instr::New(Js::OpCode opcode, Opnd *dstOpnd, Func *func)
{
    Instr * instr;

    instr = Instr::New(opcode, func);
    if (dstOpnd)
    {
        instr->SetDst(dstOpnd);
    }
    return instr;
}

///----------------------------------------------------------------------------
///
/// Instr::New
///
///     Create an Instr with dst and a src.
///
///----------------------------------------------------------------------------

Instr *
Instr::New(Js::OpCode opcode, Opnd *dstOpnd, Opnd *src1Opnd, Func *func)
{
    Instr * instr;

    instr = Instr::New(opcode, dstOpnd, func);
    instr->SetSrc1(src1Opnd);

    return instr;
}

///----------------------------------------------------------------------------
///
/// Instr::New
///
///     Create an Instr with dst and 2 srcs.
///
///----------------------------------------------------------------------------

Instr *
Instr::New(Js::OpCode opcode, Opnd *dstOpnd, Opnd *src1Opnd, Opnd *src2Opnd, Func *func)
{
    Instr * instr;

    instr = Instr::New(opcode, dstOpnd, src1Opnd, func);
    instr->SetSrc2(src2Opnd);

    return instr;
}

///----------------------------------------------------------------------------
///
/// Instr::SetDst
///
///     Set the dst for 'this' instruction.  Automatically maintain isSingleDef
///     and instrDef of stackSyms.
///
///----------------------------------------------------------------------------

Opnd *
Instr::SetDst(Opnd * newDst)
{
    AssertMsg(newDst != nullptr, "Calling SetDst with a NULL dst");
    AssertMsg(this->m_dst == nullptr, "Calling SetDst without unlinking/freeing the current dst");
    Assert(!(newDst->IsRegOpnd() && newDst->AsRegOpnd()->IsSymValueFrozen()));

    newDst = newDst->Use(m_func);
    this->m_dst = newDst;

    // If newDst isSingleDef, set instrDef

    StackSym *stackSym;

    if (newDst->IsRegOpnd() && newDst->AsRegOpnd()->m_sym)
    {
        stackSym = newDst->AsRegOpnd()->m_sym->AsStackSym();
    }
    else if (newDst->IsSymOpnd() && newDst->AsSymOpnd()->m_sym->IsStackSym())
    {
        stackSym = newDst->AsSymOpnd()->m_sym->AsStackSym();
    }
    else
    {
        stackSym = nullptr;
    }

    if (stackSym && stackSym->m_isSingleDef)
    {
        if (stackSym->m_instrDef)
        {
            AssertMsg(!stackSym->IsArgSlotSym(), "Arg Slot sym needs to be single def to maintain the StartCall arg links");

            // Multiple defs, clear isSingleDef flag
            stackSym->m_isSingleDef = false;
            stackSym->m_instrDef    = nullptr;
            stackSym->m_isConst     = false;
            stackSym->m_isIntConst  = false;
            stackSym->m_isInt64Const= false;
            stackSym->m_isTaggableIntConst  = false;
            stackSym->m_isNotNumber    = false;
            stackSym->m_isStrConst  = false;
            stackSym->m_isStrEmpty  = false;
            stackSym->m_isFltConst  = false;
        }
        else
        {
            stackSym->m_instrDef = this;
        }
    }

    return newDst;
}

Opnd *
Instr::SetFakeDst(Opnd * newDst)
{
    AssertMsg(newDst != nullptr, "Calling SetDst with a NULL dst");
    AssertMsg(this->m_dst == nullptr, "Calling SetDst without unlinking/freeing the current dst");
    Assert(!(newDst->IsRegOpnd() && newDst->AsRegOpnd()->IsSymValueFrozen()));

    newDst = newDst->Use(m_func);
    this->m_dst = newDst;

#if DBG
    newDst->isFakeDst = true;
#endif
    return newDst;
}

///----------------------------------------------------------------------------
///
/// Instr::UnlinkDst
///
///     Unlinks the dst for 'this' instruction.  Automatically maintains
///     instrDef of stackSyms.
///
///----------------------------------------------------------------------------

Opnd *
Instr::UnlinkDst()
{
    Opnd * oldDst = this->m_dst;
    StackSym *stackSym = nullptr;

    // If oldDst isSingleDef, clear instrDef

    if (oldDst->IsRegOpnd())
    {
        stackSym = oldDst->AsRegOpnd()->m_sym;
    }
    else if (oldDst->IsSymOpnd())
    {
        Sym *sym = oldDst->AsSymOpnd()->m_sym;
        if (sym->IsStackSym())
        {
            stackSym = sym->AsStackSym();
        }
    }

    if (stackSym && stackSym->m_isSingleDef)
    {
        if (stackSym->m_instrDef == this)
        {
            stackSym->m_instrDef = nullptr;
        }
        else
        {
            Assert(oldDst->isFakeDst);
        }
    }
#if DBG
    if (oldDst->isFakeDst)
    {
        oldDst->isFakeDst = false;
    }
#endif

    oldDst->UnUse();
    this->m_dst = nullptr;

    return oldDst;
}

///----------------------------------------------------------------------------
///
/// Instr::FreeDst
///
///     Unlinks and free the dst for 'this' instruction.
///
///----------------------------------------------------------------------------

void
Instr::FreeDst()
{
    Opnd * unlinkedDst;
    unlinkedDst = this->UnlinkDst();
    unlinkedDst->Free(this->m_func);
}

///----------------------------------------------------------------------------
///
/// Instr::ReplaceDst
///
///     Unlink this dst from this instr, free it, and replace it with newDst.
///     The new dst is returned.
///
///----------------------------------------------------------------------------

Opnd *
Instr::ReplaceDst(Opnd * newDst)
{
    this->FreeDst();
    return this->SetDst(newDst);
}

///----------------------------------------------------------------------------
///
/// Instr::SinkDst
///
///     Replace current dst with new symbol, and assign new symbol using the
///     given opcode to the previous dst.
///
///----------------------------------------------------------------------------

Instr *
Instr::SinkDst(Js::OpCode assignOpcode, RegNum regNum, IR::Instr *insertAfterInstr)
{
    return SinkDst(assignOpcode, StackSym::New(this->GetDst()->GetType(), m_func), regNum, insertAfterInstr);
}

Instr *
Instr::SinkDst(Js::OpCode assignOpcode, StackSym * stackSym, RegNum regNum, IR::Instr *insertAfterInstr)
{
    if(!insertAfterInstr)
    {
        insertAfterInstr = this;
    }

    Opnd *oldDst, *newDst;
    Instr * newInstr;
    IRType type;

    oldDst = this->UnlinkDst();
    type = oldDst->GetType();
    newDst = this->SetDst(RegOpnd::New(stackSym, regNum, type, m_func));
    newInstr = Instr::New(assignOpcode, oldDst, newDst, m_func);
    insertAfterInstr->InsertAfter(newInstr);

    return newInstr;
}

IR::Instr *
Instr::SinkInstrBefore(IR::Instr * instrTarget)
{
    // Move this instruction down to the target location, preserving
    // the use(s), if necessary, from redefinition between the original
    // location and the new one.

    if (this->m_next == instrTarget)
    {
        return this->m_prev;
    }

    StackSym *sym;
    if (this->m_src1)
    {
        sym = this->m_src1->GetStackSym();
        if (sym && !sym->m_isSingleDef)
        {
            this->HoistSrc1(Js::OpCode::Ld_A);
        }

        if (this->m_src2)
        {
            sym = this->m_src2->GetStackSym();
            if (sym && !sym->m_isSingleDef)
            {
                this->HoistSrc2(Js::OpCode::Ld_A);
            }
        }
    }

    // Move the instruction down to the target. Return the instruction
    // that preceded the sunk instruction at its original location.
    // (This lets the caller find a Ld_A that this call inserted.)
    IR::Instr * instrPrev = this->m_prev;
    this->Unlink();
    instrTarget->InsertBefore(this);
    return instrPrev;
}

///----------------------------------------------------------------------------
///
/// Instr::UnlinkSrc1
///
///     Unlinks the src1 for 'this' instruction.
///
///----------------------------------------------------------------------------

Opnd *
Instr::UnlinkSrc1()
{
    Opnd * oldSrc = this->m_src1;
    oldSrc->UnUse();
    this->m_src1 = nullptr;

    return oldSrc;
}

///----------------------------------------------------------------------------
///
/// Instr::FreeSrc1
///
///     Unlinks and free the src1 for 'this' instruction.
///
///----------------------------------------------------------------------------

void
Instr::FreeSrc1()
{
    Opnd * unlinkedSrc;
    unlinkedSrc = this->UnlinkSrc1();
    unlinkedSrc->Free(this->m_func);
}

///----------------------------------------------------------------------------
///
/// Instr::ReplaceSrc1
///
///     Unlink src1 from this instr, free it, and replace it with newSrc.
///     The new src is returned.
///
///----------------------------------------------------------------------------

Opnd *
Instr::ReplaceSrc1(Opnd * newSrc)
{
    this->FreeSrc1();
    return this->SetSrc1(newSrc);
}

///----------------------------------------------------------------------------
///
/// Instr::HoistSrc1
///
///     Replace current src with new symbol, and assign new symbol using the
///     given opcode from the previous src.
///
///----------------------------------------------------------------------------

Instr *
Instr::HoistSrc1(Js::OpCode assignOpcode, RegNum regNum, StackSym *newSym)
{
    Opnd *oldSrc, *newSrc;
    Instr * newInstr;
    IRType type;

    oldSrc = this->UnlinkSrc1();
    type = oldSrc->GetType();

    const bool creatingNewSym = !newSym;
    if(creatingNewSym)
    {
        newSym = StackSym::New(type, m_func);
    }
    newSrc = this->SetSrc1(RegOpnd::New(newSym, regNum, type, m_func));
    newSrc->SetValueType(oldSrc->GetValueType());

    newInstr = Instr::New(assignOpcode, newSrc, oldSrc, m_func);
    this->InsertBefore(newInstr);

    if(creatingNewSym)
    {
        if (oldSrc->IsRegOpnd())
        {
            newSym->CopySymAttrs(oldSrc->AsRegOpnd()->m_sym);
        }
        else if (oldSrc->IsImmediateOpnd())
        {
            newSym->SetIsConst();
        }
    }

    return newInstr;
}

///----------------------------------------------------------------------------
///
/// Instr::UnlinkSrc2
///
///     Unlinks the src2 for 'this' instruction.
///
///----------------------------------------------------------------------------

Opnd *
Instr::UnlinkSrc2()
{
    Opnd * oldSrc = this->m_src2;
    oldSrc->UnUse();
    this->m_src2 = nullptr;

    return oldSrc;
}

///----------------------------------------------------------------------------
///
/// Instr::FreeSrc2
///
///     Unlinks and free the src2 for 'this' instruction.
///
///----------------------------------------------------------------------------

void
Instr::FreeSrc2()
{
    Opnd * unlinkedSrc;
    unlinkedSrc = this->UnlinkSrc2();
    unlinkedSrc->Free(this->m_func);
}

///----------------------------------------------------------------------------
///
/// Instr::ReplaceSrc2
///
///     Unlink src2 from this instr, free it, and replace it with newSrc.
///     The new src is returned.
///
///----------------------------------------------------------------------------

Opnd *
Instr::ReplaceSrc2(Opnd * newSrc)
{
    this->FreeSrc2();
    return this->SetSrc2(newSrc);
}

///----------------------------------------------------------------------------
///
/// Instr::HoistSrc2
///
///     Replace current src with new symbol, and assign new symbol using the
///     given opcode from the previous src.
///
///----------------------------------------------------------------------------

Instr *
Instr::HoistSrc2(Js::OpCode assignOpcode, RegNum regNum, StackSym *newSym)
{
    Opnd *oldSrc, *newSrc;
    Instr * newInstr;
    IRType type;

    oldSrc = this->UnlinkSrc2();
    type = oldSrc->GetType();

    const bool creatingNewSym = !newSym;
    if(creatingNewSym)
    {
        newSym = StackSym::New(type, m_func);
    }
    newSrc = this->SetSrc2(RegOpnd::New(newSym, regNum, type, m_func));
    newSrc->SetValueType(oldSrc->GetValueType());

    newInstr = Instr::New(assignOpcode, newSrc, oldSrc, m_func);
    this->InsertBefore(newInstr);

    if(creatingNewSym)
    {
        if (oldSrc->IsRegOpnd())
        {
            newSym->CopySymAttrs(oldSrc->AsRegOpnd()->m_sym);
        }
        else if (oldSrc->IsIntConstOpnd())
        {
            newSym->SetIsIntConst(oldSrc->AsIntConstOpnd()->GetValue());
        }
    }

    return newInstr;
}

IndirOpnd *
Instr::HoistMemRefAddress(MemRefOpnd *const memRefOpnd, const Js::OpCode loadOpCode)
{
    Assert(memRefOpnd);
#if defined(_M_IX86) || defined(_M_X64)
    Assert(!LowererMDArch::IsLegalMemLoc(memRefOpnd));
#endif
    intptr_t address = memRefOpnd->GetMemLoc();
    IR::AddrOpndKind kind = memRefOpnd->GetAddrKind();
    Func *const func = m_func;
    IR::AddrOpnd * addrOpnd = IR::AddrOpnd::New(address, kind, this->m_func, true);
    IR::IndirOpnd * indirOpnd = func->GetTopFunc()->GetConstantAddressIndirOpnd(address, addrOpnd, kind, memRefOpnd->GetType(), loadOpCode);

    if (indirOpnd == nullptr)
    {
        IR::RegOpnd * addressRegOpnd = IR::RegOpnd::New(TyMachPtr, func);
        IR::Instr *const newInstr =
            IR::Instr::New(
            loadOpCode,
            addressRegOpnd,
            IR::AddrOpnd::New(address, kind, func, true),
            func);
        InsertBefore(newInstr);

        indirOpnd = IR::IndirOpnd::New(addressRegOpnd, 0, memRefOpnd->GetType(), func, true);
#if DBG_DUMP
        // TODO: michhol oop jit, make intptr
        indirOpnd->SetAddrKind(kind, (void*)address);
#endif
    }
    return DeepReplace(memRefOpnd, indirOpnd)->AsIndirOpnd();
}


Opnd *
Instr::Replace(Opnd *oldOpnd, Opnd *newOpnd)
{
    if (oldOpnd == this->GetDst())
    {
        return this->ReplaceDst(newOpnd);
    }
    else
    {
        return this->ReplaceSrc(oldOpnd, newOpnd);
    }
}

Opnd *Instr::DeepReplace(Opnd *const oldOpnd, Opnd *const newOpnd)
{
    Assert(oldOpnd);
    Assert(newOpnd);

    IR::Opnd *opnd = GetDst();
    if(opnd && oldOpnd != opnd && oldOpnd->IsEqual(opnd))
    {
        ReplaceDst(newOpnd);
    }
    opnd = GetSrc1();
    if(opnd && oldOpnd != opnd && oldOpnd->IsEqual(opnd))
    {
        ReplaceSrc1(newOpnd);
    }
    opnd = GetSrc2();
    if(opnd && oldOpnd != opnd && oldOpnd->IsEqual(opnd))
    {
        ReplaceSrc2(newOpnd);
    }

    // Do this last because Replace will delete oldOpnd
    return Replace(oldOpnd, newOpnd);
}

Opnd *
Instr::UnlinkSrc(Opnd *src)
{
    if (src == this->GetSrc1())
    {
        return this->UnlinkSrc1();
    }
    else
    {
        AssertMsg(src == this->GetSrc2(), "Src not found");

        return this->UnlinkSrc2();
    }
}

///----------------------------------------------------------------------------
///
/// Instr::ReplaceSrc
///
///     Unlink oldSrc from this instr, free it, and replace it with newSrc.
///     The new src is returned.
///
///----------------------------------------------------------------------------

Opnd *
Instr::ReplaceSrc(Opnd *oldSrc, Opnd * newSrc)
{
    if (oldSrc == this->GetSrc1())
    {
        return this->ReplaceSrc1(newSrc);
    }
    else
    {
        AssertMsg(oldSrc == this->GetSrc2(), "OldSrc not found");

        return this->ReplaceSrc2(newSrc);
    }
}

///----------------------------------------------------------------------------
///
/// Instr::IsRealInstr
///
///     Does this instr generate code?
///
///----------------------------------------------------------------------------

bool
Instr::IsRealInstr() const
{
    switch (m_opcode)
    {
    case Js::OpCode::Label:
    case Js::OpCode::StatementBoundary:
    case Js::OpCode::NoImplicitCallUses:
    case Js::OpCode::NoIntOverflowBoundary:
#if DBG
    case Js::OpCode::CheckLowerIntBound:
    case Js::OpCode::CheckUpperIntBound:
#endif
        return false;

    default:
        return true;
    }
}

///----------------------------------------------------------------------------
///
/// Instr::GetNextRealInstr
///
///----------------------------------------------------------------------------
IR::Instr *
Instr::GetNextRealInstr() const
{
    IR::Instr *instr = this->m_next;

    while (instr != nullptr && !instr->IsRealInstr())
    {
        AssertMsg(instr->m_next || instr->IsPragmaInstr(), "GetNextRealInstr() failed...");
        instr = instr->m_next;
    }
    return instr;
}

#if DBG
IR::LabelInstr *
Instr::GetNextNonEmptyLabel() const
{
    IR::Instr *instr = const_cast<Instr*>(this);

    while (instr != nullptr && (!instr->IsLabelInstr() || instr->m_next->IsLabelInstr()))
    {
        instr = instr->m_next;
    }

    return instr->AsLabelInstr();
}
#endif

///----------------------------------------------------------------------------
///
/// Instr::GetNextRealInstrOrLabel
///
///----------------------------------------------------------------------------
IR::Instr *
Instr::GetNextRealInstrOrLabel() const
{
    IR::Instr *instr = this->m_next;

    while (instr != nullptr && !instr->IsLabelInstr() && !instr->IsRealInstr())
    {
        instr = instr->m_next;
        AssertMsg(instr, "GetNextRealInstrOrLabel() failed...");
    }
    return instr;
}

IR::Instr *
Instr::GetNextBranchOrLabel() const
{
    IR::Instr *instr = this->m_next;

    while (instr != nullptr && !instr->IsLabelInstr() && !instr->IsBranchInstr())
    {
        instr = instr->m_next;
    }
    return instr;
}

IR::Instr *
Instr::GetNextByteCodeInstr() const
{
    IR::Instr * nextInstr = GetNextRealInstrOrLabel();
    uint32 currentOffset = GetByteCodeOffset();
    const auto getNext = [](IR::Instr* nextInstr) -> IR::Instr*
    {
        if (nextInstr->IsBranchInstr())
        {
            IR::BranchInstr* branchInstr = nextInstr->AsBranchInstr();
            AssertMsg(branchInstr->IsUnconditional(), "We can't know which branch to take on a conditionnal branch");
            if (branchInstr->IsUnconditional())
            {
                return branchInstr->GetTarget();
            }
        }
        return nextInstr->GetNextRealInstrOrLabel();
    };
    while (nextInstr->GetByteCodeOffset() == Js::Constants::NoByteCodeOffset ||
        nextInstr->GetByteCodeOffset() == currentOffset)
    {
        nextInstr = getNext(nextInstr);
    }

    // Do not check if the instr trying to bailout is in the function prologue
    // nextInstr->GetByteCodeOffset() < currentOffset would always be true and we would crash
    if (currentOffset != Js::Constants::NoByteCodeOffset)
    {
        // This can happen due to break block removal
        while (nextInstr->GetByteCodeOffset() == Js::Constants::NoByteCodeOffset ||
            nextInstr->GetByteCodeOffset() < currentOffset)
        {
            nextInstr = getNext(nextInstr);
        }
    }
    else
    {
        AssertMsg(nextInstr->GetByteCodeOffset() == 0, "Only instrs before the first one are allowed to not have a bytecode offset");
    }
    return nextInstr;
}

///----------------------------------------------------------------------------
///
/// Instr::GetPrevRealInstr
///
///----------------------------------------------------------------------------
IR::Instr *
Instr::GetPrevRealInstr() const
{
    IR::Instr *instr = this->m_prev;

    while (!instr->IsRealInstr())
    {
        instr = instr->m_prev;
        AssertMsg(instr, "GetPrevRealInstr() failed...");
    }
    return instr;
}

///----------------------------------------------------------------------------
///
/// Instr::GetPrevRealInstrOrLabel
///
///----------------------------------------------------------------------------
IR::Instr *
Instr::GetPrevRealInstrOrLabel() const
{
    IR::Instr *instr = this->m_prev;

    while (!instr->IsLabelInstr() && !instr->IsRealInstr())
    {
        instr = instr->m_prev;
        AssertMsg(instr, "GetPrevRealInstrOrLabel() failed...");
    }
    return instr;
}

///----------------------------------------------------------------------------
///
/// Instr::GetPrevLabelInstr
///
///----------------------------------------------------------------------------
IR::LabelInstr *
Instr::GetPrevLabelInstr() const
{
    IR::Instr *instr = this->m_prev;

    while (!instr->IsLabelInstr())
    {
        instr = instr->m_prev;
        AssertMsg(instr, "GetPrevLabelInstr() failed...");
    }
    return instr->AsLabelInstr();
}

///----------------------------------------------------------------------------
///
/// Instr::GetPrevLabelInstr
///
///----------------------------------------------------------------------------
IR::Instr *
Instr::GetBlockStartInstr() const
{
    IR::Instr *instr = this->m_prev;

    while (!instr->StartsBasicBlock())
    {
        instr = instr->m_prev;
        AssertMsg(instr, "GetBlockStartInstr() failed...");
    }
    return instr;
}

///----------------------------------------------------------------------------
///
/// Instr::GetInsertBeforeByteCodeUsesInstr
/// Finds the instruction before which new instructions can be inserted, by skipping ByteCodeUses instructions associated with
/// this instruction.
///
///----------------------------------------------------------------------------
IR::Instr *Instr::GetInsertBeforeByteCodeUsesInstr()
{
    const uint32 byteCodeOffset = GetByteCodeOffset();
    IR::Instr *insertBeforeInstr = this;
    IR::Instr *prevInstr = insertBeforeInstr->m_prev;
    while(prevInstr && prevInstr->IsByteCodeUsesInstr() && prevInstr->GetByteCodeOffset() == byteCodeOffset)
    {
        insertBeforeInstr = prevInstr;
        prevInstr = prevInstr->m_prev;
    }
    return insertBeforeInstr;
}

bool
Instr::IsByteCodeUsesInstrFor(IR::Instr * instr) const
{
    return this->IsByteCodeUsesInstr() && this->GetByteCodeOffset() == instr->GetByteCodeOffset();
}

///----------------------------------------------------------------------------
///
/// Instr::GetOrCreateContinueLabel
///
///----------------------------------------------------------------------------
IR::LabelInstr *
Instr::GetOrCreateContinueLabel(const bool isHelper)
{
    if (m_next && m_next->IsLabelInstr() && m_next->AsLabelInstr()->isOpHelper == isHelper)
    {
        return m_next->AsLabelInstr();
    }

    IR::LabelInstr *const label = IR::LabelInstr::New(Js::OpCode::Label, m_func, isHelper);
    InsertAfter(label);
    return label;
}

bool
Instr::HasSymUseSrc(StackSym *sym, IR::Opnd* src)
{
    if (!src)
    {
        return false;
    }
    if (src->IsRegOpnd())
    {
        RegOpnd *regOpnd = src->AsRegOpnd();

        if (regOpnd->m_sym == sym)
        {
            return true;
        }
    }
    else if (src->IsIndirOpnd())
    {
        IR::IndirOpnd *indirOpnd = src->AsIndirOpnd();
        RegOpnd * baseOpnd = indirOpnd->GetBaseOpnd();
        if (baseOpnd != nullptr && baseOpnd->m_sym == sym)
        {
            return true;
        }
        else if (indirOpnd->GetIndexOpnd() && indirOpnd->GetIndexOpnd()->m_sym == sym)
        {
            return true;
        }
    }
    else if (src->IsListOpnd())
    {
        IR::ListOpnd* list = src->AsListOpnd();
        for (int i = 0; i < list->Count(); ++i)
        {
            if (HasSymUseSrc(sym, list->Item(i)))
            {
                return true;
            }
        }
    }
    else if (src->IsSymOpnd())
    {
        SymOpnd* symOpnd = src->AsSymOpnd();
        if (symOpnd->GetSym() == sym)
        {
            return true;
        }
        if (symOpnd->IsPropertySymOpnd())
        {
            PropertySymOpnd* propertySymOpnd = symOpnd->AsPropertySymOpnd();
            if (propertySymOpnd->GetObjectSym() == sym)
            {
                return true;
            }
        }
    }
    return false;
}


bool
Instr::HasSymUseDst(StackSym *sym, IR::Opnd* dst)
{
    if (!dst)
    {
        return false;
    }
    if (dst->IsIndirOpnd())
    {
        IR::IndirOpnd *indirOpnd = dst->AsIndirOpnd();
        RegOpnd * baseOpnd = indirOpnd->GetBaseOpnd();
        if (baseOpnd != nullptr && baseOpnd->m_sym == sym)
        {
            return true;
        }
        else if (indirOpnd->GetIndexOpnd() && indirOpnd->GetIndexOpnd()->m_sym == sym)
        {
            return true;
        }
    }
    else if (dst->IsListOpnd())
    {
        IR::ListOpnd* list = dst->AsListOpnd();
        for (int i = 0; i < list->Count(); ++i)
        {
            if (HasSymUseDst(sym, list->Item(i)))
            {
                return true;
            }
        }
    }
    else if (dst->IsSymOpnd())
    {
        SymOpnd* symOpnd = dst->AsSymOpnd();
        if (symOpnd->GetSym() == sym)
        {
            return true;
        }
        if (symOpnd->IsPropertySymOpnd())
        {
            PropertySymOpnd* propertySymOpnd = symOpnd->AsPropertySymOpnd();
            if (propertySymOpnd->GetObjectSym() == sym)
            {
                return true;
            }
        }
    }
    return false;
}

bool
Instr::HasSymUse(StackSym *sym)
{
    if (HasSymUseSrc(sym, this->GetSrc1()))
    {
        return true;
    }
    if (HasSymUseSrc(sym, this->GetSrc2()))
    {
        return true;
    }
    if (HasSymUseDst(sym, this->GetDst()))
    {
        return true;
    }
    return false;
}

bool
Instr::HasSymUseInRange(StackSym *sym, IR::Instr *instrBegin, IR::Instr *instrEnd)
{
    FOREACH_INSTR_IN_RANGE(instr, instrBegin, instrEnd)
    {
        Assert(instr);
        if (instr->HasSymUse(sym))
        {
            return true;
        }
    }
    NEXT_INSTR_IN_RANGE;

    return false;
}

///----------------------------------------------------------------------------
///
/// Instr::FindRegDef
///
///     Search a reg def of the given sym.  Return the RegOpnd that defines it.
///
///----------------------------------------------------------------------------

IR::RegOpnd *
Instr::FindRegDef(StackSym *sym)
{
    IR::Opnd *dst = this->GetDst();

    if (dst)
    {
        if (dst->IsRegOpnd())
        {
            RegOpnd *regOpnd = dst->AsRegOpnd();

            if (regOpnd->m_sym == sym)
            {
                return regOpnd;
            }
        }
    }

    return nullptr;
}

Instr*
Instr::FindSingleDefInstr(Js::OpCode opCode, Opnd* src)
{
    RegOpnd* src1 = src->IsRegOpnd() ? src->AsRegOpnd() : nullptr;

    return  src1 &&
        src1->m_sym->IsSingleDef() &&
        src1->m_sym->GetInstrDef()->m_opcode == opCode ?
        src1->m_sym->GetInstrDef() :
        nullptr;
}

void
Instr::TransferDstAttributesTo(Instr * instr)
{
    instr->dstIsTempNumber = this->dstIsTempNumber;
    instr->dstIsTempNumberTransferred = this->dstIsTempNumberTransferred;
    instr->dstIsTempObject = this->dstIsTempObject;
}

void
Instr::TransferTo(Instr * instr)
{
    Assert(instr->m_dst == nullptr);
    Assert(instr->m_src1 == nullptr);
    Assert(instr->m_src2 == nullptr);
    this->TransferDstAttributesTo(instr);
    instr->usesStackArgumentsObject = this->usesStackArgumentsObject;
    instr->isCloned = this->isCloned;
    instr->ignoreNegativeZero = this->ignoreNegativeZero;
    instr->ignoreIntOverflow = this->ignoreIntOverflow;
    instr->ignoreIntOverflowInRange = this->ignoreIntOverflowInRange;
    instr->ignoreOverflowBitCount = this->ignoreOverflowBitCount;
    instr->loadedArrayHeadSegment = this->loadedArrayHeadSegment;
    instr->loadedArrayHeadSegmentLength = this->loadedArrayHeadSegmentLength;
    instr->extractedUpperBoundCheckWithoutHoisting = this->extractedUpperBoundCheckWithoutHoisting;
    instr->m_number = this->m_number;
    instr->m_src1 = this->m_src1;
    instr->m_src2 = this->m_src2;
    instr->dstIsAlwaysConvertedToInt32 = this->dstIsAlwaysConvertedToInt32;
    instr->dstIsAlwaysConvertedToNumber = this->dstIsAlwaysConvertedToNumber;
    instr->dataWidth = this->dataWidth;
    instr->isCtorCall = this->isCtorCall;
    instr->forcePreOpBailOutIfNeeded = this->forcePreOpBailOutIfNeeded;
    IR::Opnd * dst = this->m_dst;

    if (dst)
    {
        instr->m_dst = dst;
        this->m_dst = nullptr;
        if (dst->IsRegOpnd())
        {
            Sym * sym = dst->AsRegOpnd()->m_sym;
            if (sym->IsStackSym() && sym->AsStackSym()->m_isSingleDef)
            {
                Assert(sym->AsStackSym()->m_instrDef == this);
                StackSym * stackSym = sym->AsStackSym();
                stackSym->m_instrDef = instr;
            }
        }
    }

    this->m_src1 = nullptr;
    this->m_src2 = nullptr;
}

// Convert an instruction to a bailout instruction and perform a shallow copy of the input instruction's BailOutInfo.
// Can optionally change the BailOutKind, otherwise the input instruction's BailOutKind will be used instead.
IR::Instr *
Instr::ConvertToBailOutInstrWithBailOutInfoCopy(BailOutInfo *bailOutInfo, IR::BailOutKind bailOutKind)
{
    BailOutInfo *bailOutInfoCopy = JitAnew(this->m_func->m_alloc, BailOutInfo, bailOutInfo->bailOutOffset, this->m_func);
    bailOutInfo->PartialDeepCopyTo(bailOutInfoCopy);
    return this->ConvertToBailOutInstr(bailOutInfoCopy, bailOutKind);
}

IR::Instr *
Instr::ConvertToBailOutInstr(IR::Instr * bailOutTarget, IR::BailOutKind kind, uint32 bailOutOffset)
{
    Func * func = bailOutTarget->m_func;
    BailOutInfo * bailOutInfo = JitAnew(func->m_alloc, BailOutInfo, bailOutOffset == Js::Constants::NoByteCodeOffset ? bailOutTarget->GetByteCodeOffset() : bailOutOffset , func);
#if ENABLE_DEBUG_CONFIG_OPTIONS
    bailOutInfo->bailOutOpcode = this->m_opcode;
#endif
    return this->ConvertToBailOutInstr(bailOutInfo, kind);
}

// Notes:
// - useAuxBailout = true specifies that this bailout further will be invisible to globopt, etc, and we'll use auxBailoutKind instead of BailoutKind.
//   Currently this is used for BailOutIgnoreException for debugger.
//
//   Here's typical workflow for scenario useAuxBailout = true.
//   - IRBuilder::Build calls this with kind == BailOutIgnoreException
//   - In here we save the kind to auxBailOut and save bail out info but set hasBailOutInfo to false.
//   - During globopt optimizations presence of this bail out is not detected and instrs can add/remove bailouts as they need.
//     - If they call to convert this instr to bail out instr, we set bailOutKind to what they want and replace bailOutInfo.
//       ** This assumes that for aux bail out bailoutInfo does not really matter (if its pre/post op, etc) **
//       - This is the case for ignore exception.
//       - This will cause to share aux bail out with regular bail out.
//   - In globopt right after OptInstr we check if there is aux bail out which wasn't shared with regular bail out,
//     and if it's not, we convert it back to regular bail out.
IR::Instr *
Instr::ConvertToBailOutInstr(BailOutInfo * bailOutInfo, IR::BailOutKind kind, bool useAuxBailOut /* = false */)
{
    Assert(!this->HasBailOutInfo());

    AssertMsg(!useAuxBailOut || !this->HasAuxBailOut(), "Already aux bail out!");
    Assert(!this->HasAuxBailOut() || this->GetAuxBailOutKind() != IR::BailOutInvalid);

    IR::Instr * bailOutInstr = nullptr;
    if (this->HasAuxBailOut())
    {
        // This instr has already been converted to bailout instr. Only possible with aux bail out.
        // Typical scenario is when globopt calls to convert to e.g. BailOutOnImplicitCalls for the instr which
        // was already converted to bail out instr with HasBailOutInfo() == false and HasAuxBailOutInfo() == true,
        // so that aux bail out is hidden in between IRBuilder and lowerer.

        AssertMsg((this->GetAuxBailOutKind() & ~(IR::BailOutIgnoreException | IR::BailOutForceByFlag)) == 0, "Only IR::BailOutIgnoreException|ForceByFlag supported here.");
        // What we rely on here is:
        // - bailout doesn't have any args.
        // - bailout doesn't use offset as we get it from DebuggingFlags at time of bailout.

        // Use prev debugger bailout kind as decoration, while keeping new kind as main.
        this->SetBailOutKind_NoAssert(kind);

        // Clear old (aux) info and set to the new bailOutInfo.
        this->ReplaceBailOutInfo(bailOutInfo);
        bailOutInfo->bailOutInstr = this;
        this->hasBailOutInfo = true;

        bailOutInstr = this;
    }
    else
    {
        switch (this->m_kind)
        {
        case InstrKindInstr:
            bailOutInstr = IR::BailOutInstr::New(this->m_opcode, kind, bailOutInfo, bailOutInfo->bailOutFunc);
            break;
        case InstrKindProfiled:
            bailOutInstr = IR::ProfiledBailOutInstr::New(this->m_opcode, kind, bailOutInfo, bailOutInfo->bailOutFunc);
            bailOutInstr->AsProfiledInstr()->u = this->AsProfiledInstr()->u;
            break;
        case InstrKindBranch:
        {
            IR::BranchInstr * branchInstr = this->AsBranchInstr();
            Assert(!branchInstr->IsMultiBranch());
            IR::BranchBailOutInstr * branchBailOutInstr = IR::BranchBailOutInstr::New(this->m_opcode, kind, bailOutInfo, bailOutInfo->bailOutFunc);
            branchBailOutInstr->SetTarget(branchInstr->GetTarget());
            branchBailOutInstr->SetByteCodeReg(branchInstr->GetByteCodeReg());
            bailOutInstr = branchBailOutInstr;
            break;
        }
        default:
            AnalysisAssert(false);
        };

        this->m_next->m_prev = bailOutInstr;
        this->m_prev->m_next = bailOutInstr;
        bailOutInstr->m_next = this->m_next;
        bailOutInstr->m_prev = this->m_prev;

        this->TransferTo(bailOutInstr);

        this->Free();
    }

    if (useAuxBailOut)
    {
        // Move bail out kind from bailOutKind to auxBailOutKind and hide bailOutInfo as if this is not a bail out instr.
        bailOutInstr->SetAuxBailOutKind(kind);
        bailOutInstr->SetBailOutKind_NoAssert(IR::BailOutInvalid);
        bailOutInstr->hasBailOutInfo = false;
        bailOutInstr->hasAuxBailOut = true;
    }

    return bailOutInstr;
}

// Convert aux bailout to regular bail out.
// Called by globopt after all optimizations are done, in case we still have aux bail out on the instr.
void Instr::PromoteAuxBailOut()
{
    Assert(!this->HasBailOutInfo());
    Assert(this->GetAuxBailOutKind() != IR::BailOutInvalid);

    this->SetBailOutKind_NoAssert(this->GetAuxBailOutKind());
    this->SetAuxBailOutKind(IR::BailOutInvalid);

    this->hasBailOutInfo = true;
    this->hasAuxBailOut = false;
}

// Reset all tracks of aux bailout but don't rest the bail out info.
// Used after we extract aux bail out in lowerer.
void Instr::ResetAuxBailOut()
{
    this->SetAuxBailOutKind(IR::BailOutInvalid);
    this->hasAuxBailOut = false;
}

void
Instr::ClearBailOutInfo()
{
    if (this->HasBailOutInfo() || this->HasAuxBailOut())
    {
        BailOutInfo * bailOutInfo = this->GetBailOutInfo();
        Assert(bailOutInfo);

        if (bailOutInfo->bailOutInstr == this)
        {
            JitArenaAllocator * alloc = this->m_func->m_alloc;
            bailOutInfo->Clear(alloc);
            JitAdelete(alloc, bailOutInfo);
        }

        this->hasBailOutInfo = false;
        this->hasAuxBailOut = false;
    }
}

bool Instr::HasAnyLoadHeapArgsOpCode()
{
    switch (m_opcode)
    {
        case Js::OpCode::LdHeapArguments:
        case Js::OpCode::LdHeapArgsCached:
        case Js::OpCode::LdLetHeapArguments:
        case Js::OpCode::LdLetHeapArgsCached:
            return true;
    }
    return false;
}

bool Instr::CanHaveArgOutChain() const
{
    return
        this->m_opcode == Js::OpCode::CallI ||
        this->m_opcode == Js::OpCode::CallIFixed ||
        this->m_opcode == Js::OpCode::NewScObject ||
        this->m_opcode == Js::OpCode::NewScObjectSpread ||
        this->m_opcode == Js::OpCode::NewScObjArray ||
        this->m_opcode == Js::OpCode::NewScObjArraySpread;
}

bool Instr::HasEmptyArgOutChain(IR::Instr** startCallInstrOut)
{
    Assert(CanHaveArgOutChain());

    if (GetSrc2()->IsRegOpnd())
    {
        IR::RegOpnd * argLinkOpnd = GetSrc2()->AsRegOpnd();
        StackSym *argLinkSym = argLinkOpnd->m_sym->AsStackSym();
        AssertMsg(!argLinkSym->IsArgSlotSym() && argLinkSym->m_isSingleDef, "Arg tree not single def...");
        IR::Instr* startCallInstr = argLinkSym->m_instrDef;
        AssertMsg(startCallInstr->m_opcode == Js::OpCode::StartCall, "Problem with arg chain.");
        if (startCallInstrOut != nullptr)
        {
            *startCallInstrOut = startCallInstr;
        }
        return true;
    }

    return false;
}

bool Instr::HasFixedFunctionAddressTarget() const
{
    Assert(
        this->m_opcode == Js::OpCode::CallI ||
        this->m_opcode == Js::OpCode::CallIFixed ||
        this->m_opcode == Js::OpCode::NewScObject ||
        this->m_opcode == Js::OpCode::NewScObjectSpread ||
        this->m_opcode == Js::OpCode::NewScObjArray ||
        this->m_opcode == Js::OpCode::NewScObjArraySpread ||
        this->m_opcode == Js::OpCode::NewScObjectNoCtor);
    return
        this->GetSrc1() != nullptr &&
        this->GetSrc1()->IsAddrOpnd() &&
        this->GetSrc1()->AsAddrOpnd()->GetAddrOpndKind() == IR::AddrOpndKind::AddrOpndKindDynamicVar &&
        this->GetSrc1()->AsAddrOpnd()->m_isFunction;
}

bool Instr::TransfersSrcValue()
{
    // Return whether the instruction transfers a value to the destination.
    // This is used to determine whether we should generate a value for the src so that it will
    // match with the dst for copy prop.

    // No point creating an unknown value for the src of a binary instr, as the dst will just be a different
    // Don't create value for instruction without dst as well. The value doesn't go anywhere.

    // Consider: Add opcode attribute to indicate whether the opcode would use the value or not

    return
        this->GetDst() != nullptr &&

        // The lack of a Src2 does not always indicate that the instr is not a transfer instr (ex: StSlotChkUndecl).
        (this->GetSrc2() == nullptr || OpCodeAttr::NonIntTransfer(this->m_opcode)) &&

        !OpCodeAttr::DoNotTransfer(this->m_opcode) &&
        !this->CallsAccessor();
}


void Instr::MoveArgs(bool generateByteCodeCapture)
{
    Assert(this->m_opcode == Js::OpCode::InlineeStart || this->m_opcode == Js::OpCode::CallDirect ||
        this->m_opcode == Js::OpCode::CallI || this->m_opcode == Js::OpCode::CallIFixed);
    IR::Instr *argInsertInstr = this;
    this->IterateArgInstrs([&](IR::Instr* argInstr)
    {
        if (generateByteCodeCapture)
        {
            argInstr->GenerateBytecodeArgOutCapture();
        }
        argInstr->Move(argInsertInstr);
        argInsertInstr = argInstr;
        return false;
    });
}

void Instr::Move(IR::Instr* insertInstr)
{
    this->Unlink();
    this->ClearByteCodeOffset();
    this->SetByteCodeOffset(insertInstr);
    insertInstr->InsertBefore(this);
}

IR::Instr* Instr::GetBytecodeArgOutCapture()
{
    Assert(this->m_opcode == Js::OpCode::ArgOut_A_Inline ||
        this->m_opcode == Js::OpCode::ArgOut_A ||
        this->m_opcode == Js::OpCode::ArgOut_A_InlineBuiltIn);
    Assert(this->m_dst->GetStackSym()->m_isArgCaptured);
    IR::Instr* instr = this->GetSrc1()->GetStackSym()->m_instrDef;
    while (instr->m_opcode != Js::OpCode::BytecodeArgOutCapture)
    {
        Assert(instr->GetSrc1() && instr->GetSrc1()->GetStackSym() && instr->GetSrc1()->GetStackSym()->IsSingleDef());
        instr = instr->GetSrc1()->GetStackSym()->m_instrDef;
    }
    Assert(instr->m_opcode == Js::OpCode::BytecodeArgOutCapture);
    return instr;
}

bool Instr::HasByteCodeArgOutCapture()
{
    Assert(this->m_opcode == Js::OpCode::ArgOut_A_FixupForStackArgs ||
        this->m_opcode == Js::OpCode::ArgOut_A_Inline ||
        this->m_opcode == Js::OpCode::ArgOut_A ||
        this->m_opcode == Js::OpCode::ArgOut_A_InlineBuiltIn ||
        this->m_opcode == Js::OpCode::ArgOut_A_FromStackArgs);
    if (this->m_dst->GetStackSym()->m_isArgCaptured)
    {
        Assert(GetBytecodeArgOutCapture() != nullptr);
        return true;
    }
    return false;
}


void Instr::GenerateBytecodeArgOutCapture()
{
    if (!HasByteCodeArgOutCapture())
    {
        this->m_dst->GetStackSym()->m_isArgCaptured = true;
        StackSym* tmpSym = StackSym::NewArgSlotRegSym(this->GetDst()->GetStackSym()->GetArgSlotNum(), this->m_func, this->GetDst()->GetType());
        IR::Instr* instr = this->HoistSrc1(Js::OpCode::BytecodeArgOutCapture, RegNOREG, tmpSym);
        instr->SetByteCodeOffset(this);
    }
}

void Instr::GenerateArgOutSnapshot()
{
    StackSym* tmpSym = StackSym::NewArgSlotRegSym(this->GetDst()->GetStackSym()->GetArgSlotNum(), this->m_func);
    IR::Instr* instr = this->HoistSrc1(Js::OpCode::Ld_A, RegNOREG, tmpSym);
    instr->SetByteCodeOffset(this);
}

IR::Instr* Instr::GetArgOutSnapshot()
{
    Assert(this->m_opcode == Js::OpCode::ArgOut_A_FixupForStackArgs);
    IR::Instr* instr = this->GetSrc1()->GetStackSym()->m_instrDef;
    Assert(instr->m_opcode == Js::OpCode::Ld_A);
    return instr;
}

bool Instr::OpndHasAnyImplicitCalls(IR::Opnd* opnd, bool isSrc)
{
    return opnd && (
        (opnd->IsSymOpnd() && opnd->AsSymOpnd()->m_sym->IsPropertySym()) ||
        opnd->IsIndirOpnd() ||
        (isSrc && !opnd->GetValueType().IsPrimitive()) ||
        (opnd->IsListOpnd() && opnd->AsListOpnd()->Any([isSrc](IR::Opnd* lOpnd) { return OpndHasAnyImplicitCalls(lOpnd, isSrc); }))
    );
}

bool Instr::HasAnyImplicitCalls() const
{
    // there can be no implicit calls in asm.js
    if (m_func->GetJITFunctionBody()->IsAsmJsMode())
    {
        return false;
    }
    if (OpCodeAttr::HasImplicitCall(this->m_opcode))
    {
        return true;
    }
    if (OpCodeAttr::OpndHasImplicitCall(this->m_opcode))
    {
        return (
            OpndHasAnyImplicitCalls(this->GetDst(), false) ||
            OpndHasAnyImplicitCalls(this->GetSrc1(), true) ||
            OpndHasAnyImplicitCalls(this->GetSrc2(), true)
        );
    }

    return false;
}

bool Instr::HasAnySideEffects() const
{
    return (hasSideEffects ||
            OpCodeAttr::HasSideEffects(this->m_opcode) ||
            this->HasAnyImplicitCalls());
}

bool Instr::AreAllOpndInt64() const
{
    bool isDstInt64 = !m_dst || IRType_IsInt64(m_dst->GetType());
    bool isSrc1Int64 = !m_src1 || IRType_IsInt64(m_src1->GetType());
    bool isSrc2Int64 = !m_src2 || IRType_IsInt64(m_src2->GetType());
    return isDstInt64 && isSrc1Int64 && isSrc2Int64;
}

FixedFieldInfo* Instr::GetFixedFunction() const
{
    Assert(HasFixedFunctionAddressTarget());
    FixedFieldInfo* function = (FixedFieldInfo*)this->m_src1->AsAddrOpnd()->m_metadata;
    return function;
}

IR::Instr* Instr::GetNextArg()
{
    Assert(this->m_opcode == Js::OpCode::ArgOut_A_FixupForStackArgs ||
        this->m_opcode == Js::OpCode::ArgOut_A_Inline ||
        this->m_opcode == Js::OpCode::ArgOut_A ||
        this->m_opcode == Js::OpCode::ArgOut_A_InlineBuiltIn ||
        this->m_opcode == Js::OpCode::InlineeStart);
    IR::Instr* argInstr = this;
    while (true)
    {
        StackSym* linkSym;
        if (argInstr->GetSrc2()->IsRegOpnd())
        {
            linkSym = argInstr->GetSrc2()->AsRegOpnd()->m_sym->AsStackSym();
        }
        else
        {
            linkSym = argInstr->GetSrc2()->AsSymOpnd()->m_sym->AsStackSym();
            Assert(linkSym->IsArgSlotSym());
        }
        Assert(linkSym->IsSingleDef());
        argInstr = linkSym->m_instrDef;
        if (argInstr->m_opcode == Js::OpCode::ArgOut_A_InlineSpecialized)
        {
            continue;
        }
        if (argInstr->m_opcode == Js::OpCode::StartCall)
        {
            break;
        }
        return argInstr;
    }
    return nullptr;
}

uint Instr::GetArgOutCount(bool getInterpreterArgOutCount)
{
    // There are cases of inlining like .apply and .call target inlining, where we muck around with the ArgOut sequence,
    // and make it different from the one the interpreter sees (and expects, on a bailout).
    // In such cases, we set the interpreter version of the number of ArgOuts as the src2 of StartCall,
    // and any code that queries the argout count for bailout purposes should look at the src2 (if available) of these instructions.

    // If the src2 is not set, that means that the interpreter and the JIT versions of the argout count are the same.

    Js::OpCode opcode = this->m_opcode;
    Assert(opcode == Js::OpCode::StartCall ||
           opcode == Js::OpCode::InlineeEnd || opcode == Js::OpCode::InlineBuiltInEnd|| opcode == Js::OpCode::InlineNonTrackingBuiltInEnd ||
           opcode == Js::OpCode::EndCallForPolymorphicInlinee || opcode == Js::OpCode::LoweredStartCall);

    Assert(!getInterpreterArgOutCount || opcode == Js::OpCode::StartCall);
    uint argOutCount = !this->GetSrc2() || !getInterpreterArgOutCount || m_func->GetJITFunctionBody()->IsAsmJsMode()
        ? this->GetSrc1()->AsIntConstOpnd()->AsUint32()
        : this->GetSrc2()->AsIntConstOpnd()->AsUint32();

    return (uint)argOutCount;
}

uint Instr::GetAsmJsArgOutSize()
{
    switch (m_opcode)
    {
    case Js::OpCode::StartCall:
    case Js::OpCode::LoweredStartCall:
        return GetSrc2()->AsIntConstOpnd()->AsUint32();

    case Js::OpCode::InlineeEnd:
    {
        // StartCall instr has the size, so walk back to it
        IR::Instr *argInstr = this;
        while(argInstr->m_opcode != Js::OpCode::StartCall && argInstr->m_opcode != Js::OpCode::LoweredStartCall)
        {
            argInstr = argInstr->GetSrc2()->GetStackSym()->GetInstrDef();
        }
        // add StartCall arg size with inlinee meta args for full size
        uint size = UInt32Math::Add(argInstr->GetSrc2()->AsIntConstOpnd()->AsUint32(), Js::Constants::InlineeMetaArgCount * MachPtr);
        return size;
    }
    default:
        Assert(UNREACHED);
        return 0;
    }
    }

uint Instr::GetArgOutSize(bool getInterpreterArgOutCount)
{
    Js::OpCode opcode = this->m_opcode;
    Assert(opcode == Js::OpCode::StartCall ||
        opcode == Js::OpCode::InlineeEnd || opcode == Js::OpCode::InlineBuiltInEnd || opcode == Js::OpCode::InlineNonTrackingBuiltInEnd ||
        opcode == Js::OpCode::EndCallForPolymorphicInlinee || opcode == Js::OpCode::LoweredStartCall);

    Assert(!getInterpreterArgOutCount || opcode == Js::OpCode::StartCall);
    if (m_func->GetJITFunctionBody()->IsAsmJsMode())
    {
        return GetAsmJsArgOutSize();
    }
    return UInt32Math::Mul<MachPtr>(GetArgOutCount(getInterpreterArgOutCount));
}

PropertySymOpnd *Instr::GetPropertySymOpnd() const
{
    if (m_src1 && m_src1->IsSymOpnd() && m_src1->AsSymOpnd()->IsPropertySymOpnd())
    {
        return m_src1->AsPropertySymOpnd();
    }
    if (m_dst && m_dst->IsSymOpnd() && m_dst->AsSymOpnd()->IsPropertySymOpnd())
    {
        return m_dst->AsPropertySymOpnd();
    }
    return nullptr;
}

bool Instr::CallsAccessor(IR::PropertySymOpnd * methodOpnd)
{
    if (methodOpnd)
    {
        Assert(methodOpnd->HasObjTypeSpecFldInfo());
        return methodOpnd->UsesAccessor();
    }

    return CallsGetter() || CallsSetter();
}

bool Instr::CallsSetter()
{
    return
        this->IsProfiledInstr() &&
        (this->m_dst && this->m_dst->IsSymOpnd() && this->m_dst->AsSymOpnd()->IsPropertySymOpnd()) &&
        ((this->AsProfiledInstr()->u.FldInfo().flags & Js::FldInfo_FromAccessor) != 0);
}

bool Instr::CallsGetter()
{
    return
        this->IsProfiledInstr() &&
        (this->m_src1 && this->m_src1->IsSymOpnd() && this->m_src1->AsSymOpnd()->IsPropertySymOpnd()) &&
        ((this->AsProfiledInstr()->u.FldInfo().flags & Js::FldInfo_FromAccessor) != 0);
}

IR::Instr* IR::Instr::NewConstantLoad(IR::RegOpnd* dstOpnd, intptr_t varConst, ValueType type, Func* func, Js::Var varLocal/* = nullptr*/)
{
    IR::Opnd *srcOpnd = nullptr;
    IR::Instr *instr;

    if (Js::TaggedInt::Is(varConst))
    {
        IntConstType value = Js::TaggedInt::ToInt32((Js::Var)varConst);
        instr = IR::Instr::New(Js::OpCode::LdC_A_I4, dstOpnd, IR::IntConstOpnd::New(value, TyInt32, func), func);
        if (dstOpnd->m_sym->IsSingleDef())
        {
            dstOpnd->m_sym->SetIsIntConst(value);
        }
    }
    else
    {
        if (varConst == func->GetThreadContextInfo()->GetNullFrameDisplayAddr())
        {
            instr = IR::Instr::New(
                Js::OpCode::Ld_A,
                dstOpnd,
                IR::AddrOpnd::New(
                    func->GetThreadContextInfo()->GetNullFrameDisplayAddr(),
                    IR::AddrOpndKindDynamicMisc,
                    func),
                func);
        }
        else if (varConst == func->GetThreadContextInfo()->GetStrictNullFrameDisplayAddr())
        {
            instr = IR::Instr::New(
                Js::OpCode::Ld_A,
                dstOpnd,
                IR::AddrOpnd::New(
                    func->GetThreadContextInfo()->GetStrictNullFrameDisplayAddr(),
                    IR::AddrOpndKindDynamicMisc,
                    func),
                func);
        }
        else
        {

            ValueType valueType;
            if(type.IsString())
            {
                srcOpnd = IR::AddrOpnd::New(varConst, IR::AddrOpndKindDynamicVar, func, true, varLocal);
                instr = IR::Instr::New(Js::OpCode::LdStr, dstOpnd, srcOpnd, func);
                Assert(dstOpnd->m_sym->m_isSingleDef);
                if (dstOpnd->m_sym->IsSingleDef())
                {
                    dstOpnd->m_sym->SetIsStrConst();
                }
                dstOpnd->SetValueType(ValueType::String);
                srcOpnd->SetValueType(ValueType::String);
            }
            else if(type.IsNumber())
            {
                // TODO (michhol): OOP JIT. we may need to unbox before sending over const table

                if (!func->IsOOPJIT())
                {
                    srcOpnd = IR::FloatConstOpnd::New((Js::Var)varConst, TyFloat64, func);
                }
                else
                {
                    srcOpnd = IR::FloatConstOpnd::New((Js::Var)varConst, TyFloat64, func
#if !FLOATVAR
                        ,varLocal
#endif
                    );

                }

                instr = IR::Instr::New(Js::OpCode::LdC_A_R8, dstOpnd, srcOpnd, func);
                if (dstOpnd->m_sym->IsSingleDef())
                {
                    dstOpnd->m_sym->SetIsFloatConst();

#if FLOATVAR
                    dstOpnd->m_sym->m_isNotNumber = FALSE;
#else
                    // Don't set m_isNotNumber to true if the float constant value is an int32 or uint32. Uint32s may sometimes be
                    // treated as int32s for the purposes of int specialization.
                    dstOpnd->m_sym->m_isNotNumber = !Js::JavascriptNumber::IsInt32OrUInt32(((IR::FloatConstOpnd*)srcOpnd)->m_value);


#endif
                }
            }
            else
            {
                if (type.IsUndefined() || type.IsNull() || type.IsBoolean())
                {
                    valueType = type;
                }
                else
                {
                    valueType = ValueType::GetObject(ObjectType::Object);
                }
                srcOpnd = IR::AddrOpnd::New(varConst, IR::AddrOpndKindDynamicVar, func, true, varLocal);
                instr = IR::Instr::New(Js::OpCode::Ld_A, dstOpnd, srcOpnd, func);
                if (dstOpnd->m_sym->IsSingleDef())
                {
                    dstOpnd->m_sym->m_isConst = true;
                }
                dstOpnd->SetValueType(valueType);
                srcOpnd->SetValueType(valueType);
            }
        }
    }
    return instr;
}

bool Instr::UsesAllFields()
{
    return OpCodeAttr::UseAllFields(this->m_opcode) || this->CallsAccessor();
}

BranchInstr *
Instr::ChangeCmCCToBranchInstr(LabelInstr *targetInstr)
{
    Js::OpCode newOpcode = Js::OpCode::InvalidOpCode;
    switch (this->m_opcode)
    {
    case Js::OpCode::CmEq_A:
        newOpcode = Js::OpCode::BrEq_A;
        break;
    case Js::OpCode::CmGe_A:
        newOpcode = Js::OpCode::BrGe_A;
        break;
    case Js::OpCode::CmGt_A:
        newOpcode = Js::OpCode::BrGt_A;
        break;
    case Js::OpCode::CmLt_A:
        newOpcode = Js::OpCode::BrLt_A;
        break;
    case Js::OpCode::CmLe_A:
        newOpcode = Js::OpCode::BrLe_A;
        break;
    case Js::OpCode::CmUnGe_A:
        newOpcode = Js::OpCode::BrUnGe_A;
        break;
    case Js::OpCode::CmUnGt_A:
        newOpcode = Js::OpCode::BrUnGt_A;
        break;
    case Js::OpCode::CmUnLt_A:
        newOpcode = Js::OpCode::BrUnLt_A;
        break;
    case Js::OpCode::CmUnLe_A:
        newOpcode = Js::OpCode::BrUnLe_A;
        break;
    case Js::OpCode::CmNeq_A:
        newOpcode = Js::OpCode::BrNeq_A;
        break;
    case Js::OpCode::CmSrEq_A:
        newOpcode = Js::OpCode::BrSrEq_A;
        break;
    case Js::OpCode::CmSrNeq_A:
        newOpcode = Js::OpCode::BrSrNeq_A;
        break;
    case Js::OpCode::CmEq_I4:
        newOpcode = Js::OpCode::BrEq_I4;
        break;
    case Js::OpCode::CmGe_I4:
        newOpcode = Js::OpCode::BrGe_I4;
        break;
    case Js::OpCode::CmGt_I4:
        newOpcode = Js::OpCode::BrGt_I4;
        break;
    case Js::OpCode::CmLt_I4:
        newOpcode = Js::OpCode::BrLt_I4;
        break;
    case Js::OpCode::CmLe_I4:
        newOpcode = Js::OpCode::BrLe_I4;
        break;
    case Js::OpCode::CmUnGe_I4:
        newOpcode = Js::OpCode::BrUnGe_I4;
        break;
    case Js::OpCode::CmUnGt_I4:
        newOpcode = Js::OpCode::BrUnGt_I4;
        break;
    case Js::OpCode::CmUnLt_I4:
        newOpcode = Js::OpCode::BrUnLt_I4;
        break;
    case Js::OpCode::CmUnLe_I4:
        newOpcode = Js::OpCode::BrUnLe_I4;
        break;
    case Js::OpCode::CmNeq_I4:
        newOpcode = Js::OpCode::BrNeq_I4;
        break;
    default:
        Assert(UNREACHED);
        __assume(UNREACHED);
    }

    BranchInstr *instrBr = BranchInstr::New(newOpcode, targetInstr, this->m_func);
    this->InsertBefore(instrBr);
    instrBr->SetByteCodeOffset(this);
    instrBr->SetSrc1(this->UnlinkSrc1());
    instrBr->SetSrc2(this->UnlinkSrc2());

    this->Remove();

    return instrBr;
}

bool Instr::IsCmCC_A()
{
    return (this->m_opcode >= Js::OpCode::CmEq_A && this->m_opcode <= Js::OpCode::CmSrNeq_A) && this->GetSrc1()->IsVar();
}

bool Instr::IsCmCC_R8()
{
    return (this->m_opcode >= Js::OpCode::CmEq_A && this->m_opcode <= Js::OpCode::CmSrNeq_A) && this->GetSrc1()->IsFloat64();
}

bool Instr::IsCmCC_I4()
{
    return (this->m_opcode >= Js::OpCode::CmEq_I4 && this->m_opcode <= Js::OpCode::CmUnGe_I4);
}

bool Instr::IsNeq()
{
    switch (m_opcode)
    {
    case Js::OpCode::BrNeq_A:
    case Js::OpCode::BrNeq_I4:
    case Js::OpCode::BrNotEq_A:
    case Js::OpCode::BrSrNeq_A:
    case Js::OpCode::BrSrNotEq_A:
    case Js::OpCode::CmNeq_A:
    case Js::OpCode::CmNeq_I4:
    case Js::OpCode::CmSrNeq_A:
        return true;
    default:
        return false;
    }
}

template <typename T>
bool Instr::BinaryCalculatorT(T src1Const, T src2Const, int64 *pResult, bool checkWouldTrap)
{
    T value = 0;
    switch (this->m_opcode)
    {
#define DO_HANDLER(HANDLER, type) HANDLER(type##src1Const, type##src2Const)
#define BINARY_CASE_CHECK(OPCODE,HANDLER,CHECK_HANDLER,type) \
    case Js::OpCode::##OPCODE: \
        if (checkWouldTrap && DO_HANDLER(CHECK_HANDLER,type)) { return false; } \
        value = DO_HANDLER(HANDLER,type); \
        break;
#define BINARY_CASE(OPCODE,HANDLER,type) \
    case Js::OpCode::##OPCODE: \
        value = DO_HANDLER(HANDLER,type); \
        break;
#define BINARY_U(OPCODE,HANDLER) BINARY_CASE(OPCODE,HANDLER,(typename SignedTypeTraits<T>::UnsignedType))
#define BINARY(OPCODE,HANDLER)  BINARY_CASE(OPCODE,HANDLER,)

        BINARY(CmEq_I4, Js::AsmJsMath::CmpEq)
        BINARY(CmNeq_I4, Js::AsmJsMath::CmpNe)
        BINARY(CmLt_I4, Js::AsmJsMath::CmpLt)
        BINARY(CmGt_I4, Js::AsmJsMath::CmpGt)
        BINARY(CmLe_I4, Js::AsmJsMath::CmpLe)
        BINARY(CmGe_I4, Js::AsmJsMath::CmpGe)
        BINARY_U(CmUnLt_I4, Js::AsmJsMath::CmpLt)
        BINARY_U(CmUnGt_I4, Js::AsmJsMath::CmpGt)
        BINARY_U(CmUnLe_I4, Js::AsmJsMath::CmpLe)
        BINARY_U(CmUnGe_I4, Js::AsmJsMath::CmpGe)
        BINARY(Add_I4, Js::AsmJsMath::Add)
        BINARY(Sub_I4, Js::AsmJsMath::Sub)
        BINARY(Mul_I4, Js::AsmJsMath::Mul)
        BINARY(And_I4, Js::AsmJsMath::And)
        BINARY(Or_I4, Js::AsmJsMath::Or)
        BINARY(Xor_I4, Js::AsmJsMath::Xor)
        BINARY(Shl_I4, Wasm::WasmMath::Shl)
        BINARY(Shr_I4, Wasm::WasmMath::Shr)
        BINARY_U(ShrU_I4, Wasm::WasmMath::ShrU)
        BINARY_CASE_CHECK(DivU_I4, Js::AsmJsMath::DivChecked, Js::AsmJsMath::DivWouldTrap, (typename SignedTypeTraits<T>::UnsignedType))
        BINARY_CASE_CHECK(Div_I4, Js::AsmJsMath::DivChecked, Js::AsmJsMath::DivWouldTrap, )
        BINARY_CASE_CHECK(RemU_I4, Js::AsmJsMath::RemChecked, Js::AsmJsMath::RemWouldTrap, (typename SignedTypeTraits<T>::UnsignedType))
        BINARY_CASE_CHECK(Rem_I4, Js::AsmJsMath::RemChecked, Js::AsmJsMath::RemWouldTrap, )
        default:
            return false;
#undef BINARY
#undef BINARY_U
    }

    *pResult = value;
    return true;
}

template bool Instr::BinaryCalculatorT<int>(int src1Const64, int src2Const64, int64 *pResult, bool checkWouldTrap);
template bool Instr::BinaryCalculatorT<int64>(int64 src1Const64, int64 src2Const64, int64 *pResult, bool checkWouldTrap);

bool Instr::BinaryCalculator(IntConstType src1Const, IntConstType src2Const, IntConstType *pResult, IRType type)
{
    IntConstType value = 0;

    switch (this->m_opcode)
    {
    case Js::OpCode::Add_A:
        if (IntConstMath::Add(src1Const, src2Const, type, &value))
        {
            return false;
        }
        break;

    case Js::OpCode::Sub_A:
        if (IntConstMath::Sub(src1Const, src2Const, type, &value))
        {
            return false;
        }
        break;

    case Js::OpCode::Mul_A:
        if (IntConstMath::Mul(src1Const, src2Const, type, &value))
        {
            return false;
        }
        if (value == 0)
        {
            // might be -0
            // Bail for now...
            return false;
        }
        break;

    case Js::OpCode::Div_A:
        if (src2Const == 0)
        {
            // Could fold to INF/-INF
            // instr->HoistSrc1(Js::OpCode::Ld_A);
            return false;
        }
        if (src1Const == 0 && src2Const < 0)
        {
            // folds to -0. Bail for now...
            return false;
        }
        if (IntConstMath::Div(src1Const, src2Const, type, &value))
        {
            return false;
        }
        if (src1Const % src2Const != 0)
        {
            // Bail for now...
            return false;
        }
        break;

    case Js::OpCode::Rem_A:

        if (src2Const == 0)
        {
            // Bail for now...
            return false;
        }
        if (IntConstMath::Mod(src1Const, src2Const, type, &value))
        {
            return false;
        }
        if (value == 0)
        {
            // might be -0
            // Bail for now...
            return false;
        }
        break;

    case Js::OpCode::Shl_A:
        // We don't care about overflow here
        value = src1Const << (src2Const & 0x1F);
        break;

    case Js::OpCode::Shr_A:
        value = src1Const >> (src2Const & 0x1F);
        break;

    case Js::OpCode::ShrU_A:
        value = ((UIntConstType)src1Const) >> (src2Const & 0x1F);
        if (value < 0)
        {
            // ShrU produces a UInt32.  If it doesn't fit in an Int32, bail as we don't
            // track signs of int values.
            return false;
        }
        break;

    case Js::OpCode::And_A:
        value = src1Const & src2Const;
        break;

    case Js::OpCode::Or_A:
        value = src1Const | src2Const;
        break;

    case Js::OpCode::Xor_A:
        value = src1Const ^ src2Const;
        break;

    case Js::OpCode::InlineMathMin:
        value = src1Const < src2Const ? src1Const : src2Const;
        break;

    case Js::OpCode::InlineMathMax:
        value = src1Const > src2Const ? src1Const : src2Const;
        break;

    default:
        return false;
    }

    *pResult = value;

    return true;
}

bool Instr::UnaryCalculator(IntConstType src1Const, IntConstType *pResult, IRType type)
{
    IntConstType value = 0;

    switch (this->m_opcode)
    {
    case Js::OpCode::Neg_A:
        if (src1Const == 0)
        {
            // Could fold to -0.0
            return false;
        }

        if (IntConstMath::Neg(src1Const, type, &value))
        {
            return false;
        }
        break;

    case Js::OpCode::Not_A:
        value = ~src1Const;
        break;

    case Js::OpCode::Ld_A:
        if (this->HasBailOutInfo())
        {
            Assert(this->GetBailOutKind() == IR::BailOutExpectingInteger);
            this->ClearBailOutInfo();
        }
        value = src1Const;
        break;

    case Js::OpCode::Conv_Num:
    case Js::OpCode::Ld_I4:
        value = src1Const;
        break;

    case Js::OpCode::Incr_A:
        if (IntConstMath::Inc(src1Const, type, &value))
        {
            return false;
        }
        break;

    case Js::OpCode::Decr_A:
        if (IntConstMath::Dec(src1Const, type, &value))
        {
            return false;
        }
        break;


    case Js::OpCode::InlineMathAbs:
        if (src1Const == IntConstMin)
        {
            return false;
        }
        else
        {
            value = src1Const < 0 ? -src1Const : src1Const;
        }
        break;

    case Js::OpCode::InlineMathClz:
        DWORD clz;
        DWORD src1Const32;
        src1Const32 = (DWORD)src1Const;
        if (_BitScanReverse(&clz, src1Const32))
        {
            value = 31 - clz;
        }
        else
        {
            value = 32;
        }
        this->ClearBailOutInfo();
        break;

    case Js::OpCode::InlineMathFloor:
        value = src1Const;
        this->ClearBailOutInfo();
        break;

    case Js::OpCode::InlineMathCeil:
        value = src1Const;
        this->ClearBailOutInfo();
        break;

    case Js::OpCode::InlineMathRound:
        value = src1Const;
        this->ClearBailOutInfo();
        break;
    case Js::OpCode::ToVar:
        if (Js::TaggedInt::IsOverflow(src1Const))
        {
            return false;
        }
        else
        {
            value = src1Const;
            this->ClearBailOutInfo();
            break;
        }
    default:
        return false;
    }

    *pResult = value;
    return true;
}

GeneratorBailInInstr*
GeneratorBailInInstr::New(IR::Instr* yieldInstr, Func* func)
{
    GeneratorBailInInstr* labelInstr = JitAnew(func->m_alloc, IR::GeneratorBailInInstr, func->m_alloc, yieldInstr);
    labelInstr->Init(Js::OpCode::GeneratorBailInLabel, InstrKindLabel, func, false /* isOpHelper */);
    return labelInstr;
}

#if ENABLE_DEBUG_CONFIG_OPTIONS
///----------------------------------------------------------------------------
///
/// Instr::DumpTestTrace
///
///     Dump this instr in TestTrace.
///
///----------------------------------------------------------------------------

void
Instr::DumpTestTrace()
{
    Output::Print(_u("opcode: %s "), Js::OpCodeUtil::GetOpCodeName(m_opcode));
    SymOpnd * symOpnd;

    if (this->m_opcode == Js::OpCode::NewScFunc || this->m_opcode == Js::OpCode::NewScGenFunc)
    {
        Output::Print(_u("\n"));
        return;
    }
    Opnd * src1 = this->GetSrc1();
    if (!src1)
    {
        Output::Print(_u("\n"));
        return;
    }
    if (src1->GetKind() != OpndKindSym)
    {
        Output::Print(_u("\n"));
        return;
    }

    symOpnd = src1->AsSymOpnd();
    if (symOpnd->m_sym->IsPropertySym())
    {
        PropertySym *propertySym = symOpnd->m_sym->AsPropertySym();

        switch (propertySym->m_fieldKind)
        {
        case PropertyKindData:
            if (!JITManager::GetJITManager()->IsOOPJITEnabled())
            {
                Js::PropertyRecord const* fieldName = propertySym->GetFunc()->GetInProcThreadContext()->GetPropertyRecord(propertySym->m_propertyId);
                Output::Print(_u("field: %s "), fieldName->GetBuffer());
                break;
            }
            // else fall through
        case PropertyKindSlots:
            Output::Print(_u("field: [%d] "), propertySym->m_propertyId);
            break;
        case PropertyKindLocalSlots:
            Output::Print(_u("field: l[%d] "), propertySym->m_propertyId);
            break;
        default:
            break;
        }
        Output::Print(_u("\n"));
    }
}

///----------------------------------------------------------------------------
///
/// Instr::DumpFieldCopyPropTestTrace
///
///     Dump fieldcopyprop when testtrace is enabled.
///
///----------------------------------------------------------------------------

void
Instr::DumpFieldCopyPropTestTrace(bool inLandingPad)
{
    switch (m_opcode)
    {
    case Js::OpCode::LdSlot:
    case Js::OpCode::LdSlotArr:
    case Js::OpCode::LdFld:
    case Js::OpCode::LdFldForTypeOf:
    case Js::OpCode::LdRootFld:
    case Js::OpCode::LdRootFldForTypeOf:
    case Js::OpCode::LdMethodFld:
    case Js::OpCode::LdRootMethodFld:
    case Js::OpCode::LdMethodFromFlags:
    case Js::OpCode::ScopedLdMethodFld:
    case Js::OpCode::TypeofElem:

        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
        Output::Print(_u("TestTrace fieldcopyprop"));
        if (inLandingPad)
        {
            Output::Print(_u(" [%s]"), _u("in landing pad"));
        }
        Output::Print(_u(": function %s (%s) "),
            this->m_func->GetJITFunctionBody()->GetDisplayName(),
            this->m_func->GetDebugNumberSet(debugStringBuffer));

        if (this->IsInlined())
        {
            Output::Print(_u("inlined caller function %s (%s) "),
                this->m_func->GetTopFunc()->GetJITFunctionBody()->GetDisplayName(),
                this->m_func->GetTopFunc()->GetDebugNumberSet(debugStringBuffer));
        }
        this->DumpTestTrace();
    default:
        break;
    }
}
#endif

#if ENABLE_DEBUG_CONFIG_OPTIONS

const char *
Instr::GetBailOutKindName() const
{
    IR::BailOutKind kind = (IR::BailOutKind)0;
    if (this->HasBailOutInfo())
    {
        kind |= this->GetBailOutKind();
    }
    if (this->HasAuxBailOut())
    {
        kind |= this->GetAuxBailOutKind();
    }

    return ::GetBailOutKindName(kind);
}

#endif

//
// Debug dumpers
//

#if DBG_DUMP

void
Instr::DumpByteCodeOffset()
{
    if (m_func->HasByteCodeOffset())
    {
        Output::SkipToColumn(78);
        Output::Print(_u("#"));
        if (this->m_number != Js::Constants::NoByteCodeOffset)
        {
            Output::Print(_u("%04x"), this->m_number);
            Output::Print(this->IsCloned()? _u("*") : _u(" "));
        }
    }
    if (!this->m_func->IsTopFunc())
    {
        Output::SkipToColumn(78);
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
        Output::Print(_u(" Func #%s"), this->m_func->GetDebugNumberSet(debugStringBuffer));
    }
#ifdef BAILOUT_INJECTION
    if (this->bailOutByteCodeLocation != (uint)-1)
    {
        Output::SkipToColumn(85);
        Output::Print(_u("@%4d"), this->bailOutByteCodeLocation);
    }
#endif
    if (this->m_opcode == Js::OpCode::InlineeStart)
    {
        Output::Print(_u(" %s"), this->m_func->GetJITFunctionBody()->GetDisplayName());
    }
}

void
Instr::DumpGlobOptInstrString()
{
    if(this->globOptInstrString && !PHASE_OFF(Js::DumpGlobOptInstrPhase, m_func))
    {
        Output::Print(_u("\n\n GLOBOPT INSTR: %s\n\n"), this->globOptInstrString);
    }
}

///----------------------------------------------------------------------------
///
/// Instr::Dump
///
///     Dump this instr.
///
///----------------------------------------------------------------------------

void
Instr::Dump(IRDumpFlags flags)
{
    bool const AsmDumpMode = flags & IRDumpFlags_AsmDumpMode;
    bool const SimpleForm = !!(flags & IRDumpFlags_SimpleForm);
    bool const SkipByteCodeOffset = !!(flags & IRDumpFlags_SkipByteCodeOffset);

    const auto PrintOpCodeName = [&]() {
        Output::SkipToColumn(23);
#if DBG
        WORD oldValue = 0;
        if (this->highlight != 0)
        {
            oldValue = Output::SetConsoleForeground(this->highlight);
        }
#endif
        Output::Print(_u("%s "), Js::OpCodeUtil::GetOpCodeName(m_opcode));
#if DBG
        if (this->highlight != 0)
        {
            Output::SetConsoleForeground(oldValue);
        }
#endif
        Output::SkipToColumn(38);
    };

    // forward decl before goto statement
    Opnd * dst = nullptr;

    if(m_opcode == Js::OpCode::BoundCheck || m_opcode == Js::OpCode::UnsignedBoundCheck)
    {
        PrintOpCodeName();

        // src1 <= src2 + dst

        Assert(GetSrc1());
        if(GetSrc1()->IsIntConstOpnd())
        {
            Output::Print(_u("%d"), GetSrc1()->AsIntConstOpnd()->GetValue());
        }
        else
        {
            GetSrc1()->Dump(flags, m_func);
        }

        bool useLessThanOrEqual = true;
        bool usePlus = true;
        bool dumpSrc2 = false;
        int32 offset = GetDst() ? GetDst()->AsIntConstOpnd()->AsInt32() : 0;
        if(GetSrc2())
        {
            if(GetSrc2()->IsIntConstOpnd())
            {
            #if DBG
                int32 temp;
                Assert(!Int32Math::Add(offset, GetSrc2()->AsIntConstOpnd()->AsInt32(), &temp));
            #endif
                offset += GetSrc2()->AsIntConstOpnd()->AsInt32();
            }
            else
            {
                dumpSrc2 = true;
                if(offset == -1)
                {
                    useLessThanOrEqual = false; // < instead of <=
                    offset = 0;
                }
                else if(offset < 0 && offset != IntConstMin)
                {
                    usePlus = false;
                    offset = -offset;
                }
            }
        }

        Output::Print(_u(" %S "), useLessThanOrEqual ? "<=" : "<");
        if(dumpSrc2)
        {
            GetSrc2()->Dump(flags, m_func);
        }
        if(offset != 0)
        {
            if(dumpSrc2)
            {
                Output::Print(_u(" %C "), usePlus ? '+' : '-');
            }
            Output::Print(_u("%d"), offset);
        }

        goto PrintByteCodeOffsetEtc;
    }

    Output::SkipToColumn(4);

    dst = this->GetDst();

    if (dst)
    {
        dst->Dump(flags, this->m_func);

        bool const dumpMarkTemp = PHASE_DUMP(Js::MarkTempPhase, m_func)
            || PHASE_TRACE(Js::MarkTempPhase, m_func);
        bool const dumpMarkTempNumber = dumpMarkTemp || PHASE_DUMP(Js::MarkTempNumberPhase, m_func)
            || PHASE_TRACE(Js::MarkTempNumberPhase, m_func);
        bool const dumpMarkTempObject = dumpMarkTemp || PHASE_DUMP(Js::MarkTempObjectPhase, m_func)
            || PHASE_TRACE(Js::MarkTempObjectPhase, m_func);

        if ((dumpMarkTempNumber && (this->dstIsTempNumberTransferred || this->dstIsTempNumber))
            || (dumpMarkTempObject && this->dstIsTempObject))
        {
            Output::Print(_u("["));

            if (dumpMarkTempNumber)
            {
                if (Js::Configuration::Global.flags.Verbose || OpCodeAttr::TempNumberProducing(this->m_opcode))
                {
                    if (this->dstIsTempNumberTransferred)
                    {
                        Assert(this->dstIsTempNumber);
                        Output::Print(_u("x"));
                    }
                    else if (this->dstIsTempNumber)
                    {
                        Output::Print(_u("#"));
                    }
                }
            }
            if (dumpMarkTempObject)
            {
                if (Js::Configuration::Global.flags.Verbose || OpCodeAttr::TempObjectProducing(this->m_opcode))
                {
                    if (this->dstIsTempObject)
                    {
                        Output::Print(_u("o"));
                    }
                }
            }

            Output::Print(_u("tmp]"));
        }
        if(PHASE_DUMP(Js::TrackNegativeZeroPhase, m_func->GetTopFunc()) && !ShouldCheckForNegativeZero())
        {
            Output::Print(_u("[-0]"));
        }
        if (PHASE_DUMP(Js::TypedArrayVirtualPhase, m_func->GetTopFunc()) && (!IsDstNotAlwaysConvertedToInt32() || !IsDstNotAlwaysConvertedToNumber()))
        {
            if (!IsDstNotAlwaysConvertedToInt32())
                Output::Print(_u("[->i]"));
            else
                Output::Print(_u("[->n]"));

        }
        if(PHASE_DUMP(Js::TrackIntOverflowPhase, m_func->GetTopFunc()))
        {
            // ignoring 32-bit overflow ?
            if(!ShouldCheckFor32BitOverflow())
            {
                // ignoring 32-bits or more ?
                if(ShouldCheckForNon32BitOverflow())
                    Output::Print(_u("[OF %d]"), ignoreOverflowBitCount);
                else
                    Output::Print(_u("[OF]"));
            }
        }

        if (this->isSafeToSpeculate)
        {
            Output::SkipToColumn(19);
            Output::Print(_u("<=="));
        }
        else
        {
            Output::SkipToColumn(20);
            Output::Print(_u("="));
        }
    }

    PrintOpCodeName();

    if (this->IsBranchInstr())
    {
        BranchInstr * branchInstr = this->AsBranchInstr();
        LabelInstr * targetInstr = branchInstr->GetTarget();
        bool labelPrinted = true;
        if (targetInstr == NULL)
        {
            // Checking the 'm_isMultiBranch' field here directly as well to bypass asserting when tracing IR builder
            if(branchInstr->m_isMultiBranch && branchInstr->IsMultiBranch())
            {
                IR::MultiBranchInstr * multiBranchInstr = branchInstr->AsMultiBrInstr();

                // If this MultiBranchInstr has been lowered to a machine instruction, which means
                // its opcode is not Js::OpCode::MultiBr, there is no need to print the labels.
                if (this->m_opcode == Js::OpCode::MultiBr)
                {
                    multiBranchInstr->MapMultiBrLabels([](IR::LabelInstr * labelInstr) -> void
                    {
                        Output::Print(_u("$L%d "), labelInstr->m_id);
                    });
                }
                else
                {
                    labelPrinted = false;
                }
            }
            else
            {
                Output::Print(_u("??"));
            }
        }
        else
        {
            Output::Print(_u("$L%d"), targetInstr->m_id);
        }
        if (this->GetSrc1() && labelPrinted)
        {
            Output::Print(_u(", "));
        }
    }
    else if (this->IsPragmaInstr() && this->m_opcode == Js::OpCode::StatementBoundary)
    {
        Output::Print(_u("#%d"), this->AsPragmaInstr()->m_statementIndex);
    }

    // scope
    {
        Opnd * src1 = this->GetSrc1();
        if (this->m_opcode == Js::OpCode::NewScFunc || this->m_opcode == Js::OpCode::NewScGenFunc)
        {
            Assert(src1->IsIntConstOpnd());
            Js::ParseableFunctionInfo * function = nullptr;
            if (!m_func->IsOOPJIT())
            {
                function = ((Js::ParseableFunctionInfo *)m_func->GetJITFunctionBody()->GetAddr())->GetNestedFunctionForExecution((uint)src1->AsIntConstOpnd()->GetValue())->GetParseableFunctionInfo();
            }
            Output::Print(_u("func:%s()"), function ? function->GetDisplayName() : _u("???"));
            Output::Print(_u(", env:"));
            this->GetSrc2()->AsRegOpnd()->m_sym->Dump(flags);
        }
        else if (src1)
        {
            src1->Dump(flags, this->m_func);
            Opnd * src2 = this->GetSrc2();
            if (src2)
            {
                Output::Print(_u(", "));
                src2->Dump(flags, this->m_func);
            }
        }
    }

    if (this->IsByteCodeUsesInstr() || this->m_opcode == Js::OpCode::SpeculatedLoadFence)
    {
        ByteCodeUsesInstr* tempbcu = static_cast<ByteCodeUsesInstr*>(this);
        if (tempbcu->GetByteCodeUpwardExposedUsed())
        {
            bool first = true;
            FOREACH_BITSET_IN_SPARSEBV(id, tempbcu->GetByteCodeUpwardExposedUsed())
            {
                Output::Print(first? _u("s%d") : _u(", s%d"), id);
                first = false;
            }
            NEXT_BITSET_IN_SPARSEBV;
        }
        if (tempbcu->propertySymUse)
        {
            Output::Print(_u("  PropSym: %d"), tempbcu->propertySymUse->m_id);
        }
    }

PrintByteCodeOffsetEtc:
    if (!AsmDumpMode && !SkipByteCodeOffset)
    {
        this->DumpByteCodeOffset();
    }

    if (!SimpleForm)
    {
        if (this->HasBailOutInfo() || this->HasAuxBailOut())
        {
            BailOutInfo * bailOutInfo = this->GetBailOutInfo();
            Output::SkipToColumn(85);
            if (!AsmDumpMode)
            {
                Output::Print(_u("Bailout: #%04x"), bailOutInfo->bailOutOffset);
            }
            if (!bailOutInfo->bailOutFunc->IsTopFunc())
            {
                char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                Output::Print(_u(" Func %s"), bailOutInfo->bailOutFunc->GetDebugNumberSet(debugStringBuffer));
            }
            Output::Print(_u(" (%S)"), this->GetBailOutKindName());
        }
    }
    if ((flags & IRDumpFlags_SkipEndLine) == 0)
    {
        Output::Print(_u("\n"));
    }
}

#if DBG
bool
Instr::ShouldEmitIntRangeCheck()
{
    // currently only emitting int range check for opnds of instructions with following opcodes:
    return m_opcode == Js::OpCode::ToVar ||
        m_opcode == Js::OpCode::LdElemI_A ||
        m_opcode == Js::OpCode::LdMethodElem ||
        m_opcode == Js::OpCode::StElemI_A ||
        m_opcode == Js::OpCode::StElemI_A_Strict ||
        m_opcode == Js::OpCode::StElemC;
}
#endif
///----------------------------------------------------------------------------
///
/// LabelInstr::Dump
///
///     Dump this label.
///
///----------------------------------------------------------------------------

void
LabelInstr::Dump(IRDumpFlags flags)
{
    if (this->m_block != NULL)
    {
        this->m_block->DumpHeader();
    }
#if DBG
    if (this->m_name != nullptr)
    {
        Output::Print(_u("$L%d (%s):"), this->m_id, this->m_name);
    }
    else
#endif
    {
        Output::Print(_u("$L%d:"), this->m_id);
    }
    if (this->isOpHelper)
    {
        Output::Print(_u(" [helper]"));
    }
    if (this->m_isLoopTop)
    {
        Output::Print(_u(" >>>>>>>>>>>>>  LOOP TOP  >>>>>>>>>>>>>"));
    }
    if (this->IsProfiledLabelInstr())
    {
        Output::SkipToColumn(50);
        switch (this->AsProfiledLabelInstr()->loopImplicitCallFlags)
        {
        case Js::ImplicitCall_HasNoInfo:
            Output::Print(_u("Implicit call: ???"));
            break;
        case Js::ImplicitCall_None:
            Output::Print(_u("Implicit call: no"));
            break;
        default:
            Output::Print(_u("Implicit call: yes"));
            break;
        }
    }
    if ((flags & (IRDumpFlags_AsmDumpMode | IRDumpFlags_SkipByteCodeOffset)) == 0)
    {
        this->DumpByteCodeOffset();
    }
    Output::Print(_u("\n"));
}


void
PragmaInstr::Dump(IRDumpFlags flags)
{
    if (Js::Configuration::Global.flags.PrintSrcInDump && this->m_opcode == Js::OpCode::StatementBoundary)
    {
        Js::FunctionBody * functionBody = nullptr;
        if (!m_func->IsOOPJIT())
        {
            functionBody = ((Js::FunctionBody*)m_func->GetJITFunctionBody()->GetAddr());
        }
        if (functionBody && !functionBody->GetUtf8SourceInfo()->GetIsLibraryCode())
        {
            functionBody->PrintStatementSourceLine(this->m_statementIndex);
        }
    }
    __super::Dump(flags);
}

///----------------------------------------------------------------------------
///
/// Instr::Dump
///
///     Dump a window of instructions around this instr.
///
///----------------------------------------------------------------------------

void
Instr::Dump(int window)
{
    Instr * instr;
    int i;

    Output::Print(_u("-------------------------------------------------------------------------------"));

    if (this == NULL)
    {
        return;
    }

    for (i = 0, instr = this; (instr->m_prev != NULL && i < window/2); instr = instr->m_prev, ++i)
    {} // Nothing


    for (i = 0; (instr != nullptr && i < window); instr = instr->m_next, ++i)
    {
        if (instr == this)
        {
            Output::Print(_u("=>"));
        }
        instr->Dump();
    }
}

void
Instr::Dump()
{
    this->Dump(IRDumpFlags_None);
}

void
Instr::DumpSimple()
{
    this->Dump(IRDumpFlags_SimpleForm);
}

char16 *
Instr::DumpString()
{
    Output::CaptureStart();
    this->Dump();
    return Output::CaptureEnd();
}

void
Instr::DumpRange(Instr *instrEnd)
{
    Output::Print(_u("-------------------------------------------------------------------------------\n"));

    FOREACH_INSTR_IN_RANGE(instr, this, instrEnd)
    {
        instr->Dump();
    }
    NEXT_INSTR_IN_RANGE;

    Output::Print(_u("-------------------------------------------------------------------------------\n"));
}

#endif

} // namespace IR
