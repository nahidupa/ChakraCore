//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class ScriptFunctionType : public DynamicType
    {
    public:
        static ScriptFunctionType * New(FunctionProxy * proxy, bool isShared);
        static ScriptFunctionType * New(FunctionProxy * proxy, DynamicTypeHandler * typeHandler, RecyclableObject * prototype, bool isShared);
        static DWORD GetEntryPointInfoOffset() { return offsetof(ScriptFunctionType, entryPointInfo); }
        ProxyEntryPointInfo * GetEntryPointInfo() const { return entryPointInfo; }
        void SetEntryPointInfo(ProxyEntryPointInfo * entryPointInfo) { this->entryPointInfo = entryPointInfo; }
        void ChangeEntryPoint(ProxyEntryPointInfo * entryPointInfo, JavascriptMethod entryPoint, bool isAsmJS);
    private:
        ScriptFunctionType(ScriptFunctionType * type);
        ScriptFunctionType(ScriptContext* scriptContext, RecyclableObject* prototype,
            JavascriptMethod entryPoint, ProxyEntryPointInfo * entryPointInfo, DynamicTypeHandler * typeHandler,
            bool isLocked, bool isShared);

        Field(ProxyEntryPointInfo *) entryPointInfo;

        friend class ScriptFunction;
        friend class JavascriptLibrary;
    };
};
