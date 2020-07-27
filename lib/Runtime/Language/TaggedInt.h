//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js {

    class TaggedInt
    {
    public:
        static Var Increment(Var aRight, ScriptContext* scriptContext);
        static Var Decrement(Var aRight, ScriptContext* scriptContext);
        static Var Negate(Var aRight, ScriptContext* scriptContext);
        static Var NegateUnchecked(Var aRight);
        static Var Not(Var aRight, ScriptContext* scriptContext);

        static Var Add(Var aLeft, Var aRight, ScriptContext* scriptContext);
        static Var Divide(Var aLeft, Var aRight, ScriptContext* scriptContext);
        static Var DivideInPlace(Var aLeft, Var aRight, ScriptContext* scriptContext, JavascriptNumber *result);
        static Var Modulus(Var aLeft, Var aRight, ScriptContext* scriptContext);
        static Var Multiply(Var aLeft, Var aRight,ScriptContext* scriptContext);
        static Var MultiplyInPlace(Var aLeft, Var aRight, ScriptContext* scriptContext, JavascriptNumber *result);
        static Var Subtract(Var aLeft, Var aRight,ScriptContext* scriptContext);

        static Var And(Var aLeft, Var aRight);
        static Var Or(Var aLeft, Var aRight);
        static Var Xor(Var aLeft, Var aRight);
        static Var ShiftLeft(Var aLeft, Var aShift, ScriptContext* scriptContext);
        static Var ShiftRight(Var aLeft, Var aShift);
        static Var ShiftRightU(Var aLeft, Var aShift, ScriptContext* scriptContext);

        static Var Speculative_And(Var aLeft, Var aRight);

#if defined(__clang__) && defined(_M_IX86)
        static bool IsOverflow(intptr_t nValue);
#endif
        static bool IsOverflow(int32 nValue);
        static bool IsOverflow(uint32 nValue);
        static bool IsOverflow(int64 nValue);
        static bool IsOverflow(uint64 nValue);


        static bool Is(Var aValue);
        static bool Is(intptr_t aValue);
        static bool IsPair(Var aLeft, Var aRight);
        static bool OnlyContainsTaggedInt(Js::Arguments& args);
        static double ToDouble(Var aValue);
        static int32 ToInt32(Var aValue);
        static int32 ToInt32(intptr_t aValue);
        static uint32 ToUInt32(Var aValue);
        static int64 ToInt64(Var aValue);
        static uint16 ToUInt16(Var aValue);
        static Var ToVarUnchecked(int nValue);
        static int ToBuffer(Var aValue, __out_ecount_z(bufSize) char16 * buffer, uint bufSize);
        static int ToBuffer(int value, __out_ecount_z(bufSize) char16 * buffer, uint bufSize);
        static int ToBuffer(uint value, __out_ecount_z(bufSize) char16 * buffer, uint bufSize);
        static JavascriptString* ToString(Var aValue,ScriptContext* scriptContext);
        static JavascriptString* ToString(int value,ScriptContext* scriptContext);
        static JavascriptString* ToString(uint value,ScriptContext* scriptContext);

        static int SignedToString(__int64 value, char16 *buffer, int bufferSize);
        static int UnsignedToString(unsigned __int64 value, char16 *buffer, int bufferSize);

        static Var MinVal() { return ToVarUnchecked(k_nMinValue); }
        static Var MaxVal() { return ToVarUnchecked(k_nMaxValue); }

    private:
        static Var DivideByZero(int nLeft, ScriptContext* scriptContext);
        static Var __stdcall OverflowHelper(int overflowValue, ScriptContext* scriptContext);
        static Var __stdcall IncrementOverflowHelper(ScriptContext* scriptContext);
        static Var __stdcall DecrementUnderflowHelper(ScriptContext* scriptContext);

#ifdef DBG
        static Var DbgAdd(Var aLeft, Var aRight,ScriptContext* scriptContext);
        static Var DbgSubtract(Var aLeft, Var aRight,ScriptContext* scriptContext);
#endif

        static const int k_nMinValue = INT_MIN / AtomTag_Multiply;
        static const int k_nMaxValue = INT_MAX / AtomTag_Multiply;
    };

    template <> inline bool VarIs<TaggedInt>(Var aValue)
    {
        return TaggedInt::Is(aValue);
    }

    // Helper representing operations and checks on TaggedInteger
    // and JavascriptNumber ( aka TaggedFloat) - JavascriptNumber is a Tagged value
    // only for 64-bit platforms.
    class TaggedNumber
    {
    public:
        static bool Is(Var aValue);
    };
} // namespace Js
