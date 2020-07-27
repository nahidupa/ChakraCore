//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    template <typename T>
    class DictionaryTypeHandlerBase: public DynamicTypeHandler
    {
    private:
        friend class NullTypeHandlerBase;
        friend class DeferredTypeHandlerBase;
        template <DeferredTypeInitializer initializer, typename DeferredTypeFilter, bool isPrototypeTemplate, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots>
        friend class DeferredTypeHandler;
        friend class PathTypeHandlerBase;
        template<size_t size>
        friend class SimpleTypeHandler;
        friend class DynamicObject;
        friend class DynamicTypeHandler;
        template <typename TPropertyIndex, typename TMapKey, bool IsNotExtensibleSupported> friend class SimpleDictionaryTypeHandlerBase;
        template <typename T> friend class DictionaryTypeHandlerBase;

        // Explicit non leaf allocator as the key is non-leaf
        typedef JsUtil::BaseDictionary<const PropertyRecord*, DictionaryPropertyDescriptor<T>, RecyclerNonLeafAllocator, DictionarySizePolicy<PowerOf2Policy, 1>, PropertyRecordStringHashComparer>
            PropertyDescriptorMap;
        typedef PropertyDescriptorMap PropertyDescriptorMapType; // alias used by diagnostics

    private:
        Field(PropertyDescriptorMap*) propertyMap;
        Field(T) nextPropertyIndex;

#if ENABLE_FIXED_FIELDS
        Field(RecyclerWeakReference<DynamicObject>*) singletonInstance;
#endif

        typedef PropertyIndexRanges<T> PropertyIndexRangesType;
        static const T MaxPropertyIndexSize = PropertyIndexRangesType::MaxValue;
        static const T NoSlots = PropertyIndexRangesType::NoSlots;

    public:
        DEFINE_GETCPPNAME();

    protected:
        DictionaryTypeHandlerBase(Recycler* recycler);
        DictionaryTypeHandlerBase(Recycler* recycler, int slotCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots);
        DictionaryTypeHandlerBase(DictionaryTypeHandlerBase* typeHandler);
        DictionaryTypeHandlerBase(Recycler* recycler, DictionaryTypeHandlerBase * typeHandler);
        DEFINE_VTABLE_CTOR_NO_REGISTER(DictionaryTypeHandlerBase, DynamicTypeHandler);

        // Create a new type handler for a future DynamicObject. This is for public usage. "initialCapacity" indicates desired slotCapacity, subject to alignment round up.
        template <typename SubClassType>
        static SubClassType* NewTypeHandler(Recycler * recycler, int initialCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots)
        {
            PropertyIndexRangesType::VerifySlotCapacity(initialCapacity);
            return RecyclerNew(recycler, SubClassType, recycler, initialCapacity, inlineSlotCapacity, offsetOfInlineSlots);
        }

    public:
        typedef T PropertyIndexType;

        // Create a new type handler for a future DynamicObject. This is for public usage. "initialCapacity" indicates desired slotCapacity, subject to alignment round up.
        static DictionaryTypeHandlerBase* New(Recycler * recycler, int initialCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots);

        virtual DynamicTypeHandler * Clone(Recycler * recycler);

        BOOL IsBigDictionaryTypeHandler();

        virtual BOOL IsLockable() const override { return false; }
        virtual BOOL IsSharable() const override { return false; }        
        virtual int GetPropertyCount() override;

        virtual PropertyId GetPropertyId(ScriptContext* scriptContext, PropertyIndex index) override;
        virtual PropertyId GetPropertyId(ScriptContext* scriptContext, BigPropertyIndex index) override;

        virtual BOOL FindNextProperty(ScriptContext* scriptContext, PropertyIndex& index, JavascriptString** propertyString,
            PropertyId* propertyId, PropertyAttributes* attributes, Type* type, DynamicType *typeToEnumerate, EnumeratorFlags flags, DynamicObject* instance, PropertyValueInfo* info) override;
        virtual BOOL FindNextProperty(ScriptContext* scriptContext, BigPropertyIndex& index, JavascriptString** propertyString,
            PropertyId* propertyId, PropertyAttributes* attributes, Type* type, DynamicType *typeToEnumerate, EnumeratorFlags flags, DynamicObject* instance, PropertyValueInfo* info) override;

        virtual PropertyIndex GetPropertyIndex(PropertyRecord const* propertyRecord) override;
#if ENABLE_NATIVE_CODEGEN
        virtual bool GetPropertyEquivalenceInfo(PropertyRecord const* propertyRecord, PropertyEquivalenceInfo& info) override;
        virtual bool IsObjTypeSpecEquivalent(const Type* type, const TypeEquivalenceRecord& record, uint& failedPropertyIndex) override;
        virtual bool IsObjTypeSpecEquivalent(const Type* type, const EquivalentPropertyEntry* entry) override;
#endif

        virtual BOOL HasProperty(DynamicObject* instance, PropertyId propertyId, bool *noRedecl = nullptr, _Inout_opt_ PropertyValueInfo* info = nullptr) override;
        virtual BOOL HasProperty(DynamicObject* instance, JavascriptString* propertyNameString) override;
        virtual BOOL GetProperty(DynamicObject* instance, Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetProperty(DynamicObject* instance, Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL InitProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(DynamicObject* instance, JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetInternalProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags) override;
        virtual DescriptorFlags GetSetter(DynamicObject* instance, PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(DynamicObject* instance, JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL DeleteProperty(DynamicObject* instance, PropertyId propertyId, PropertyOperationFlags flags) override;
        virtual BOOL DeleteProperty(DynamicObject* instance, JavascriptString *propertyNameString, PropertyOperationFlags flags) override;

        virtual PropertyIndex GetRootPropertyIndex(PropertyRecord const* propertyRecord) override;

        virtual BOOL HasRootProperty(DynamicObject* instance, PropertyId propertyId, bool *noRedecl, bool *pDeclaredProperty = nullptr, bool *pNonconfigurableProperty = nullptr) override;
        virtual BOOL GetRootProperty(DynamicObject* instance, Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetRootProperty(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual DescriptorFlags GetRootSetter(DynamicObject* instance, PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL DeleteRootProperty(DynamicObject* instance, PropertyId propertyId, PropertyOperationFlags flags) override;

        virtual BOOL IsDictionaryTypeHandler() const {return TRUE;}
#if DBG
        virtual bool IsLetConstGlobal(DynamicObject* instance, PropertyId propertyId) override;
#endif
        virtual bool NextLetConstGlobal(int& index, RootObjectBase* instance, const PropertyRecord** propertyRecord, Var* value, bool* isConst) override;

        virtual BOOL IsEnumerable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL IsWritable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL IsConfigurable(DynamicObject* instance, PropertyId propertyId) override;
        virtual BOOL SetEnumerable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetWritable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetConfigurable(DynamicObject* instance, PropertyId propertyId, BOOL value) override;
        virtual BOOL SetItem(DynamicObject* instance, uint32 index, Var value, PropertyOperationFlags flags) override;
        virtual BOOL SetItemWithAttributes(DynamicObject* instance, uint32 index, Var value, PropertyAttributes attributes) override;
        virtual BOOL SetItemAttributes(DynamicObject* instance, uint32 index, PropertyAttributes attributes) override;
        virtual BOOL SetItemAccessors(DynamicObject* instance, uint32 index, Var getter, Var setter) override;
        virtual DescriptorFlags GetItemSetter(DynamicObject* instance, uint32 index, Var* setterValue, ScriptContext* requestContext) override;
        virtual BOOL SetAccessors(DynamicObject* instance, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override sealed;
        _Check_return_ _Success_(return) virtual BOOL GetAccessors(DynamicObject* instance, PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter) override;
        virtual BOOL PreventExtensions(DynamicObject *instance) override;
        virtual BOOL Seal(DynamicObject* instance) override;
        virtual BOOL IsSealed(DynamicObject* instance) override;
        virtual BOOL IsFrozen(DynamicObject* instance) override;
        virtual BOOL SetPropertyWithAttributes(DynamicObject* instance, PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override;
        virtual BOOL SetAttributes(DynamicObject* instance, PropertyId propertyId, PropertyAttributes attributes) override;
        virtual BOOL GetAttributesWithPropertyIndex(DynamicObject * instance, PropertyId propertyId, BigPropertyIndex index, PropertyAttributes * attributes) override;

        virtual void SetAllPropertiesToUndefined(DynamicObject* instance, bool invalidateFixedFields) override;
        virtual void MarshalAllPropertiesToScriptContext(DynamicObject* instance, ScriptContext* targetScriptContext, bool invalidateFixedFields) override;
        virtual DynamicTypeHandler* ConvertToTypeWithItemAttributes(DynamicObject* instance) override;

        virtual void SetIsPrototype(DynamicObject* instance) override;

#if DBG
        virtual bool SupportsPrototypeInstances() const { return true; }
        virtual bool CanStorePropertyValueDirectly(const DynamicObject* instance, PropertyId propertyId, bool allowLetConst) override;
#endif

#if ENABLE_FIXED_FIELDS
        virtual void DoShareTypeHandler(ScriptContext* scriptContext) override { AssertMsg(false, "DictionaryTypeHandlers cannot be shared."); };
        virtual BOOL IsFixedProperty(const DynamicObject* instance, PropertyId propertyId) override;

        virtual bool HasSingletonInstance() const override sealed;
        virtual bool TryUseFixedProperty(PropertyRecord const * propertyRecord, Var * pProperty, FixedPropertyKind propertyType, ScriptContext * requestContext) override;
        virtual bool TryUseFixedAccessor(PropertyRecord const * propertyRecord, Var * pAccessor, FixedPropertyKind propertyType, bool getter, ScriptContext * requestContext) override;

#if DBG
        virtual bool CheckFixedProperty(PropertyRecord const * propertyRecord, Var * pProperty, ScriptContext * requestContext) override;
        virtual bool HasAnyFixedProperties() const override;
#endif

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        virtual void DumpFixedFields() const override;
        static void TraceFixedFieldsBeforeTypeHandlerChange(
            const char16* oldTypeHandlerName, const char16* newTypeHandlerName,
            DynamicObject* instance, DynamicTypeHandler* oldTypeHandler, DynamicType* oldType, RecyclerWeakReference<DynamicObject>* oldSingletonInstanceBefore);
        static void TraceFixedFieldsAfterTypeHandlerChange(
            DynamicObject* instance, DynamicTypeHandler* oldTypeHandler, DynamicTypeHandler* newTypeHandler,
            DynamicType* oldType, RecyclerWeakReference<DynamicObject>* oldSingletonInstanceBefore);
        static void TraceFixedFieldsBeforeSetIsProto(
            DynamicObject* instance, DynamicTypeHandler* oldTypeHandler, DynamicType* oldType, RecyclerWeakReference<DynamicObject>* oldSingletonInstanceBefore);
        static void TraceFixedFieldsAfterSetIsProto(
            DynamicObject* instance, DynamicTypeHandler* oldTypeHandler, DynamicTypeHandler* newTypeHandler,
            DynamicType* oldType, RecyclerWeakReference<DynamicObject>* oldSingletonInstanceBefore);
#endif

    private:

        template <bool allowNonExistent, bool markAsUsed>
        bool TryGetFixedProperty(PropertyRecord const * propertyRecord, Var * pProperty, FixedPropertyKind propertyType, ScriptContext * requestContext);
        template <bool allowNonExistent, bool markAsUsed>
        bool TryGetFixedAccessor(PropertyRecord const * propertyRecord, Var * pAccessor, FixedPropertyKind propertyType, bool getter, ScriptContext * requestContext);

    public:
        virtual RecyclerWeakReference<DynamicObject>* GetSingletonInstance() const sealed { Assert(HasSingletonInstanceOnlyIfNeeded()); return this->singletonInstance; }

        virtual void SetSingletonInstanceUnchecked(RecyclerWeakReference<DynamicObject>* instance) override
        {
            Assert(!GetIsShared());
            Assert(this->singletonInstance == nullptr);
            this->singletonInstance = instance;
        }

        virtual void ClearSingletonInstance() override sealed
        {
            Assert(HasSingletonInstanceOnlyIfNeeded());
            this->singletonInstance = nullptr;
        }

#if DBG
        bool HasSingletonInstanceOnlyIfNeeded() const
        {
            return AreSingletonInstancesNeeded() || this->singletonInstance == nullptr;
        }
#endif

    private:
        void CopySingletonInstance(DynamicObject* instance, DynamicTypeHandler* typeHandler);

        template <typename TPropertyKey>
        void InvalidateFixedField(DynamicObject* instance, TPropertyKey propertyKey, DictionaryPropertyDescriptor<T>* descriptor);
#endif
    private:
#if ENABLE_NATIVE_CODEGEN
        template<bool doLock>
        bool IsObjTypeSpecEquivalentImpl(const Type* type, const EquivalentPropertyEntry *entry);
#endif
        void SetNumDeletedProperties(const byte n) {}

        void Add(const PropertyRecord* propertyId, PropertyAttributes attributes, ScriptContext *const scriptContext);
        void Add(const PropertyRecord* propertyId, PropertyAttributes attributes, bool isInitialized, bool isFixed, bool usedAsFixed, ScriptContext *const scriptContext);

        void EnsureSlotCapacity(DynamicObject * instance, T increment = 1);

        BOOL AddProperty(DynamicObject* instance, const PropertyRecord* propertyRecord, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, bool throwIfNotExtensible, SideEffects possibleSideEffects);
        ES5ArrayTypeHandlerBase<T>* ConvertToES5ArrayType(DynamicObject *instance);

        BigDictionaryTypeHandler* ConvertToBigDictionaryTypeHandler(DynamicObject* instance);

        void SetPropertyValueInfo(PropertyValueInfo* info, RecyclableObject* instance, T propIndex, DictionaryPropertyDescriptor<T>* descriptor);
        void SetPropertyValueInfoNonFixed(PropertyValueInfo* info, RecyclableObject* instance, T propIndex, PropertyAttributes attributes, InlineCacheFlags flags = InlineCacheNoFlags);

        template<bool allowLetConstGlobal>
        inline BOOL HasProperty_Internal(DynamicObject* instance, PropertyId propertyId, bool *noRedecl, _Inout_opt_ PropertyValueInfo* info, bool *pDeclaredProperty, bool *pNonconfigurableProperty);
        template<bool allowLetConstGlobal>
        inline PropertyIndex GetPropertyIndex_Internal(PropertyRecord const* propertyRecord);
        template<bool allowLetConstGlobal>
        inline BOOL GetProperty_Internal(DynamicObject* instance, Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext);
        template<bool allowLetConstGlobal>
        inline BOOL SetProperty_Internal(DynamicObject* instance, PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info, bool isInit = false);
        template<bool allowLetConstGlobal>
        inline DescriptorFlags  GetSetter_Internal(DynamicObject* instance, PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext);
        template<bool allowLetConstGlobal>
        inline BOOL DeleteProperty_Internal(DynamicObject* instance, PropertyId propertyId, PropertyOperationFlags flags);


        template<bool allowLetConstGlobal, typename PropertyType>
        inline BOOL GetPropertyFromDescriptor(DynamicObject* instance, Var originalInstance, DictionaryPropertyDescriptor<T>* descriptor,
            Var* value, PropertyValueInfo* info, PropertyType propertyT, ScriptContext* requestContext);
        template<bool allowLetConstGlobal>
        inline DescriptorFlags GetSetterFromDescriptor(DynamicObject* instance, DictionaryPropertyDescriptor<T> * descriptor, Var* setterValue, PropertyValueInfo* info);
        template <bool allowLetConstGlobal>
        inline void SetPropertyWithDescriptor(
            _In_ DynamicObject* instance,
            _In_ PropertyRecord const* propertyRecord,
            _Inout_ DictionaryPropertyDescriptor<T> ** pdescriptor,
            _In_ Var value,
            _In_ PropertyOperationFlags flags,
            _Inout_opt_ PropertyValueInfo* info);

    protected:
        virtual BOOL FreezeImpl(DynamicObject* instance, bool isConvertedType) override;
        virtual BigDictionaryTypeHandler* NewBigDictionaryTypeHandler(Recycler* recycler, int slotCapacity, uint16 inlineSlotCapacity, uint16 offsetOfInlineSlots);

#if ENABLE_TTD
    public:
        virtual void MarkObjectSlots_TTD(TTD::SnapshotExtractor* extractor, DynamicObject* obj) const override;

        virtual uint32 ExtractSlotInfo_TTD(TTD::NSSnapType::SnapHandlerPropertyEntry* entryInfo, ThreadContext* threadContext, TTD::SlabAllocator& alloc) const override;

        virtual Js::BigPropertyIndex GetPropertyIndex_EnumerateTTD(const Js::PropertyRecord* pRecord) override;
#endif

#if DBG_DUMP
    public:
        void Dump(unsigned indent = 0) const override;
#endif
    };

    template <bool allowLetConstGlobal>
    inline PropertyAttributes GetLetConstGlobalPropertyAttributes(PropertyAttributes attributes);
}
