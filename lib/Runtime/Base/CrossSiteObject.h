//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    template <typename T>
    class CrossSiteObject : public T
    {
    private:
        DEFINE_VTABLE_CTOR(CrossSiteObject<T>, T);

    public:
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        _Check_return_ _Success_(return) virtual BOOL GetAccessors(PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = NULL) override;
        virtual BOOL SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override;
        virtual BOOL InitPropertyScoped(PropertyId propertyId, Var value) override;
        virtual BOOL InitFuncScoped(PropertyId propertyId, Var value) override;

        virtual PropertyQueryFlags GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual DescriptorFlags GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext) override;
        virtual BOOL SetItem(uint32 index, Var value, PropertyOperationFlags flags) override;
        virtual Var GetHostDispatchVar() override;

        virtual DescriptorFlags GetSetter(PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags) override;
        virtual void RemoveFromPrototype(ScriptContext * requestContext, bool * allProtoCachesInvalidated) override;
        virtual void AddToPrototype(ScriptContext * requestContext, bool * allProtoCachesInvalidated) override;
        virtual void SetPrototype(RecyclableObject* newPrototype) override;

        virtual BOOL IsCrossSiteObject() const override { return TRUE; }
        virtual void MarshalToScriptContext(ScriptContext * requestContext) override
        {
            AssertMsg(false, "CrossSite::MarshalVar should have handled this");
        }

#if ENABLE_TTD
        virtual void MarshalCrossSite_TTDInflate() override
        {
            TTDAssert(false, "Should never call this!!!");
        }
#endif
    };

    template <typename T>
    PropertyQueryFlags CrossSiteObject<T>::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        originalInstance = CrossSite::MarshalVar(this->GetScriptContext(), originalInstance);
        PropertyQueryFlags result = __super::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
        if (JavascriptConversion::PropertyQueryFlagsToBoolean(result))
        {
            *value = CrossSite::MarshalVar(requestContext, *value);
        }
        return result;
    }

    template <typename T>
    PropertyQueryFlags CrossSiteObject<T>::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyQueryFlags result = __super::GetPropertyQuery(originalInstance, propertyNameString, value, info, requestContext);
        if (JavascriptConversion::PropertyQueryFlagsToBoolean(result))
        {
            *value = CrossSite::MarshalVar(requestContext, *value);
        }
        return result;
    }

    template <typename T>
    _Check_return_ _Success_(return) BOOL CrossSiteObject<T>::GetAccessors(PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter, ScriptContext * requestContext)
    {
        BOOL result = __super::GetAccessors(propertyId, getter, setter, requestContext);
        if (result)
        {
            if (*getter != nullptr)
            {
                *getter = CrossSite::MarshalVar(requestContext, *getter);
            }
            if (*setter != nullptr)
            {
                *setter = CrossSite::MarshalVar(requestContext, *setter);
            }
        }
        return result;
    }

    template <typename T>
    PropertyQueryFlags CrossSiteObject<T>::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        originalInstance = CrossSite::MarshalVar(this->GetScriptContext(), originalInstance);
        PropertyQueryFlags result = __super::GetPropertyReferenceQuery(originalInstance, propertyId, value, info, requestContext);
        if (JavascriptConversion::PropertyQueryFlagsToBoolean(result))
        {
            *value = CrossSite::MarshalVar(requestContext, *value);
        }
        return result;
    }

    template <typename T>
    BOOL CrossSiteObject<T>::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        value = CrossSite::MarshalVar(this->GetScriptContext(), value);
        return __super::SetProperty(propertyId, value, flags, info);
    }

    template <typename T>
    BOOL CrossSiteObject<T>::SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        value = CrossSite::MarshalVar(this->GetScriptContext(), value);
        return __super::SetProperty(propertyNameString, value, flags, info);
    }

    template <typename T>
    BOOL CrossSiteObject<T>::InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        value = CrossSite::MarshalVar(this->GetScriptContext(), value);
        return __super::InitProperty(propertyId, value, flags, info);
    }

    template <typename T>
    BOOL CrossSiteObject<T>::SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects /* = SideEffects_Any */)
    {
        value = CrossSite::MarshalVar(this->GetScriptContext(), value);
        return __super::SetPropertyWithAttributes(propertyId, value, attributes, info, flags, possibleSideEffects);
    }

    template <typename T>
    BOOL CrossSiteObject<T>::InitPropertyScoped(PropertyId propertyId, Var value)
    {
        value = CrossSite::MarshalVar(this->GetScriptContext(), value);
        return __super::InitPropertyScoped(propertyId, value);
    }

    template <typename T>
    BOOL CrossSiteObject<T>::InitFuncScoped(PropertyId propertyId, Var value)
    {
        value = CrossSite::MarshalVar(this->GetScriptContext(), value);
        return __super::InitFuncScoped(propertyId, value);
    }

    template <typename T>
    PropertyQueryFlags CrossSiteObject<T>::GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        originalInstance = CrossSite::MarshalVar(this->GetScriptContext(), originalInstance);
        PropertyQueryFlags result = __super::GetItemQuery(originalInstance, index, value, requestContext);
        if (JavascriptConversion::PropertyQueryFlagsToBoolean(result))
        {
            *value = CrossSite::MarshalVar(requestContext, *value);
        }
        return result;
    }

    template <typename T>
    PropertyQueryFlags CrossSiteObject<T>::GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        originalInstance = CrossSite::MarshalVar(this->GetScriptContext(), originalInstance);
        PropertyQueryFlags result = __super::GetItemReferenceQuery(originalInstance, index, value, requestContext);
        if (JavascriptConversion::PropertyQueryFlagsToBoolean(result))
        {
            *value = CrossSite::MarshalVar(requestContext, *value);
        }
        return result;
    }

    template <typename T>
    DescriptorFlags CrossSiteObject<T>::GetItemSetter(uint32 index, Var *setterValue, ScriptContext* requestContext)
    {
        DescriptorFlags flags = __super::GetItemSetter(index, setterValue, requestContext);
        if ((flags & Accessor) == Accessor && *setterValue)
        {
            *setterValue = CrossSite::MarshalVar(requestContext, *setterValue);
        }
        return flags;
    }

    template <typename T>
    BOOL CrossSiteObject<T>::SetItem(uint32 index, Var value, PropertyOperationFlags flags)
    {
        value = CrossSite::MarshalVar(this->GetScriptContext(), value);
        return __super::SetItem(index, value, flags);
    }

    template <typename T>
    DescriptorFlags CrossSiteObject<T>::GetSetter(PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        DescriptorFlags flags = __super::GetSetter(propertyId, setterValue, info, requestContext);
        if ((flags & Accessor) == Accessor && *setterValue)
        {
            PropertyValueInfo::SetNoCache(info, this);
            *setterValue = CrossSite::MarshalVar(requestContext, *setterValue);
        }
        return flags;
    }

    template <typename T>
    DescriptorFlags CrossSiteObject<T>::GetSetter(JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        DescriptorFlags flags = __super::GetSetter(propertyNameString, setterValue, info, requestContext);
        if ((flags & Accessor) == Accessor && *setterValue)
        {
            PropertyValueInfo::SetNoCache(info, this);
            *setterValue = CrossSite::MarshalVar(requestContext, *setterValue);
        }
        return flags;
    }

    template <typename T>
    BOOL CrossSiteObject<T>::SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        if (getter != nullptr)
        {
            getter = CrossSite::MarshalVar(this->GetScriptContext(), getter);
        }
        if (setter != nullptr)
        {
            setter = CrossSite::MarshalVar(this->GetScriptContext(), setter);
        }
        return __super::SetAccessors(propertyId, getter, setter, flags);
    }

    template <typename T>
    void CrossSiteObject<T>::RemoveFromPrototype(ScriptContext * requestContext, bool * allProtoCachesInvalidated)
    {
        __super::RemoveFromPrototype(this->GetScriptContext(), allProtoCachesInvalidated);
    }

    template <typename T>
    void CrossSiteObject<T>::AddToPrototype(ScriptContext * requestContext, bool * allProtoCachesInvalidated)
    {
        __super::AddToPrototype(this->GetScriptContext(), allProtoCachesInvalidated);
    }

    template <typename T>
    void CrossSiteObject<T>::SetPrototype(RecyclableObject* newPrototype)
    {
        newPrototype = (RecyclableObject*)CrossSite::MarshalVar(this->GetScriptContext(), newPrototype);
        __super::SetPrototype(newPrototype);
    }

    template <typename T>
    Var CrossSiteObject<T>::GetHostDispatchVar()
    {
        Var hostDispatch = __super::GetHostDispatchVar();
        AssertMsg(hostDispatch, "hostDispatch");
        hostDispatch = CrossSite::MarshalVar(this->GetScriptContext(), hostDispatch);
        return hostDispatch;
    }
}
