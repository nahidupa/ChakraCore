//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

//
// Contains constants and tables used by the encoder.
//

// Define the encodings of the second byte in instructions. This value is
// or'ed in with the mod r/m fields to build a constant reg field.
//
// mod reg r/m  00reg000   reg 0->7.
#define RNON    0x00
#define R000    0x00
#define R001    0x08
#define R010    0x10
#define R011    0x18
#define R100    0x20
#define R101    0x28
#define R110    0x30
#define R111    0x38

/* values for the opcode dope vector */
#define DFLT    0x1      /* floating point instructions */
#define DSETCC  0x2      /* sets one or more condition codes */
#define DUSECC  0x4      /* uses one or more condition codes */
#define DNOMF   0x8      /* floating point instruction, no "mf" bits */
#define DREGSRC 0x10     // the instruction takes only register sources
#define DNO16   0x20     /* 16 bit op size override not legal */
#define DOPEQ   0x40     /* DST and SRC1 must be the same */
#define DUSEPRE 0x80     /* this instruction can have use a prefix instruction*/
#define DISPRE  0x100    /* this is a 'prefix' instruction */
#define DFWAIT  0x200    /* generate a WAIT before this instruction */
#define DDST    0x800    /* get the first operand from the DST list */
#define DCOMMOP 0x1000   /* opcode is commutative */
#define D66EX   0x2000   // Optional 0x66 for WNI/MNI 128-bit extended MMX opcodes
#define DSSE    0x4000   /* Instruction operates on XMM registers but not AVX */
#define DF3     0x8000   /* the 0xF3 0x0F style KNI opcodes */
#define DMOV    0x10000  /* Instruction is a MOV or a synonym for MOV (e.g., MOV_TRUNC) */
#define D66     0x100000 // 0x66 0x0F style WNI form (usually 128-bit DP FP)
#define DF2     0x200000 /* 0xF2 0x0F style WNI form (usually 64-bit DP FP) */
#define DREXSRC  0x400000 /* Use src1's size to generate REX byte */
#define DLOCK   0x800000 /* Prefix the instruction with the lock byte (0xf0) */

// 2nd 3 bits is options
#define SBIT 0x20
#define DBIT 0x40
#define WBIT 0x80       // applies to MODRM, IMM, NO, SHFT, AX_IM
#define FAC  0x20       // applies to FMODRM
#define FINT 0x40       // applies to FMODRM, FUMODRM

// values for the "form" field
// first 5 bits is basic form
#define FORM_MASK 0x1F
#define INVALID   0
#define MODRM     1    // MODRM
#define IMM       2    // long immediate
#define AX_MEM    3    // EAX, mem
#define SH_IM     4    // short immediate
#define SHIMR     5    // short immediate, reg
#define SH_REG    6
#define NO        7
#define LABREL1   8
#define LABREL2   9    // near jmp/call
#define SPECIAL   13   // requires special handling
#define FMODRM    14
#define FUMODRM   15
#define FNSMODRM  16
#define NO2       17
#define SHFT      18
#define AXMODRM   19
#define AX_IM     20   // EAX, immediate
#define FMODRMR   21
// Forms
#define TEMPLATE_FORM_BINOP      {AX_IM+SBIT+WBIT,IMM+SBIT+WBIT, MODRM+DBIT+WBIT, INVALID}
#define TEMPLATE_FORM_CALL       {LABREL2, MODRM, INVALID}
#define TEMPLATE_FORM_INCDEC     {MODRM+WBIT, INVALID}
#define TEMPLATE_FORM_JMP        {LABREL1, LABREL2, MODRM, INVALID}
#define TEMPLATE_FORM_JCC        {/* LABREL1, */ LABREL2, INVALID}
#define TEMPLATE_FORM_MODRM      {MODRM, INVALID}
#define TEMPLATE_FORM_MODRMW     {MODRM+WBIT, INVALID}
#define TEMPLATE_FORM_MOV        {AX_MEM+WBIT, SHIMR, IMM+WBIT, MODRM+WBIT+DBIT, INVALID}
#define TEMPLATE_FORM_MULDIV     {AXMODRM+WBIT, INVALID}
#define TEMPLATE_FORM_NO         {NO, INVALID}
#define TEMPLATE_FORM_PSHPOP     {SH_REG, SH_IM+SBIT, MODRM, INVALID}
#define TEMPLATE_FORM_SHIFT      {SHFT+WBIT, INVALID}
#define TEMPLATE_FORM_SPECIAL    {SPECIAL, INVALID}
#define TEMPLATE_FORM_SPMOD      {SPECIAL, MODRM, INVALID}
#define TEMPLATE_FORM_TEST       {AX_IM+WBIT, IMM+WBIT, MODRM+WBIT, INVALID}
#define TEMPLATE_FORM_XCHG       {SPECIAL, MODRM+WBIT, INVALID}

// LeadIn  (commented ones are not used in Chakra)
#define OLB_NONE 0x0    // opcode is one byte: no lead-in bytes
#define OLB_0F   0x1    // opcode is 0F xx or VEX.mmmmm = 00001
//#define OLB_0F01 0x2    // opcode is 0F 01 xx (no VEX encoding)
//#define OLB_0F38 0x3    // opcode is 0F 38 xx or VEX.mmmmm = 00010
#define OLB_0F3A 0x4    // opcode is 0F 3A xx or VEX.mmmmm = 00011
//#define OLB_XOP8 0x8    // XOP prefix with mmmmm = 01000
//#define OLB_XOP9 0x9    // XOP prefix with mmmmm = 01001
//#define OLB_XOPA 0xA    // XOP prefix with mmmmm = 01010

enum Forms : BYTE
{
    FORM_BINOP,
    FORM_CALL,
    FORM_FILD,
    FORM_FLD,
    FORM_INCDEC,
    FORM_JMP,
    FORM_JCC,
    FORM_MODRM,
    FORM_MODRMW,
    FORM_MOV,
    FORM_MULDIV,
    FORM_NO,
    FORM_PSHPOP,
    FORM_SHIFT,
    FORM_SPECIAL,
    FORM_SPMOD,
    FORM_TEST,
    FORM_TEST_AH,
    FORM_XCHG
};

// OpBytes
#define OPBYTE_ADD      {0x4, 0x80, 0x0}         // binop, byte2=0x0

#define OPBYTE_ADDPD    {0x58}                  // modrm
#define OPBYTE_ADDPS    {0x58}                  // modrm

#define OPBYTE_ADDSD    {0x58}                   // modrm
#define OPBYTE_ADDSS    {0x58}                   // modrm

#define OPBYTE_ANDNPD   {0x55}                  // modrm
#define OPBYTE_ANDNPS   {0x55}                  // modrm

#define OPBYTE_ANDPD    {0x54}                   // modrm
#define OPBYTE_ANDPS    {0x54}                   // modrm
#define OPBYTE_SUB      {0x2c, 0x80, 0x28}       // binop, byte2=0x5
#define OPBYTE_AND      {0x24, 0x80, 0x20}       // binop, byte2=0x4
#define OPBYTE_BSF      {0xbc}                   // modrm
#define OPBYTE_BSR      {0xbd}                   // modrm
#define OPBYTE_BT       {0xba, 0xa3}             // special, modrm
#define OPBYTE_BTR      {0xba, 0xb3}             // special, modrm
#define OPBYTE_BTS      {0xba, 0xab}             // special, modrm
#define OPBYTE_CALL     {0xe8, 0xff}             // call, byte2=2
#define OPBYTE_CDQ      {0x99}                   // no
#define OPBYTE_CQO      {0x99}                   // no
#define OPBYTE_CMOVA    {0x47}                   // modrm
#define OPBYTE_CMOVAE   {0x43}                   // modrm
#define OPBYTE_CMOVB    {0x42}                   // modrm
#define OPBYTE_CMOVBE   {0x46}                   // modrm
#define OPBYTE_CMOVE    {0x44}                   // modrm
#define OPBYTE_CMOVG    {0x4F}                   // modrm
#define OPBYTE_CMOVGE   {0x4D}                   // modrm
#define OPBYTE_CMOVL    {0x4C}                   // modrm
#define OPBYTE_CMOVLE   {0x4E}                   // modrm
#define OPBYTE_CMOVNE   {0x45}                   // modrm
#define OPBYTE_CMOVNO   {0x41}                   // modrm
#define OPBYTE_CMOVNP   {0x4B}                   // modrm
#define OPBYTE_CMOVNS   {0x49}                   // modrm
#define OPBYTE_CMOVO    {0x40}                   // modrm
#define OPBYTE_CMOVP    {0x4A}                   // modrm
#define OPBYTE_CMOVS    {0x48}                   // modrm
#define OPBYTE_CMP      {0x3c, 0x80, 0x38}       // binop, byte2=7

#define OPBYTE_CMPPD    {0xc2}                  // modrm
#define OPBYTE_CMPPS    {0xc2}                  // modrm
#define OPBYTE_CMPXCHG8B {0xC7,0xC7}            // special, modrm

#define OPBYTE_COMISD   {0x2F}                   // modrm
#define OPBYTE_COMISS   {0x2F}                   // modrm
#define OPBYTE_CVTDQ2PD {0xE6}                   // modrm

#define OPBYTE_CVTDQ2PS {0x5B}                  // special (modrm)
#define OPBYTE_CVTPD2PS {0x5A}                  // modrm
#define OPBYTE_CVTTPS2DQ {0x5B}                  // special (modrm)
#define OPBYTE_CVTTPD2DQ {0xE6}                 // modrm

#define OPBYTE_CVTSD2SI {0x2D}                   // modrm
#define OPBYTE_CVTSI2SD {0x2A}                   // modrm
#define OPBYTE_CVTSI2SS {0x2A}                   // modrm
#define OPBYTE_CVTPS2PD {0x5A}                   // modrm
#define OPBYTE_CVTSD2SS {0x5A}                   // modrm
#define OPBYTE_CVTSS2SD {0x5A}                   // modrm
#define OPBYTE_CVTSS2SI {0x2D}                   // modrm
#define OPBYTE_CVTTSD2SI {0x2C}                  // modrm
#define OPBYTE_CVTTSS2SI {0x2C}                  // modrm
#define OPBYTE_DEC      {0xfe}                   // incdec, byte2=1
#define OPBYTE_DIV      {0xf6}                   // imul, byte2=6

#define OPBYTE_DIVPS    {0x5E}                  // modrm
#define OPBYTE_DIVPD    {0x5E}                  // modrm

#define OPBYTE_DIVSD    {0x5E}                   // modrm
#define OPBYTE_DIVSS    {0x5E}                   // modrm
#define OPBYTE_IDIV     {0xf6}                   // imul, byte2=7
#define OPBYTE_IMUL     {0xf6}                   // imul, byte2=5
#define OPBYTE_IMUL2    {0x69, 0xaf}             // special, modrm
#define OPBYTE_INC      {0xfe}                   // incdec, byte2=0
#define OPBYTE_INT      {0xcc}                   // special
#define OPBYTE_JO       {0x80}                   // jo  (rel32 only)
#define OPBYTE_JNO      {0x81}                   // jno (rel32 only)
#define OPBYTE_JB       {0x82}                   // jb  (rel32 only)
#define OPBYTE_JAE      {0x83}                   // jae (rel32 only)
#define OPBYTE_JEQ      {0x84}                   // jeq (rel32 only)
#define OPBYTE_JNE      {0x85}                   // jne (rel32 only)
#define OPBYTE_JBE      {0x86}                   // jbe (rel32 only)
#define OPBYTE_JA       {0x87}                   // ja  (rel32 only)
#define OPBYTE_JSB      {0x88}                   // jsb (rel32 only)
#define OPBYTE_JNSB     {0x89}                   // jnsb (rel32 only)
#define OPBYTE_JP       {0x8a}                   // jp  (rel32 only)
#define OPBYTE_JNP      {0x8b}                   // jnp (rel32 only)
#define OPBYTE_JLT      {0x8c}                   // jlt (rel32 only)
#define OPBYTE_JGE      {0x8d}                   // jge (rel32 only)
#define OPBYTE_JLE      {0x8e}                   // jle (rel32 only)
#define OPBYTE_JGT      {0x8f}                   // jgt (rel32 only)
#define OPBYTE_JMP      {0xeb, 0xe9, 0xff}       // jmp, byte2=4
#define OPBYTE_LEA      {0x8d}                   // modrm

#define OPBYTE_MAXPD    {0x5f}                  // modrm
#define OPBYTE_MAXPS    {0x5f}                  // modrm
#define OPBYTE_MINPD    {0x5d}                  // modrm
#define OPBYTE_MINPS    {0x5d}                  // modrm

#define OPBYTE_TZCNT    {0xbc}                   // modrm
#define OPBYTE_LZCNT    {0xbd}                   // modrm

#define OPBYTE_OR       {0x0c, 0x80, 0x08}       // binop, byte2=0x1
#define OPBYTE_MOV      {0xa0, 0xb0, 0xc6, 0x88} // mov, byte2=0
#define OPBYTE_MOVD     {0x6e}                   // special

#define OPBYTE_MOVHLPS  {0x12}                  // modrm
#define OPBYTE_MOVHPD   {0x16}                  // special
#define OPBYTE_MOVLPD   {0x12}                  // special
#define OPBYTE_MOVLHPS  {0x16}                  // modrm
#define OPBYTE_MOVMSKPD {0x50}                  // modrm
#define OPBYTE_MOVMSKPS {0x50}                  // modrm

#define OPBYTE_MOVQ     {0x6f}                   // special

#define OPBYTE_MOVSD    {0x10}                   // modrm
#define OPBYTE_MOVSS    {0x10}                   // special, modrm
#define OPBYTE_MOVAPD   {0x28}                   // special, modrm
#define OPBYTE_MOVAPS   {0x28}                   // special, modrm
#define OPBYTE_MOVSX    {0xbe}                   // modrm
#define OPBYTE_MOVSXW   {0xbf}                   // modrm

#define OPBYTE_MOVUPS   {0x10}                  // special

#define OPBYTE_MOVZX    {0xb6}                   // modrm
#define OPBYTE_MOVZXW   {0xb7}                   // modrm
#define OPBYTE_MOVSXD   {0x63}                   // MODRM

#define OPBYTE_MULPD    {0x59}                  // modrm
#define OPBYTE_MULPS    {0x59}                  // modrm

#define OPBYTE_MULSD    {0x59}                   // modrm
#define OPBYTE_MULSS    {0x59}                   // modrm
#define OPBYTE_NEG      {0xf6}                   // modrm, byte2=3
#define OPBYTE_NOP      {0x90}                   // special
#define OPBYTE_NOT      {0xf6}                   // modrm, byte2=2

#define OPBYTE_ORPS     {0x56}                  // modrm
#define OPBYTE_PADDB    {0xfc}                  // modrm
#define OPBYTE_PADDD    {0xfe}                  // modrm
#define OPBYTE_PADDQ    {0xd4}                  // modrm
#define OPBYTE_PADDW    {0xfd}                  // modrm
#define OPBYTE_PADDSB   {0xec}                  // modrm
#define OPBYTE_PADDSW   {0xed}                  // modrm
#define OPBYTE_PADDUSB  {0xdc}                  // modrm
#define OPBYTE_PADDUSW  {0xdd}                  // modrm
#define OPBYTE_PAND     {0xdb}                  // modrm
#define OPBYTE_PANDN    {0xdf}                  // modrm
#define OPBYTE_PCMPEQB  {0x74}                  // modrm
#define OPBYTE_PCMPEQD  {0x76}                  // modrm
#define OPBYTE_PCMPEQW  {0x75}                  // modrm
#define OPBYTE_PCMPGTB  {0x64}                  // modrm
#define OPBYTE_PCMPGTD  {0x66}                  // modrm
#define OPBYTE_PCMPGTW  {0x65}                  // modrm
#define OPBYTE_PMAXSW   {0xee}                  // modrm
#define OPBYTE_PMAXUB   {0xde}                  // modrm
#define OPBYTE_PMINSW   {0xea}                  // modrm
#define OPBYTE_PMINUB   {0xda}                  // modrm
#define OPBYTE_PMOVMSKB {0xd7}                  // modrm
#define OPBYTE_PMULLW   {0xd5}                  // modrm
#define OPBYTE_PMULUDQ  {0xf4}                  // modrm
#define OPBYTE_PMULLW   {0xd5}                  // modrm

#define OPBYTE_POP      {0x58, 0, 0x8f}          // pshpop, byte2=0 immed not legal

#define OPBYTE_POR      {0xeb}                  // modrm
#define OPBYTE_POPCNT   {0xB8}                  // modrm
#define OPBYTE_PSHUFD   {0x70}                  // special
#define OPBYTE_PEXTRW   {0xc5}                  // special
#define OPBYTE_PEXTRD   {0x16}                  // special
#define OPBYTE_PEXTRQ   {0x16}                  // special
#define OPBYTE_PINSRW   {0xc4}                  // special
#define OPBYTE_PINSRD   {0x22}                  // special
#define OPBYTE_PINSRQ   {0x22}                  // special
#define OPBYTE_PSLLDQ   {0x73}                  // mmxshift
#define OPBYTE_PSRLDQ   {0x73}                  // mmxshift

#define OPBYTE_PSLLW    {0x71}                  // mmx lane shift
#define OPBYTE_PSLLD    {0x72}                  // mmx lane shift
#define OPBYTE_PSLLQ    {0xf3}                  // mmx lane shift
#define OPBYTE_PSRAW    {0x71}                  // mmx lane shift
#define OPBYTE_PSRAD    {0x72}                  // mmx lane shift
#define OPBYTE_PSRLW    {0x71}                  // mmx lane shift
#define OPBYTE_PSRLD    {0x72}                  // mmx lane shift
#define OPBYTE_PSRLQ    {0xd3}                  // mmx lane shift

#define OPBYTE_PSUBB    {0xf8}                  // modrm
#define OPBYTE_PSUBD    {0xfa}                  // modrm
#define OPBYTE_PSUBQ    {0xfb}                  // modrm
#define OPBYTE_PSUBW    {0xf9}                  // modrm
#define OPBYTE_PSUBSB   {0xe8}                  // modrm
#define OPBYTE_PSUBSW   {0xe9}                  // modrm
#define OPBYTE_PSUBUSB  {0xd8}                  // modrm
#define OPBYTE_PSUBUSW  {0xd9}                  // modrm
#define OPBYTE_PUNPCKLBW {0x60}                 // modrm
#define OPBYTE_PUNPCKLDQ {0x62}                 // modrm
#define OPBYTE_PUNPCKLWD {0x61}                 // modrm

#define OPBYTE_PUSH     {0x50, 0x68, 0xff}       // pshpop, byte2=6

#define OPBYTE_PXOR     {0xef}                  // modrm

#define OPBYTE_RET      {0xc2}                   // special
#define OPBYTE_ROL      {0xc0, 0xd2}             // shift, byte2=0
#define OPBYTE_ROR      {0xc0, 0xd2}             // shift, byte2=1
#define OPBYTE_ROUNDSD  {0x0B}                   // modrm
#define OPBYTE_ROUNDSS  {0x0A}                   // modrm
#define OPBYTE_SAR      {0xc0, 0xd2}             // shift, byte2=7
#define OPBYTE_SBB      {0x1c, 0x80, 0x18}       // binop, byte2=3
#define OPBYTE_SETO     {0x90}                   // modrm
#define OPBYTE_SETNO    {0x91}                   // modrm
#define OPBYTE_SETB     {0x92}                   // modrm
#define OPBYTE_SETAE    {0x93}                   // modrm
#define OPBYTE_SETE     {0x94}                   // modrm
#define OPBYTE_SETNE    {0x95}                   // modrm
#define OPBYTE_SETBE    {0x96}                   // modrm
#define OPBYTE_SETA     {0x97}                   // modrm
#define OPBYTE_SETS     {0x98}                   // modrm
#define OPBYTE_SETNS    {0x99}                   // modrm
#define OPBYTE_SETP     {0x9a}                   // modrm
#define OPBYTE_SETNP    {0x9b}                   // modrm
#define OPBYTE_SETL     {0x9c}                   // modrm
#define OPBYTE_SETGE    {0x9d}                   // modrm
#define OPBYTE_SETLE    {0x9e}                   // modrm
#define OPBYTE_SETG     {0x9f}                   // modrm
#define OPBYTE_SHL      {0xc0, 0xd2}             // shift, byte2=4
#define OPBYTE_SHR      {0xc0, 0xd2}             // shift, byte2=5

#define OPBYTE_SHUFPD   {0xc6}                  // special
#define OPBYTE_SHUFPS   {0xc6}                  // special

#define OPBYTE_SQRTSD   {0x51}                   // modrm

#define OPBYTE_SQRTPD   {0x51}                  // modrm
#define OPBYTE_SQRTPS   {0x51}                  // modrm

#define OPBYTE_SQRTSS   {0x51}                   // modrm
#define OPBYTE_SHLD     {0xa4}                   // special
#define OPBYTE_SUB      {0x2c, 0x80, 0x28}       // binop, byte2=5

#define OPBYTE_SUBPD    {0x5C}                  // modrm
#define OPBYTE_SUBPS    {0x5C}                  // modrm

#define OPBYTE_SUBSD    {0x5C}                   // modrm
#define OPBYTE_SUBSS    {0x5C}                   // modrm
#define OPBYTE_TEST     {0xa8, 0xf6, 0x84}       // test, byte2=0
#define OPBYTE_UCOMISD  {0x2e}                   // modrm
#define OPBYTE_UCOMISS  {0x2E}                   // modrm
#define OPBYTE_XCHG     {0x90, 0x86}             // xchg
#define OPBYTE_XOR      {0x34, 0x80, 0x30}       // binop, byte2=0x6
#define OPBYTE_XORPS    {0x57}                   // modrm
