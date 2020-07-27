//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{

#if defined(__clang__) && defined(_M_IX86)
    inline bool TaggedInt::IsOverflow(intptr_t nValue)
    {
        return (nValue < k_nMinValue) || (nValue > k_nMaxValue);
    }
#endif
    inline bool TaggedInt::IsOverflow(int32 nValue)
    {
        return (nValue < k_nMinValue) || (nValue > k_nMaxValue);
    }

    inline bool TaggedInt::IsOverflow(uint32 nValue)
    {
        return nValue > k_nMaxValue;
    }

    inline bool TaggedInt::IsOverflow(int64 nValue)
    {
        return (nValue < k_nMinValue) || (nValue > k_nMaxValue);
    }

    inline bool TaggedInt::IsOverflow(uint64 nValue)
    {
        return nValue > k_nMaxValue;
    }

#if INT32VAR
    inline bool TaggedInt::Is(Var aValue)
    {
        bool result = (((uintptr_t) aValue) >> VarTag_Shift) == AtomTag;
        if(result)
        {
            Assert((uintptr_t)aValue >> 32 == (AtomTag << 16));
        }
        return result;
    }

    inline bool TaggedInt::Is(intptr_t aValue)
    {
        bool result = (aValue >> VarTag_Shift) == AtomTag;
        if (result)
        {
            Assert((aValue >> 32) == (AtomTag << 16));
        }
        return result;
    }

    inline bool TaggedInt::IsPair(Var aLeft, Var aRight)
    {
        uint32 tags = (uint32)((uint64)aLeft >> 32 | (uint64)aRight >> 48);
        bool result = (tags == AtomTag_Pair);
        AssertMsg(result == (TaggedInt::Is(aLeft) && TaggedInt::Is(aRight)), "TaggedInt::IsPair() logic is incorrect");
        return result;
    }

    inline int32 TaggedInt::ToInt32(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'TaggedInt'");

        return ::Math::PointerCastToIntegralTruncate<int32>(aValue);
    }

    inline int32 TaggedInt::ToInt32(intptr_t aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually an 'TaggedInt'");

        return (int32)aValue;
    }

    inline uint32 TaggedInt::ToUInt32(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'TaggedInt'");

        return ::Math::PointerCastToIntegralTruncate<uint32>(aValue);
    }

    inline int64 TaggedInt::ToInt64(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'TaggedInt'");

        int64 nValue = (int64)(::Math::PointerCastToIntegralTruncate<int32>(aValue));
        AssertMsg(nValue == (int64) ToInt32(aValue),
                "Ensure 32-bit and 64-bit operations return same result");

        return nValue;
    }

    inline uint16 TaggedInt::ToUInt16(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'TaggedInt'");

        return ::Math::PointerCastToIntegralTruncate<uint16>(aValue);
    }

    inline double TaggedInt::ToDouble(Var aValue)
    {
        return (double)::Math::PointerCastToIntegralTruncate<int32>(aValue);
    }

    inline Var TaggedInt::ToVarUnchecked(int nValue)
    {
        //
        // To convert to a var we first cast to uint32 to lose the signedness and then
        // extend it to a 64-bit uintptr_t before OR'ing the 64-bit atom tag.
        //

        AssertMsg(!IsOverflow(nValue), "Ensure no information loss from conversion");

        return reinterpret_cast<Var>(((uintptr_t)(uint32)nValue) | AtomTag_IntPtr);
    }

#else
    inline bool TaggedInt::Is(const Var aValue)
    {
        return (((uintptr_t) aValue) & AtomTag) == AtomTag_IntPtr;
    }

    __inline bool TaggedInt::Is(intptr_t aValue)
    {
        return (aValue & AtomTag) == AtomTag_IntPtr;
    }

    inline bool TaggedInt::IsPair(Var aLeft, Var aRight)
    {
        //
        // Check if both Atoms are tagged as "SmInts":
        // - Because we're checking tag bits, we don't need to check against 'null', since it won't
        //   be tagged as a TaggedInt.
        // - This degenerates into bitwise arithmetic that avoids the overhead of branching and
        //   short-circuit evaluation.
        //

        return (((uintptr_t) aLeft) & ((uintptr_t) aRight) & AtomTag) == AtomTag_IntPtr;
    }

    inline int32 TaggedInt::ToInt32(Var aValue)
    {
        //
        // To convert from a var, must first convert to an 'int32' to properly sign-extend
        // negative values.  Then, use shift operation to remove the tag bits.
        //

        AssertMsg(Is(aValue), "Ensure var is actually a 'TaggedInt'");

        return ((int) aValue) >> VarTag_Shift;
    }

    inline uint32 TaggedInt::ToUInt32(Var aValue)
    {
        //
        // To convert from a var, must use ToInt32() to properly sign-extend negative values, then
        // convert to an (unsigned) uint32.
        //

        return (uint32) ToInt32(aValue);
    }

    inline int64 TaggedInt::ToInt64(Var aValue)
    {
       //
        // To convert from a var, must first convert to an 'int64' to properly sign-extend
        // negative values.  Then, use shift operation to remove the tag bits.
        //

        AssertMsg(Is(aValue), "Ensure var is actually a 'TaggedInt'");

        int64 nValue = ((int32) aValue) >> VarTag_Shift;
        AssertMsg(nValue == (int64) ToInt32(aValue),
                "Ensure 32-bit and 64-bit operations return same result");

        return nValue;
    }

    inline uint16 TaggedInt::ToUInt16(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'TaggedInt'");

        return (uint16)(((int) aValue) >> VarTag_Shift);
    }

    inline double TaggedInt::ToDouble(Var aValue)
    {
        return (double) ToInt32(aValue);
    }

    inline Var TaggedInt::ToVarUnchecked(int nValue)
    {
        //
        // To convert to a var, must first multiply (which will be converted into a shift
        // operation) to create space for the tag while properly preserving negative values.  Then,
        // add the tag.
        //

        AssertMsg(!IsOverflow(nValue), "Ensure no information loss from conversion");

        return reinterpret_cast<Var>((nValue << VarTag_Shift) | AtomTag_IntPtr);
    }
#endif

    // Returns true when the provided args only contains TaggedInts. Note that
    // iteration starts from 1 (to account for 'this' at index 0).
    bool inline TaggedInt::OnlyContainsTaggedInt(Js::Arguments& args)
    {
        bool onlyContainsInts = true;
        for (uint idxArg = 1; idxArg < args.Info.Count; idxArg++)
        {
            if (!TaggedInt::Is(args[idxArg]))
            {
                onlyContainsInts = false;
                break;
            }
        }

        return onlyContainsInts;
    }

    inline Var TaggedInt::Add(Var aLeft,Var aRight,ScriptContext* scriptContext)
#ifdef DBG
    {
        Var sum = DbgAdd(aLeft, aRight, scriptContext);
        AssertMsg(JavascriptConversion::ToNumber(sum,scriptContext) == ToDouble(aLeft) + ToDouble(aRight), "TaggedInt fast addition is broken");
        return sum;
    }

    inline Var TaggedInt::DbgAdd(Var aLeft,Var aRight,ScriptContext* scriptContext)
#endif
    {
#if _M_IX86
        //
        // Perform the signed, integer addition directly on Atoms without converting to integers:
        //
        // T        = AtomTag_Int32
        // nResult  = A1 + A2
        //  Step 1: (N1 * S + T) + (N2 * S + T)
        //  Step 2: ((N1 + N2) * S + T) + T
        //  Step 3: A3 + T
        //
        // NOTE: As demonstrated above, the FromVar() / ToVar() calls in (T) will cancel out,
        // enabling an optimized operation.
        //


        __asm
        {
            mov     eax, aLeft
            dec     eax             // Get rid of one of the tags, since they'll add up
            add     eax, aRight     // Perform the addition
            jno     LblDone         // Check for overflow/underflow
                                    // The carry flag indicates whether the sum has
                                    // overflowed (>INT_MAX) or underflowed (< INT_MIN)

            push    scriptContext
            rcr     eax, 1          // Convert to int32 and set the sign to the carry bit
            push    eax
            call    TaggedInt::OverflowHelper

LblDone:
            // Sum is in eax
        }

#elif defined(_M_X64) || defined(_M_ARM32_OR_ARM64)

        //
        // Perform the signed, integer addition directly on Atoms using 64-bit math for overflow
        // checking.
        //

        int64 nResult64 = ToInt64(aLeft) + ToInt64(aRight);
        if (IsOverflow(nResult64))
        {
            //
            // Promote result to double.
            //

            return JavascriptNumber::ToVarNoCheck((double) nResult64, scriptContext);
        }
        else
        {
            //
            // Return A3
            //

            int nResult32 = (int) nResult64;
            return ToVarUnchecked(nResult32);
        }

#else

#error Unsupported processor type

#endif
    }


    //
    // True if the value is a tagged number representation (for x64 - float & integers) - otherwise false.
    //
    inline bool TaggedNumber::Is(const Var aValue)
    {
        bool isTaggedNumber;
#if FLOATVAR
        // If we add another tagged representation that is not numerical - this will not work.
        isTaggedNumber = !VarIs<RecyclableObject>(aValue);
#else
        isTaggedNumber = TaggedInt::Is(aValue);
#endif
        return isTaggedNumber;
    }
} // namespace Js
