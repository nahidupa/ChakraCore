//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js {

    struct SIMDFloat64x2Operation
    {
        // following are operation wrappers for SIMD.Float64x2 general implementation
        static SIMDValue OpFloat64x2(double x, double y);
        static SIMDValue OpSplat(double x);

        // conversion
        static SIMDValue OpFromFloat32x4(const SIMDValue& value);
        static SIMDValue OpFromInt32x4(const SIMDValue& value);

        // Unary Ops
        static SIMDValue OpAbs(const SIMDValue& v);
        static SIMDValue OpNeg(const SIMDValue& v);
        static SIMDValue OpNot(const SIMDValue& v);
        static SIMDValue OpReciprocal(const SIMDValue& v);
        static SIMDValue OpReciprocalSqrt(const SIMDValue& v);
        static SIMDValue OpSqrt(const SIMDValue& v);

        // Binary Ops
        static SIMDValue OpAdd(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpSub(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMul(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpDiv(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpAnd(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpOr (const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpXor(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMin(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMax(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpScale(const SIMDValue& Value, double scaleValue);
        static SIMDValue OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpEqual(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpSelect(const SIMDValue& mV, const SIMDValue& tV, const SIMDValue& fV);
        template<typename T> static void OpConv(SIMDValue* dst, SIMDValue* src);
    };

} // namespace Js
