//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

//
//review:  shouldn't we use _PAGESIZE_ from heap.h instead of hardcoded 8KB.
//
const int PAGESIZE = 2 * 0x1000;
const IRType TyMachReg = TyInt64;
const IRType TyMachPtr = TyUint64;
const IRType TyMachDouble = TyFloat64;
const IRType TyMachSimd128F4 = TySimd128F4;
const IRType TyMachSimd128I4 = TySimd128I4;
const IRType TyMachSimd128I8 = TySimd128I8;
const IRType TyMachSimd128I16 = TySimd128I16;
const IRType TyMachSimd128U4 = TySimd128U4;
const IRType TyMachSimd128U8 = TySimd128U8;
const IRType TyMachSimd128U16 = TySimd128U16;
const IRType TyMachSimd128B4 = TySimd128B4;
const IRType TyMachSimd128B8 = TySimd128B8;
const IRType TyMachSimd128B16 = TySimd128B16;
const IRType TyMachSimd128D2 = TySimd128D2;
const IRType TyMachSimd128I2 = TySimd128I2;

const DWORD EMIT_BUFFER_ALIGNMENT = 16;
const DWORD INSTR_ALIGNMENT = 1;
