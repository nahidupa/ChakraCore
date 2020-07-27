//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    // Currently only used to implement Date.prototype.getVarDate
    // If there are other MS specific extensions that return variants,
    // this class could be renamed and updated to support other VT_* types,
    // or relevant functionality could be moved to a base class to inherit from.
    class JavascriptVariantDate : public RecyclableObject
    {
    private:
        Field(double) value; // the double form of the value of a VT_DATE variant.

        static JavascriptString* ConvertVariantDateToStr(
            double dbl,
            ScriptContext* scriptContext);

    protected:
        DEFINE_VTABLE_CTOR(JavascriptVariantDate, RecyclableObject);

    public:
        JavascriptVariantDate(const double val, StaticType * type) : value(val), RecyclableObject(type) { }

        // Used for making function calls to external objects requiring string params.
        JavascriptString* GetValueString(ScriptContext* scriptContext);

        double GetValue() { return value; }

        virtual BOOL Equals(Var other, BOOL* value, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags HasPropertyQuery(Js::PropertyId propertyId, _Inout_opt_ PropertyValueInfo* info) override { return PropertyQueryFlags::Property_NotFound; };
        virtual PropertyQueryFlags GetPropertyQuery(Js::Var originalInstance, Js::PropertyId propertyId, Js::Var* value, Js::PropertyValueInfo* info, Js::ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Js::Var originalInstance, Js::PropertyId propertyId, Js::Var* value, Js::PropertyValueInfo* info, Js::ScriptContext* requestContext) override;
        virtual BOOL SetProperty(Js::PropertyId propertyId, Js::Var value, Js::PropertyOperationFlags flags, Js::PropertyValueInfo* info) override;
        virtual BOOL SetProperty(Js::JavascriptString* propertyNameString, Js::Var value, Js::PropertyOperationFlags flags, Js::PropertyValueInfo* info) override;
        virtual BOOL InitProperty(Js::PropertyId propertyId, Js::Var value, PropertyOperationFlags flags = PropertyOperation_None, Js::PropertyValueInfo* info = NULL) override;
        virtual BOOL DeleteProperty(Js::PropertyId propertyId, Js::PropertyOperationFlags flags) override;
        virtual BOOL DeleteProperty(JavascriptString *propertyNameString, Js::PropertyOperationFlags flags) override;
        virtual PropertyQueryFlags HasItemQuery(uint32 index) override { return PropertyQueryFlags::Property_NotFound; }
        virtual PropertyQueryFlags GetItemReferenceQuery(Js::Var originalInstance, uint32 index, Js::Var* value, Js::ScriptContext * scriptContext) override;
        virtual PropertyQueryFlags GetItemQuery(Js::Var originalInstance, uint32 index, Js::Var* value, Js::ScriptContext * scriptContext) override;
        virtual BOOL SetItem(uint32 index, Js::Var value, Js::PropertyOperationFlags flags) override;
        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL ToPrimitive(JavascriptHint hint, Var* result, ScriptContext * requestContext) override;
        virtual Var GetTypeOfString(ScriptContext * requestContext) override;
        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;
        virtual RecyclableObject* ToObject(ScriptContext* requestContext) override;
    };

    template <> inline bool VarIsImpl<JavascriptVariantDate>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_VariantDate;
    }
}
