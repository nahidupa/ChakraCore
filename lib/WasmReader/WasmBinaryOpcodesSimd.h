//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef WASM_LANE_OPCODE
#define WASM_LANE_OPCODE(opname, opcode, sig, imp, wat) WASM_OPCODE(opname, opcode, sig, imp, wat)
#endif

#ifndef WASM_EXTRACTLANE_OPCODE
#define WASM_EXTRACTLANE_OPCODE(opname, opcode, sig, asmjsop, imp, wat) WASM_LANE_OPCODE(opname, opcode, sig, imp, wat)
#endif

#ifndef WASM_REPLACELANE_OPCODE
#define WASM_REPLACELANE_OPCODE(opname, opcode, sig, asmjsop, imp, wat) WASM_LANE_OPCODE(opname, opcode, sig, imp, wat)
#endif

#ifndef WASM_SIMD_MEMREAD_OPCODE
#define WASM_SIMD_MEMREAD_OPCODE(opname, opcode, sig, asmjsop, viewtype, dataWidth, imp, wat) WASM_MEM_OPCODE(opname, opcode, sig, imp, wat)
#endif

#ifndef WASM_SIMD_MEMSTORE_OPCODE
#define WASM_SIMD_MEMSTORE_OPCODE(opname, opcode, sig, asmjsop, viewtype, dataWidth, imp, wat) WASM_MEM_OPCODE(opname, opcode, sig, imp, wat)
#endif

#define WASM_PREFIX_SIMD 0xfd
WASM_PREFIX(Simd, WASM_PREFIX_SIMD, Wasm::Simd::IsEnabled(), "WebAssembly Simd support is not enabled")

//SIMD Signatures
WASM_SIGNATURE(F_V128, 2, WasmTypes::F32, WasmTypes::V128)
WASM_SIGNATURE(I_V128, 2, WasmTypes::I32, WasmTypes::V128)
WASM_SIGNATURE(V128_I, 2, WasmTypes::V128, WasmTypes::I32)
WASM_SIGNATURE(V128_F, 2, WasmTypes::V128, WasmTypes::F32)
WASM_SIGNATURE(V128_L, 2, WasmTypes::V128, WasmTypes::I64)
WASM_SIGNATURE(V128_D, 2, WasmTypes::V128, WasmTypes::F64)
WASM_SIGNATURE(V128X3, 3, WasmTypes::V128, WasmTypes::V128, WasmTypes::V128)
WASM_SIGNATURE(V128_V128_I, 3, WasmTypes::V128, WasmTypes::V128, WasmTypes::I32)
WASM_SIGNATURE(V128_V128, 2, WasmTypes::V128, WasmTypes::V128)
WASM_SIGNATURE(L_V128, 2, WasmTypes::I64, WasmTypes::V128)
WASM_SIGNATURE(D_V128_I, 3, WasmTypes::F64, WasmTypes::V128, WasmTypes::I32)

#define __prefix (WASM_PREFIX_SIMD << 8)
WASM_SIMD_MEMREAD_OPCODE(V128Load, __prefix | 0x00, V128_I, Simd128_LdArr_F4, Js::ArrayBufferView::TYPE_FLOAT32, 16, true, "v128.load")
WASM_SIMD_MEMSTORE_OPCODE(V128Store, __prefix | 0x01, V128_I, Simd128_StArr_F4, Js::ArrayBufferView::TYPE_FLOAT32, 16, true, "v128.store")
WASM_MISC_OPCODE(V128Const, __prefix | 0x02, Limit, true, "v128.const")
WASM_MISC_OPCODE(V8X16Shuffle, __prefix | 0x03, Limit, true, "v8x16.shuffle")
WASM_UNARY__OPCODE(I16Splat, __prefix | 0x04, V128_I, Simd128_Splat_I16, true, "i8x16.splat")
WASM_EXTRACTLANE_OPCODE(I16ExtractLaneS, __prefix | 0x05, I_V128, Simd128_ExtractLane_I16, true, "i8x16.extract_lane_s")
WASM_EXTRACTLANE_OPCODE(I16ExtractLaneU, __prefix | 0x06, I_V128, Simd128_ExtractLane_U16, true, "i8x16.extract_lane_u")
WASM_REPLACELANE_OPCODE(I16ReplaceLane, __prefix | 0x07, V128_I, Simd128_ReplaceLane_I16, true, "i8x16.replace_lane")
WASM_UNARY__OPCODE(I8Splat, __prefix | 0x08, V128_I, Simd128_Splat_I8, true, "i16x8.splat")
WASM_EXTRACTLANE_OPCODE(I8ExtractLaneS, __prefix | 0x09, I_V128, Simd128_ExtractLane_I8, true, "i16x8.extract_lane_s")
WASM_EXTRACTLANE_OPCODE(I8ExtractLaneU, __prefix | 0x0a, I_V128, Simd128_ExtractLane_U8, true, "i16x8.extract_lane_u")
WASM_REPLACELANE_OPCODE(I8ReplaceLane, __prefix | 0x0b, V128_I, Simd128_ReplaceLane_I8, true, "i16x8.replace_lane")
WASM_UNARY__OPCODE(I4Splat, __prefix | 0x0c, V128_I, Simd128_Splat_I4, true, "i32x4.splat")
WASM_EXTRACTLANE_OPCODE(I4ExtractLane, __prefix | 0x0d, I_V128, Simd128_ExtractLane_I4, true, "i32x4.extract_lane")
WASM_REPLACELANE_OPCODE(I4ReplaceLane, __prefix | 0x0e, V128_I, Simd128_ReplaceLane_I4, true, "i32x4.replace_lane")
WASM_UNARY__OPCODE(I2Splat, __prefix | 0x0f, V128_L, Simd128_Splat_I2, true, "i64x2.splat")
WASM_EXTRACTLANE_OPCODE(I2ExtractLane, __prefix | 0x10, L_V128, Simd128_ExtractLane_I2, true, "i64x2.extract_lane")
WASM_REPLACELANE_OPCODE(I2ReplaceLane, __prefix | 0x11, V128_L, Simd128_ReplaceLane_I2, true, "i64x2.replace_lane")
WASM_UNARY__OPCODE(F4Splat, __prefix | 0x12, V128_F, Simd128_Splat_F4, true, "f32x4.splat")
WASM_EXTRACTLANE_OPCODE(F4ExtractLane, __prefix | 0x13, F_V128, Simd128_ExtractLane_F4, true, "f32x4.extract_lane")
WASM_REPLACELANE_OPCODE(F4ReplaceLane, __prefix | 0x14, V128_F, Simd128_ReplaceLane_F4, true, "f32x4.replace_lane")
WASM_UNARY__OPCODE(F2Splat, __prefix | 0x15, V128_D, Simd128_Splat_D2, true, "f64x2.splat")
WASM_BINARY_OPCODE(F2ExtractLane, __prefix | 0x16, D_V128_I, Simd128_ExtractLane_D2, true, "f64x2.extract_lane")
WASM_REPLACELANE_OPCODE(F2ReplaceLane, __prefix | 0x17, V128_D, Simd128_ReplaceLane_D2, true, "f64x2.replace_lane")
WASM_BINARY_OPCODE(I16Eq, __prefix | 0x18, V128X3, Simd128_Eq_I16, true, "i8x16.eq")
WASM_BINARY_OPCODE(I16Ne, __prefix | 0x19, V128X3, Simd128_Neq_I16, true, "i8x16.ne")
WASM_BINARY_OPCODE(I16LtS, __prefix | 0x1a, V128X3, Simd128_Lt_I16, true, "i8x16.lt_s")
WASM_BINARY_OPCODE(I16LtU, __prefix | 0x1b, V128X3, Simd128_Lt_U16, true, "i8x16.lt_u")
WASM_BINARY_OPCODE(I16GtS, __prefix | 0x1c, V128X3, Simd128_Gt_I16, true, "i8x16.gt_s")
WASM_BINARY_OPCODE(I16GtU, __prefix | 0x1d, V128X3, Simd128_Gt_U16, true, "i8x16.gt_u")
WASM_BINARY_OPCODE(I16LeS, __prefix | 0x1e, V128X3, Simd128_LtEq_I16, true, "i8x16.le_s")
WASM_BINARY_OPCODE(I16LeU, __prefix | 0x1f, V128X3, Simd128_LtEq_U16, true, "i8x16.le_u")
WASM_BINARY_OPCODE(I16GeS, __prefix | 0x20, V128X3, Simd128_GtEq_I16, true, "i8x16.ge_s")
WASM_BINARY_OPCODE(I16GeU, __prefix | 0x21, V128X3, Simd128_GtEq_U16, true, "i8x16.ge_u")
WASM_BINARY_OPCODE(I8Eq, __prefix | 0x22, V128X3, Simd128_Eq_I8, true, "i16x8.eq")
WASM_BINARY_OPCODE(I8Ne, __prefix | 0x23, V128X3, Simd128_Neq_I8, true, "i16x8.ne")
WASM_BINARY_OPCODE(I8LtS, __prefix | 0x24, V128X3, Simd128_Lt_I8, true, "i16x8.lt_s")
WASM_BINARY_OPCODE(I8LtU, __prefix | 0x25, V128X3, Simd128_Lt_U8, true, "i16x8.lt_u")
WASM_BINARY_OPCODE(I8GtS, __prefix | 0x26, V128X3, Simd128_Gt_I8, true, "i16x8.gt_s")
WASM_BINARY_OPCODE(I8GtU, __prefix | 0x27, V128X3, Simd128_Gt_U8, true, "i16x8.gt_u")
WASM_BINARY_OPCODE(I8LeS, __prefix | 0x28, V128X3, Simd128_LtEq_I8, true, "i16x8.le_s")
WASM_BINARY_OPCODE(I8LeU, __prefix | 0x29, V128X3, Simd128_LtEq_U8, true, "i16x8.le_u")
WASM_BINARY_OPCODE(I8GeS, __prefix | 0x2a, V128X3, Simd128_GtEq_I8, true, "i16x8.ge_s")
WASM_BINARY_OPCODE(I8GeU, __prefix | 0x2b, V128X3, Simd128_GtEq_U8, true, "i16x8.ge_u")
WASM_BINARY_OPCODE(I4Eq, __prefix | 0x2c, V128X3, Simd128_Eq_I4, true, "i32x4.eq")
WASM_BINARY_OPCODE(I4Ne, __prefix | 0x2d, V128X3, Simd128_Neq_I4, true, "i32x4.ne")
WASM_BINARY_OPCODE(I4LtS, __prefix | 0x2e, V128X3, Simd128_Lt_I4, true, "i32x4.lt_s")
WASM_BINARY_OPCODE(I4LtU, __prefix | 0x2f, V128X3, Simd128_Lt_U4, true, "i32x4.lt_u")
WASM_BINARY_OPCODE(I4GtS, __prefix | 0x30, V128X3, Simd128_Gt_I4, true, "i32x4.gt_s")
WASM_BINARY_OPCODE(I4GtU, __prefix | 0x31, V128X3, Simd128_Gt_U4, true, "i32x4.gt_u")
WASM_BINARY_OPCODE(I4LeS, __prefix | 0x32, V128X3, Simd128_LtEq_I4, true, "i32x4.le_s")
WASM_BINARY_OPCODE(I4LeU, __prefix | 0x33, V128X3, Simd128_LtEq_U4, true, "i32x4.le_u")
WASM_BINARY_OPCODE(I4GeS, __prefix | 0x34, V128X3, Simd128_GtEq_I4, true, "i32x4.ge_s")
WASM_BINARY_OPCODE(I4GeU, __prefix | 0x35, V128X3, Simd128_GtEq_U4, true, "i32x4.ge_u")
WASM_BINARY_OPCODE(F4Eq, __prefix | 0x40, V128X3, Simd128_Eq_F4, true, "f32x4.eq")
WASM_BINARY_OPCODE(F4Ne, __prefix | 0x41, V128X3, Simd128_Neq_F4, true, "f32x4.ne")
WASM_BINARY_OPCODE(F4Lt, __prefix | 0x42, V128X3, Simd128_Lt_F4, true, "f32x4.lt")
WASM_BINARY_OPCODE(F4Gt, __prefix | 0x43, V128X3, Simd128_Gt_F4, true, "f32x4.gt")
WASM_BINARY_OPCODE(F4Le, __prefix | 0x44, V128X3, Simd128_LtEq_F4, true, "f32x4.le")
WASM_BINARY_OPCODE(F4Ge, __prefix | 0x45, V128X3, Simd128_GtEq_F4, true, "f32x4.ge")
WASM_BINARY_OPCODE(F2Eq, __prefix | 0x46, V128X3, Simd128_Eq_D2, true, "f64x2.eq")
WASM_BINARY_OPCODE(F2Ne, __prefix | 0x47, V128X3, Simd128_Neq_D2, true, "f64x2.ne")
WASM_BINARY_OPCODE(F2Lt, __prefix | 0x48, V128X3, Simd128_Lt_D2, true, "f64x2.lt")
WASM_BINARY_OPCODE(F2Gt, __prefix | 0x49, V128X3, Simd128_Gt_D2, true, "f64x2.gt")
WASM_BINARY_OPCODE(F2Le, __prefix | 0x4a, V128X3, Simd128_LtEq_D2, true, "f64x2.le")
WASM_BINARY_OPCODE(F2Ge, __prefix | 0x4b, V128X3, Simd128_GtEq_D2, true, "f64x2.ge")
WASM_UNARY__OPCODE(V128Not, __prefix | 0x4c, V128_V128, Simd128_Not_I4, true, "v128.not")
WASM_BINARY_OPCODE(V128And, __prefix | 0x4d, V128X3, Simd128_And_I4, true, "v128.and")
WASM_BINARY_OPCODE(V128Or, __prefix | 0x4e, V128X3, Simd128_Or_I4, true, "v128.or")
WASM_BINARY_OPCODE(V128Xor, __prefix | 0x4f, V128X3, Simd128_Xor_I4, true, "v128.xor")
WASM_MISC_OPCODE(V128Bitselect, __prefix | 0x50, Limit, true, "v128.bitselect")
WASM_UNARY__OPCODE(I16Neg, __prefix | 0x51, V128_V128, Simd128_Neg_I16, true, "i8x16.neg")
WASM_UNARY__OPCODE(I16AnyTrue, __prefix | 0x52, I_V128, Simd128_AnyTrue_B16, true, "i8x16.any_true")
WASM_UNARY__OPCODE(I16AllTrue, __prefix | 0x53, I_V128, Simd128_AllTrue_B16, true, "i8x16.all_true")
WASM_BINARY_OPCODE(I16Shl, __prefix | 0x54, V128_V128_I, Simd128_ShLtByScalar_I16, true, "i8x16.shl")
WASM_BINARY_OPCODE(I16ShrS, __prefix | 0x55, V128_V128_I, Simd128_ShRtByScalar_I16, true, "i8x16.shr_s")
WASM_BINARY_OPCODE(I16ShrU, __prefix | 0x56, V128_V128_I, Simd128_ShRtByScalar_U16, true, "i8x16.shr_u")
WASM_BINARY_OPCODE(I16Add, __prefix | 0x57, V128X3, Simd128_Add_I16, true, "i8x16.add")
WASM_BINARY_OPCODE(I16AddSaturateS, __prefix | 0x58, V128X3, Simd128_AddSaturate_I16, true, "i8x16.add_saturate_s")
WASM_BINARY_OPCODE(I16AddSaturateU, __prefix | 0x59, V128X3, Simd128_AddSaturate_U16, true, "i8x16.add_saturate_u")
WASM_BINARY_OPCODE(I16Sub, __prefix | 0x5a, V128X3, Simd128_Sub_I16, true, "i8x16.sub")
WASM_BINARY_OPCODE(I16SubSaturateS, __prefix | 0x5b, V128X3, Simd128_SubSaturate_I16, true, "i8x16.sub_saturate_s")
WASM_BINARY_OPCODE(I16SubSaturateU, __prefix | 0x5c, V128X3, Simd128_SubSaturate_U16, true, "i8x16.sub_saturate_u")
WASM_BINARY_OPCODE(I16Mul, __prefix | 0x5d, V128X3, Simd128_Mul_I16, true, "i8x16.mul")
WASM_UNARY__OPCODE(I8Neg, __prefix | 0x62, V128_V128, Simd128_Neg_I8, true, "i16x8.neg")
WASM_UNARY__OPCODE(I8AnyTrue, __prefix | 0x63, I_V128, Simd128_AnyTrue_B8, true, "i16x8.any_true")
WASM_UNARY__OPCODE(I8AllTrue, __prefix | 0x64, I_V128, Simd128_AllTrue_B8, true, "i16x8.all_true")
WASM_BINARY_OPCODE(I8Shl, __prefix | 0x65, V128_V128_I, Simd128_ShLtByScalar_I8, true, "i16x8.shl")
WASM_BINARY_OPCODE(I8ShrS, __prefix | 0x66, V128_V128_I, Simd128_ShRtByScalar_I8, true, "i16x8.shr_s")
WASM_BINARY_OPCODE(I8ShrU, __prefix | 0x67, V128_V128_I, Simd128_ShRtByScalar_U8, true, "i16x8.shr_u")
WASM_BINARY_OPCODE(I8Add, __prefix | 0x68, V128X3, Simd128_Add_I8, true, "i16x8.add")
WASM_BINARY_OPCODE(I8AddSaturateS, __prefix | 0x69, V128X3, Simd128_AddSaturate_I8, true, "i16x8.add_saturate_s")
WASM_BINARY_OPCODE(I8AddSaturateU, __prefix | 0x6a, V128X3, Simd128_AddSaturate_U8, true, "i16x8.add_saturate_u")
WASM_BINARY_OPCODE(I8Sub, __prefix | 0x6b, V128X3, Simd128_Sub_I8, true, "i16x8.sub")
WASM_BINARY_OPCODE(I8SubSaturateS, __prefix | 0x6c, V128X3, Simd128_SubSaturate_I8, true, "i16x8.sub_saturate_s")
WASM_BINARY_OPCODE(I8SubSaturateU, __prefix | 0x6d, V128X3, Simd128_SubSaturate_U8, true, "i16x8.sub_saturate_u")
WASM_BINARY_OPCODE(I8Mul, __prefix | 0x6e, V128X3, Simd128_Mul_I8, true, "i16x8.mul")
WASM_UNARY__OPCODE(I4Neg, __prefix | 0x73, V128_V128, Simd128_Neg_I4, true, "i32x4.neg")
WASM_UNARY__OPCODE(I4AnyTrue, __prefix | 0x74, I_V128, Simd128_AnyTrue_B4, true, "i32x4.any_true")
WASM_UNARY__OPCODE(I4AllTrue, __prefix | 0x75, I_V128, Simd128_AllTrue_B4, true, "i32x4.all_true")
WASM_BINARY_OPCODE(I4Shl, __prefix | 0x76, V128_V128_I, Simd128_ShLtByScalar_I4, true, "i32x4.shl")
WASM_BINARY_OPCODE(I4ShrS, __prefix | 0x77, V128_V128_I, Simd128_ShRtByScalar_I4, true, "i32x4.shr_s")
WASM_BINARY_OPCODE(I4ShrU, __prefix | 0x78, V128_V128_I, Simd128_ShRtByScalar_U4, true, "i32x4.shr_u")
WASM_BINARY_OPCODE(I4Add, __prefix | 0x79, V128X3, Simd128_Add_I4, true, "i32x4.add")
WASM_BINARY_OPCODE(I4Sub, __prefix | 0x7c, V128X3, Simd128_Sub_I4, true, "i32x4.sub")
WASM_BINARY_OPCODE(I4Mul, __prefix | 0x7f, V128X3, Simd128_Mul_I4, true, "i32x4.mul")
WASM_UNARY__OPCODE(I2Neg, __prefix | 0x84, V128_V128, Simd128_Neg_I2, true, "i64x2.neg")
WASM_UNARY__OPCODE(I2AnyTrue, __prefix | 0x85, I_V128, Simd128_AnyTrue_B2, true, "i64x2.any_true")
WASM_UNARY__OPCODE(I2AllTrue, __prefix | 0x86, I_V128, Simd128_AllTrue_B2, true, "i64x2.all_true")
WASM_BINARY_OPCODE(I2Shl, __prefix | 0x87, V128_V128_I, Simd128_ShLtByScalar_I2, true, "i64x2.shl")
WASM_BINARY_OPCODE(I2ShrS, __prefix | 0x88, V128_V128_I, Simd128_ShRtByScalar_I2, true, "i64x2.shr_s")
WASM_BINARY_OPCODE(I2ShrU, __prefix | 0x89, V128_V128_I, Simd128_ShRtByScalar_U2, true, "i64x2.shr_u")
WASM_BINARY_OPCODE(I2Add, __prefix | 0x8a, V128X3, Simd128_Add_I2, true, "i64x2.add")
WASM_BINARY_OPCODE(I2Sub, __prefix | 0x8d, V128X3, Simd128_Sub_I2, true, "i64x2.sub")
WASM_UNARY__OPCODE(F4Abs, __prefix | 0x95, V128_V128, Simd128_Abs_F4, true, "f32x4.abs")
WASM_UNARY__OPCODE(F4Neg, __prefix | 0x96, V128_V128, Simd128_Neg_F4, true, "f32x4.neg")
WASM_UNARY__OPCODE(F4Sqrt, __prefix | 0x97, V128_V128, Simd128_Sqrt_F4, true, "f32x4.sqrt")
WASM_BINARY_OPCODE(F4Add, __prefix | 0x9a, V128X3, Simd128_Add_F4, true, "f32x4.add")
WASM_BINARY_OPCODE(F4Sub, __prefix | 0x9b, V128X3, Simd128_Sub_F4, true, "f32x4.sub")
WASM_BINARY_OPCODE(F4Mul, __prefix | 0x9c, V128X3, Simd128_Mul_F4, true, "f32x4.mul")
WASM_BINARY_OPCODE(F4Div, __prefix | 0x9d, V128X3, Simd128_Div_F4, true, "f32x4.div")
WASM_BINARY_OPCODE(F4Min, __prefix | 0x9e, V128X3, Simd128_Min_F4, true, "f32x4.min")
WASM_BINARY_OPCODE(F4Max, __prefix | 0x9f, V128X3, Simd128_Max_F4, true, "f32x4.max")
WASM_UNARY__OPCODE(F2Abs, __prefix | 0xa0, V128_V128, Simd128_Abs_D2, true, "f64x2.abs")
WASM_UNARY__OPCODE(F2Neg, __prefix | 0xa1, V128_V128, Simd128_Neg_D2, true, "f64x2.neg")
WASM_UNARY__OPCODE(F2Sqrt, __prefix | 0xa2, V128_V128, Simd128_Sqrt_D2, true, "f64x2.sqrt")
WASM_BINARY_OPCODE(F2Add, __prefix | 0xa5, V128X3, Simd128_Add_D2, true, "f64x2.add")
WASM_BINARY_OPCODE(F2Sub, __prefix | 0xa6, V128X3, Simd128_Sub_D2, true, "f64x2.sub")
WASM_BINARY_OPCODE(F2Mul, __prefix | 0xa7, V128X3, Simd128_Mul_D2, true, "f64x2.mul")
WASM_BINARY_OPCODE(F2Div, __prefix | 0xa8, V128X3, Simd128_Div_D2, true, "f64x2.div")
WASM_BINARY_OPCODE(F2Min, __prefix | 0xa9, V128X3, Simd128_Min_D2, true, "f64x2.min")
WASM_BINARY_OPCODE(F2Max, __prefix | 0xaa, V128X3, Simd128_Max_D2, true, "f64x2.max")
WASM_UNARY__OPCODE(I4TruncS, __prefix | 0xab, V128_V128, Simd128_FromFloat32x4_I4, true, "i32x4.trunc_s/f32x4:sat")
WASM_UNARY__OPCODE(I4TruncU, __prefix | 0xac, V128_V128, Simd128_FromFloat32x4_U4, true, "i32x4.trunc_u/f32x4:sat")
WASM_UNARY__OPCODE(I2TruncS, __prefix | 0xad, V128_V128, Simd128_FromFloat64x2_I2, true, "i64x2.trunc_s/f64x2:sat")
WASM_UNARY__OPCODE(I2TruncU, __prefix | 0xae, V128_V128, Simd128_FromFloat64x2_U2, true, "i64x2.trunc_u/f64x2:sat")
WASM_UNARY__OPCODE(F4ConvertS, __prefix | 0xaf, V128_V128, Simd128_FromInt32x4_F4, true, "f32x4.convert_s/i32x4")
WASM_UNARY__OPCODE(F4ConvertU, __prefix | 0xb0, V128_V128, Simd128_FromUint32x4_F4, true, "f32x4.convert_u/i32x4")
WASM_UNARY__OPCODE(F2ConvertS, __prefix | 0xb1, V128_V128, Simd128_FromInt64x2_D2, true, "f64x2.convert_s/i64x2")
WASM_UNARY__OPCODE(F2ConvertU, __prefix | 0xb2, V128_V128, Simd128_FromUint64x2_D2, true, "f64x2.convert_u/i64x2")

#undef __prefix
#undef WASM_PREFIX_SIMD
#undef WASM_SIMD_BUILD_OPCODE
#undef WASM_LANE_OPCODE
#undef WASM_EXTRACTLANE_OPCODE
#undef WASM_SIMD_MEMREAD_OPCODE
#undef WASM_SIMD_MEMSTORE_OPCODE
#undef WASM_REPLACELANE_OPCODE
