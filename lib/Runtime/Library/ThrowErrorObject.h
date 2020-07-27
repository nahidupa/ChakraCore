//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    //
    // This object throws an error when invoked.
    //
    class ThrowErrorObject : public RecyclableObject
    {
    private:
        Field(JavascriptError*) m_error;

    protected:
        ThrowErrorObject(StaticType* type, JavascriptError* error);
        DEFINE_VTABLE_CTOR(ThrowErrorObject, RecyclableObject);

    public:
        static Var DefaultEntryPoint(RecyclableObject* function, CallInfo callInfo, ...);

        static ThrowErrorObject* New(StaticType* type, JavascriptError* error, Recycler* recycler);

        static RecyclableObject* CreateThrowTypeErrorObject(ScriptContext* scriptContext, int32 hCode, PCWSTR varName);
        static RecyclableObject* CreateThrowTypeErrorObject(ScriptContext* scriptContext, int32 hCode, JavascriptString* varName);

    private:
        typedef JavascriptError* (JavascriptLibrary::*CreateErrorFunc)();
        static RecyclableObject* CreateThrowErrorObject(CreateErrorFunc createError, ScriptContext* scriptContext, int32 hCode, PCWSTR varName);
    };

    template <> inline bool VarIsImpl<ThrowErrorObject>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_Undefined;
    }
}
