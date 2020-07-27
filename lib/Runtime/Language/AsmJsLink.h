//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#ifdef ASMJS_PLAT
namespace Js{

    class ASMLink{
    public:
        static bool CheckParams(ScriptContext* scriptContext, AsmJsModuleInfo* info , const Var stdlib, const Var foreign, Var bufferView);
    private:
        static bool CheckArrayBuffer(ScriptContext* scriptContext, const Var bufferView, const AsmJsModuleInfo* info);
        static bool CheckStdLib(ScriptContext* scriptContext, const AsmJsModuleInfo* info, const Var stdlib);
        static bool CheckFFI(ScriptContext* scriptContext, AsmJsModuleInfo* info, const Var foreign);
        static bool CheckArrayLibraryMethod(ScriptContext* scriptContext, const Var stdlib, const AsmJSTypedArrayBuiltinFunction arrayBuiltin);
        static bool CheckMathLibraryMethod(ScriptContext* scriptContext, const Var asmMathObject, const AsmJSMathBuiltinFunction mathBuiltin);
        static bool CheckIsBuiltinFunction(ScriptContext* scriptContext, const Var object, PropertyId propertyId, const FunctionInfo& funcInfo);
        static bool CheckIsBuiltinValue(ScriptContext* scriptContext, const Var object, PropertyId propertyId, double value);
    };
}
#endif
