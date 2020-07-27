//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#ifdef ASMJS_PLAT
namespace OpCodeAttrAsmJs
{
    // False if the opcode results in jump to end of the function and there cannot be fallthrough.
    bool HasFallThrough(Js::OpCodeAsmJs opcode);
    // True if the opcode has a small/large layout
    bool HasMultiSizeLayout(Js::OpCodeAsmJs opcode);
    bool HasProfiledOp(Js::OpCodeAsmJs opcode);
    bool IsProfiledOp(Js::OpCodeAsmJs opcode);
};
#endif
