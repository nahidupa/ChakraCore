//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class DeferredTypeHandlerBase : public DynamicTypeHandler
    {
    public:
        DEFINE_GETCPPNAME();

    protected:
        DeferredTypeHandlerBase(bool isPrototype, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots) :
            DynamicTypeHandler(0, inlineSlotCapacity, offsetOfInlineSlots, DefaultFlags | IsLockedFlag | MayBecomeSharedFlag | IsSharedFlag | (isPrototype ? IsPrototypeFlag : 0))
        {
            SetIsInlineSlotCapacityLocked();
            this->ClearHasOnlyWritableDataProperties(); // Until the type handler is initialized, we cannot
                                                        // be certain that the type has only writable data properties.
        }

        DeferredTypeHandlerBase(DeferredTypeHandlerBase * typeHandler) : DynamicTypeHandler(typeHandler)
        {
            Assert(this->GetIsInlineSlotCapacityLocked() == typeHandler->GetIsInlineSlotCapacityLocked());
            Assert(this->GetHasOnlyWritableDataProperties() == typeHandler->GetHasOnlyWritableDataProperties());
        }
    public:
        void ConvertFunction(JavascriptFunction * instance, DynamicTypeHandler * handler);
        void Convert(DynamicObject * instance, DeferredInitializeMode mode, int initSlotCapacity,  BOOL hasAccessor = false);

        virtual void SetAllPropertiesToUndefined(DynamicObject* instance, bool invalidateFixedFields) override {};
        virtual void MarshalAllPropertiesToScriptContext(DynamicObject* instance, ScriptContext* targetScriptContext, bool invalidateFixedFields) override {};

        virtual BOOL IsDeferredTypeHandler() const override { return TRUE; }

        virtual void SetIsPrototype(DynamicObject* instance) override { Assert(false); }
#if DBG
        virtual bool SupportsPrototypeInstances() const { Assert(false); return false; }
        virtual bool RespectsIsolatePrototypes() const { return false; }
        virtual bool RespectsChangeTypeOnProto() const { return false; }
#endif

    private:
        template <typename T>
        T* ConvertToTypeHandler(DynamicObject* instance, int initSlotCapacity, BOOL isProto = FALSE);

        DictionaryTypeHandler * ConvertToDictionaryType(DynamicObject* instance, int initSlotCapacity, BOOL isProto);
        SimpleDictionaryTypeHandler * ConvertToSimpleDictionaryType(DynamicObject* instance, int initSlotCapacity, BOOL isProto);
        ES5ArrayTypeHandler * ConvertToES5ArrayType(DynamicObject* instance, int initSlotCapacity);

#if ENABLE_TTD
    public:
        virtual void MarkObjectSlots_TTD(TTD::SnapshotExtractor* extractor, DynamicObject* obj) const override
        {
            ;
        }

        virtual uint32 ExtractSlotInfo_TTD(TTD::NSSnapType::SnapHandlerPropertyEntry* entryInfo, ThreadContext* threadContext, TTD::SlabAllocator& alloc) const override
        {
            return 0;
        }
#endif
    };

    class DefaultDeferredTypeFilter
    {
    public:
        static bool HasFilter() { return false; }
        static bool HasProperty(PropertyId propertyId) { Assert(false); return false; }
    };

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter = DefaultDeferredTypeFilter, bool isPrototypeTemplate = false, uint16 _inlineSlotCapacity = 0, uint16 _offsetOfInlineSlots = 0>
    class DeferredTypeHandler : public DeferredTypeHandlerBase
    {
        friend class DynamicTypeHandler;

    public:
        DEFINE_GETCPPNAME();

    private:
        DeferredTypeHandler() : DeferredTypeHandlerBase(isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots), m_initializer(initializer) { }
        DeferredTypeHandler(DeferredTypeHandler * typeHandler) :
            DeferredTypeHandlerBase(typeHandler),
            m_initializer(typeHandler->m_initializer)
        {}

    public:
        static DeferredTypeHandler *GetDefaultInstance() { return &defaultInstance; }

        virtual DynamicTypeHandler * Clone(Recycler* recycler);
        virtual BOOL IsLockable() const override { return true; }
        virtual BOOL IsSharable() const override { return true; }
        virtual int GetPropertyCount() override;
        virtual PropertyId GetPropertyId(ScriptContext* scriptContext, PropertyIndex index) override;
        virtual PropertyId GetPropertyId(ScriptContext* scriptContext, BigPropertyIndex index) override;
        virtual BOOL FindNextProperty(ScriptContext* scriptContext, PropertyIndex& index, JavascriptString** propertyString,
            PropertyId* propertyId, PropertyAttributes* attributes, Type* type, DynamicType *typeToEnumerate, EnumeratorFlags flags, DynamicObject* instance, PropertyValueInfo* info) override;
        virtual PropertyIndex GetPropertyIndex(PropertyRecord const* propertyRecord) override;
#if ENABLE_NATIVE_CODEGEN
        virtual bool GetPropertyEquivalenceInfo(PropertyRecord const* propertyRecord, PropertyEquivalenceInfo& info) override;
        virtual bool IsObjTypeSpecEquivalent(const Type* type, const TypeEquivalenceRecord& record, uint& failedPropertyIndex) override;
        virtual bool IsObjTypeSpecEquivalent(const Type* type, const EquivalentPropertyEntry* entry) override;
#endif
        virtual bool EnsureObjectReady(DynamicObject* instance) override;
        virtual BOOL HasProperty(DynamicObject* instance, PropertyId propertyId, __out_opt bool *noRedecl = nullptr, _Inout_opt_ PropertyValueInfo* info = nullptr) override;
        virtual BOOL HasProperty(DynamicObject* instance, JavascriptString* propertyNameString) override;
        virtual BOOL GetProperty(DynamicObject* instance, Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetProperty(DynamicObject* instance, Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(DynamicObject* instance, JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetInternalProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags) override;
        virtual DescriptorFlags GetSetter(DynamicObject* instance, PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(DynamicObject* instance, JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL DeleteProperty(DynamicObject* instance, PropertyId propertyId, PropertyOperationFlags flags) override;

        virtual BOOL HasItem(DynamicObject* instance, uint32 index);
        virtual BOOL SetItem(DynamicObject* instance, uint32 index, Var value, PropertyOperationFlags flags) override;
        virtual BOOL SetItemWithAttributes(DynamicObject* instance, uint32 index, Var value, PropertyAttributes attributes) override;
        virtual BOOL SetItemAttributes(DynamicObject* instance, uint32 index, PropertyAttributes attributes) override;
        virtual BOOL SetItemAccessors(DynamicObject* instance, uint32 index, Var getter, Var setter) override;
        virtual BOOL DeleteItem(DynamicObject* instance, uint32 index, PropertyOperationFlags flags) override;
        virtual BOOL GetItem(DynamicObject* instance, Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual DescriptorFlags GetItemSetter(DynamicObject* instance, uint32 index, Var* setterValue, ScriptContext* requestContext) override;

        virtual BOOL IsEnumerable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL IsWritable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL IsConfigurable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL IsFrozen(DynamicObject *instance) override;
        virtual BOOL IsSealed(DynamicObject *instance) override;
        virtual BOOL SetEnumerable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetWritable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetConfigurable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetAccessors(DynamicObject* instance, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override;
        _Check_return_ _Success_(return) virtual BOOL GetAccessors(DynamicObject* instance, PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter) override;
        virtual BOOL PreventExtensions(DynamicObject *instance) override;
        virtual BOOL Seal(DynamicObject *instance) override;
        virtual BOOL SetPropertyWithAttributes(DynamicObject* instance, PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override;
        virtual BOOL SetAttributes(DynamicObject* instance, PropertyId propertyId, PropertyAttributes attributes) override;
        virtual BOOL GetAttributesWithPropertyIndex(DynamicObject * instance, PropertyId propertyId, BigPropertyIndex index, PropertyAttributes * attributes) override;

        virtual DynamicTypeHandler* ConvertToTypeWithItemAttributes(DynamicObject* instance) override;

        virtual void SetIsPrototype(DynamicObject* instance) override;

#if DBG
        virtual bool SupportsPrototypeInstances() const { return isPrototypeTemplate; }
#endif

    private:
        static DeferredTypeHandler defaultInstance;

        FieldNoBarrier(DeferredTypeInitializer) m_initializer;

        bool EnsureObjectReady(DynamicObject* instance, DeferredInitializeMode mode);
        virtual BOOL FreezeImpl(DynamicObject *instance, bool isConvertedType) override;
#if DBG_DUMP
    public:
        void Dump(unsigned indent = 0) const override
        {
            Output::Print(_u("%*sDeferredTypeHandler (0x%p): Dump unimplemented\n"), indent, _u(""), this);
        }
#endif
    };

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots> DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::defaultInstance;

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    DynamicTypeHandler * DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::Clone(Recycler * recycler)
    {
        return RecyclerNew(recycler, DeferredTypeHandler, this);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    int DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::GetPropertyCount()
    {
        return 0;
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    PropertyId DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::GetPropertyId(ScriptContext* scriptContext, PropertyIndex index)
    {
        Assert(false);
        return Constants::NoProperty;
    }
    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    PropertyId DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::GetPropertyId(ScriptContext* scriptContext, BigPropertyIndex index)
    {
        Assert(false);
        return Constants::NoProperty;
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::FindNextProperty(ScriptContext* scriptContext, PropertyIndex& index,
        __out JavascriptString** propertyString, __out PropertyId* propertyId, __out_opt PropertyAttributes* attributes, Type* type, DynamicType *typeToEnumerate, EnumeratorFlags flags, DynamicObject* instance, PropertyValueInfo* info)
    {
        Assert(false);
        return FALSE;
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    PropertyIndex DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::GetPropertyIndex(PropertyRecord const* propertyRecord)
    {
        return Constants::NoSlot;
    }

#if ENABLE_NATIVE_CODEGEN
    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    bool DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::GetPropertyEquivalenceInfo(PropertyRecord const* propertyRecord, PropertyEquivalenceInfo& info)
    {
        info.slotIndex = Constants::NoSlot;
        info.isAuxSlot = false;
        info.isWritable = false;
        return false;
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    bool DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::IsObjTypeSpecEquivalent(const Type* type, const TypeEquivalenceRecord& record, uint& failedPropertyIndex)
    {
        uint propertyCount = record.propertyCount;
        EquivalentPropertyEntry* properties = record.properties;
        for (uint pi = 0; pi < propertyCount; pi++)
        {
            const EquivalentPropertyEntry* refInfo = &properties[pi];
            if (!this->DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::IsObjTypeSpecEquivalent(type, refInfo))
            {
                failedPropertyIndex = pi;
                return false;
            }
        }
        return true;
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    bool DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::IsObjTypeSpecEquivalent(const Type* type, const EquivalentPropertyEntry* entry)
    {
        if (!DeferredTypeFilter::HasFilter())
        {
            return false;
        }

        if (entry->slotIndex != Constants::NoSlot || entry->mustBeWritable || DeferredTypeFilter::HasProperty(entry->propertyId))
        {
            return false;
        }

        return true;
    }
#endif

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    bool DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::EnsureObjectReady(DynamicObject* instance)
    {
        return EnsureObjectReady(instance, DeferredInitializeMode_Default);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    bool DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::EnsureObjectReady(DynamicObject* instance, DeferredInitializeMode mode)
    {
        Assert(initializer == m_initializer);
        return m_initializer(instance, this, mode);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::HasProperty(DynamicObject* instance, PropertyId propertyId, __out_opt bool *noRedecl, _Inout_opt_ PropertyValueInfo* info)
    {
        if (noRedecl != nullptr)
        {
            *noRedecl = false;
        }

        if (DeferredTypeFilter::HasFilter() && DeferredTypeFilter::HasProperty(propertyId))
        {
            return TRUE;
        }

        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->HasProperty(instance, propertyId, noRedecl, info);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::HasProperty(DynamicObject* instance, JavascriptString* propertyNameString)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->HasProperty(instance, propertyNameString);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::GetProperty(DynamicObject* instance, Var originalInstance,
        PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        if (DeferredTypeFilter::HasFilter() && !DeferredTypeFilter::HasProperty(propertyId))
        {
            *value = requestContext->GetMissingPropertyResult();
            return FALSE;
        }

        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            *value = requestContext->GetMissingPropertyResult();

            // Return true so that we stop walking the prototype
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->GetProperty(instance, originalInstance, propertyId, value, info, requestContext);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::GetProperty(DynamicObject* instance, Var originalInstance,
        JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            *value = requestContext->GetMissingPropertyResult();

            // Return true so that we stop walking the prototype
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->GetProperty(instance, originalInstance, propertyNameString, value, info, requestContext);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Set))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetProperty(instance, propertyId, value, flags, info);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetProperty(DynamicObject* instance, JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Set))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetProperty(instance, propertyNameString, value, flags, info);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetInternalProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Set))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetInternalProperty(instance, propertyId, value, flags);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    DescriptorFlags DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::GetSetter(DynamicObject* instance, PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        if (setterValue != nullptr)
        {
            *setterValue = nullptr;
        }

        if (DeferredTypeFilter::HasFilter() && !DeferredTypeFilter::HasProperty(propertyId))
        {
            return DescriptorFlags::None;
        }

        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return DescriptorFlags::None;
        }

        return GetCurrentTypeHandler(instance)->GetSetter(instance, propertyId, setterValue, info, requestContext);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    DescriptorFlags DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::GetSetter(DynamicObject* instance, JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        if (setterValue != nullptr)
        {
            *setterValue = nullptr;
        }

        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return DescriptorFlags::None;
        }

        return GetCurrentTypeHandler(instance)->GetSetter(instance, propertyNameString, setterValue, info, requestContext);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::DeleteProperty(DynamicObject* instance, PropertyId propertyId, PropertyOperationFlags flags)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->DeleteProperty(instance, propertyId, flags);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::HasItem(DynamicObject* instance, uint32 index)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->HasItem(instance, index);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetItem(DynamicObject* instance, uint32 index, Var value, PropertyOperationFlags flags)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetItem(instance, index, value, flags);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetItemWithAttributes(DynamicObject* instance, uint32 index, Var value, PropertyAttributes attributes)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }
        return GetCurrentTypeHandler(instance)->SetItemWithAttributes(instance, index, value, attributes);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetItemAttributes(DynamicObject* instance, uint32 index, PropertyAttributes attributes)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetItemAttributes(instance, index, attributes);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetItemAccessors(DynamicObject* instance, uint32 index, Var getter, Var setter)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetItemAccessors(instance, index, getter, setter);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::DeleteItem(DynamicObject* instance, uint32 index, PropertyOperationFlags flags)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->DeleteItem(instance, index, flags);
    }
    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::GetItem(DynamicObject* instance, Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            *value = requestContext->GetMissingItemResult();
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->GetItem(instance, originalInstance, index, value, requestContext);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    DescriptorFlags DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::GetItemSetter(DynamicObject* instance, uint32 index, Var* setterValue, ScriptContext* requestContext)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return DescriptorFlags::None;
        }

        return GetCurrentTypeHandler(instance)->GetItemSetter(instance, index, setterValue, requestContext);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::IsEnumerable(DynamicObject* instance, PropertyId propertyId)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->IsEnumerable(instance, propertyId);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::IsWritable(DynamicObject* instance, PropertyId propertyId)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->IsWritable(instance, propertyId);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::IsConfigurable(DynamicObject* instance, PropertyId propertyId)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->IsConfigurable(instance, propertyId);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetEnumerable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetEnumerable(instance, propertyId, value);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetWritable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetWritable(instance, propertyId, value);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetConfigurable(DynamicObject* instance, PropertyId propertyId, BOOL value)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetConfigurable(instance, propertyId, value);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetAccessors(DynamicObject* instance, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_SetAccessors))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetAccessors(instance, propertyId, getter, setter, flags);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    _Check_return_ _Success_(return) BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::GetAccessors(DynamicObject* instance, PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter)
    {
        if (getter != nullptr)
        {
            *getter = nullptr;
        }

        if (setter != nullptr)
        {
            *setter = nullptr;
        }

        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->GetAccessors(instance, propertyId, getter, setter);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::IsSealed(DynamicObject *instance)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->IsSealed(instance);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::IsFrozen(DynamicObject *instance)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->IsFrozen(instance);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::PreventExtensions(DynamicObject* instance)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Extensions))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->PreventExtensions(instance);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::Seal(DynamicObject* instance)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Extensions))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->Seal(instance);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::FreezeImpl(DynamicObject* instance, bool isConvertedType)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Extensions))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->Freeze(instance, true);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetPropertyWithAttributes(DynamicObject* instance, PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Set))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetPropertyWithAttributes(instance, propertyId, value, attributes, info, flags, possibleSideEffects);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetAttributes(DynamicObject* instance, PropertyId propertyId, PropertyAttributes attributes)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Set))
        {
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->SetAttributes(instance, propertyId, attributes);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    BOOL DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::GetAttributesWithPropertyIndex(DynamicObject * instance, PropertyId propertyId, BigPropertyIndex index, PropertyAttributes * attributes)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Default))
        {
            if (attributes)
            {
                *attributes = PropertyDynamicTypeDefaults;
            }
            return TRUE;
        }

        return GetCurrentTypeHandler(instance)->GetAttributesWithPropertyIndex(instance, propertyId, index, attributes);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    DynamicTypeHandler* DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::ConvertToTypeWithItemAttributes(DynamicObject* instance)
    {
        if (!EnsureObjectReady(instance, DeferredInitializeMode_Set))
        {
            AssertOrFailFastMsg(false, "Is this a valid scenario?");
            return nullptr;
        }

        return GetCurrentTypeHandler(instance)->ConvertToTypeWithItemAttributes(instance);
    }

    template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 _inlineSlotCapacity, uint16 _offsetOfInlineSlots>
    void DeferredTypeHandler<initializer, DeferredTypeFilter, isPrototypeTemplate, _inlineSlotCapacity, _offsetOfInlineSlots>::SetIsPrototype(DynamicObject* instance)
    {
        if (!isPrototypeTemplate)
        {
            // We don't force a type transition even when ChangeTypeOnProto() == true, because objects with NullTypeHandlers don't
            // have any properties, so there is nothing to invalidate.  Types with NullTypeHandlers also aren't cached in typeWithoutProperty
            // caches, so there will be no fast property add path that could skip prototype cache invalidation.
            DeferredTypeHandlerBase* protoTypeHandler = DeferredTypeHandler<initializer, DeferredTypeFilter, true, _inlineSlotCapacity, _offsetOfInlineSlots>::GetDefaultInstance();
            AssertMsg(protoTypeHandler->GetFlags() == (GetFlags() | IsPrototypeFlag), "Why did we change the flags of a DeferredTypeHandler?");
            Assert(this->GetIsInlineSlotCapacityLocked() == protoTypeHandler->GetIsInlineSlotCapacityLocked());
            protoTypeHandler->SetPropertyTypes(PropertyTypesWritableDataOnly | PropertyTypesWritableDataOnlyDetection | PropertyTypesHasSpecialProperties, GetPropertyTypes());
            SetInstanceTypeHandler(instance, protoTypeHandler);
        }
    }

}
