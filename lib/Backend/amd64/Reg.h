//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#define RA_DONTALLOCATE     0x1
#define RA_CALLEESAVE       0x2
#define RA_CALLERSAVE       0x4

#define RA_BYTEABLE         0x8


enum RegNum : BYTE {
#define REGDAT(Name, Listing,    Encode,    Type,    BitVec)  Reg ## Name,
#include "RegList.h"
    RegNumCount,

// alias RegArg0, RegArg1, ...
#define REG_INT_ARG(Index, Name)  RegArg ## Index = Reg ## Name,
#include "RegList.h"

// alias RegXmmArg0, RegXmmArg1, ...
#define REG_XMM_ARG(Index, Name)  RegXmmArg ## Index = Reg ## Name,
#include "RegList.h"
};

// IntArgRegsCount
enum _IntArgRegs {
#define REG_INT_ARG(Index, Name)  _RegArg ## Index,
#include "RegList.h"
    IntArgRegsCount
};

// XmmArgRegsCount
enum _XmmArgRegs {
#define REG_XMM_ARG(Index, Name)  _RegXmmArg ## Index,
#include "RegList.h"
    XmmArgRegsCount
};

#define REGNUM_ISXMMXREG(r) ((r) >= RegXMM0 && (r) <= RegXMM15)
#define FIRST_XMM_REG RegXMM0
#define FIRST_FLOAT_REG RegXMM0
#define FIRST_FLOAT_ARG_REG RegXMM0
#define XMM_REGCOUNT 16
#define CATCH_OBJ_REG RegRAX

#define FOREACH_REG(reg) \
        for (RegNum reg = (RegNum)(RegNOREG+1); reg != RegNumCount; reg = (RegNum)(reg+1))
#define NEXT_REG

#define FOREACH_INT_REG(reg) \
        for (RegNum reg = (RegNum)(RegNOREG+1); reg != FIRST_FLOAT_REG; reg = (RegNum)(reg+1))
#define NEXT_INT_REG

#define FOREACH_FLOAT_REG(reg) \
        for (RegNum reg = FIRST_FLOAT_REG; reg != RegNumCount; reg = (RegNum)(reg+1))
#define NEXT_FLOAT_REG

