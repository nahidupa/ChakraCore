//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js {

    // Cache property index and IsWritable info for UpdatePatch
    class PropertyValueInfo
    {
        enum CacheInfoFlag
        {
            preventFalseReferenceFlag = 0x1, // avoid false positive for GC
            disablePrototypeCacheFlag = 0x2,
            enableStoreFieldCacheFlag = 0x4,
            defaultInfoFlags = preventFalseReferenceFlag | enableStoreFieldCacheFlag
        };

    private:
        RecyclableObject* m_instance;    // Slot owner instance
        PropertyIndex m_propertyIndex;   // Slot index on m_instance for the property, or NoSlot to indicate the object can't cache
        PropertyAttributes m_attributes; // Attributes of the property -- only Writable is used
        InlineCacheFlags flags;
        CacheInfoFlag cacheInfoFlag;
        InlineCache* inlineCache;
        PolymorphicInlineCache * polymorphicInlineCache;
        FunctionBody * functionBody;

        RecyclableObject* prop; // Symbol or PropertyString associated with this property
        PropertyRecordUsageCache* propertyRecordUsageCache; // Usage cache for the Symbol or PropertyString `prop` (interior pointer).

        uint inlineCacheIndex;
        bool isFunctionPIC;
        bool allowResizingPolymorphicInlineCache;

        void Set(RecyclableObject* instance, PropertyIndex propertyIndex, PropertyAttributes attributes, InlineCacheFlags flags)
        {
            m_instance = instance;
            m_propertyIndex = propertyIndex;
            m_attributes = attributes;
            this->flags = flags;
        }

        void SetInfoFlag(CacheInfoFlag newFlag)  { cacheInfoFlag = (CacheInfoFlag)(cacheInfoFlag | newFlag); }
        void ClearInfoFlag(CacheInfoFlag newFlag)  { cacheInfoFlag = (CacheInfoFlag)(cacheInfoFlag & ~newFlag); }
        BOOL IsInfoFlagSet(CacheInfoFlag checkFlag) const { return (cacheInfoFlag & checkFlag) == checkFlag; }

    public:
        PropertyValueInfo()
            : m_instance(NULL), m_propertyIndex(Constants::NoSlot), m_attributes(PropertyNone), flags(InlineCacheNoFlags),
            cacheInfoFlag(CacheInfoFlag::defaultInfoFlags), inlineCache(nullptr), polymorphicInlineCache(nullptr),
            prop(nullptr), propertyRecordUsageCache(nullptr), functionBody(nullptr),
            inlineCacheIndex(Constants::NoInlineCacheIndex), allowResizingPolymorphicInlineCache(true)
        {
        }

        RecyclableObject* GetInstance() const       { return m_instance; }
        PropertyIndex GetPropertyIndex() const      { return m_propertyIndex; }
        bool IsWritable() const                     { return (m_attributes & PropertyWritable) != 0; }
        bool IsEnumerable() const                   { return (m_attributes & PropertyEnumerable) != 0; }
        bool IsNoCache() const                      { return m_instance && m_propertyIndex == Constants::NoSlot; }
        void AddFlags(InlineCacheFlags newFlag)     { flags = (InlineCacheFlags)(flags | newFlag); }
        InlineCacheFlags GetFlags() const           { return flags; }
        PropertyAttributes GetAttributes() const    { return m_attributes; }

        // Set property index and IsWritable cache info
        static void Set(PropertyValueInfo* info, RecyclableObject* instance, PropertyIndex propertyIndex, PropertyAttributes attributes = PropertyWritable,
            InlineCacheFlags flags = InlineCacheNoFlags)
        {
            if (info)
            {
                info->Set(instance, propertyIndex, attributes, flags);
            }
        }

        static void SetCacheInfo(PropertyValueInfo* info, InlineCache *const inlineCache);
        static void SetCacheInfo(PropertyValueInfo* info, FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, const bool allowResizingPolymorphicInlineCache);
        static void SetCacheInfo(PropertyValueInfo* info, FunctionBody *const functionBody, PolymorphicInlineCache *const polymorphicInlineCache, const InlineCacheIndex inlineCacheIndex, const bool allowResizingPolymorphicInlineCache);
        template <typename TProperty> static void SetCacheInfo(
            _Out_ PropertyValueInfo* info,
            _In_opt_ TProperty * prop,
            _In_ PolymorphicInlineCache *const polymorphicInlineCache,
            bool allowResizing)
        {
            SetCacheInfo(info, prop, prop->GetPropertyRecordUsageCache(), polymorphicInlineCache, allowResizing);
        }
        static void SetCacheInfo(
            _Out_ PropertyValueInfo* info,
            _In_opt_ RecyclableObject * prop,
            _In_opt_ PropertyRecordUsageCache *const propertyRecordUsageCache,
            _In_ PolymorphicInlineCache *const polymorphicInlineCache,
            bool allowResizing);
        static void SetCacheInfo(_Out_ PropertyValueInfo* info, _In_ PolymorphicInlineCache *const polymorphicInlineCache, bool allowResizing);
        static void ClearCacheInfo(PropertyValueInfo* info);

        InlineCache * GetInlineCache() const
        {
            return this->inlineCache;
        }

        PolymorphicInlineCache * GetPolymorphicInlineCache() const
        {
            return this->polymorphicInlineCache;
        }

        FunctionBody * GetFunctionBody() const
        {
            return this->functionBody;
        }

        PropertyRecordUsageCache * GetPropertyRecordUsageCache() const
        {
            return this->propertyRecordUsageCache;
        }

        RecyclableObject * GetProperty() const
        {
            return this->prop;
        }

        uint GetInlineCacheIndex() const
        {
            return this->inlineCacheIndex;
        }

        bool AllowResizingPolymorphicInlineCache() const
        {
            return allowResizingPolymorphicInlineCache;
        }

        // Set to indicate the instance can't cache property index / IsWritable
        static void SetNoCache(PropertyValueInfo* info, RecyclableObject* instance)
        {
            Set(info, instance, Constants::NoSlot, PropertyNone, InlineCacheNoFlags);
        }

        static void DisablePrototypeCache(PropertyValueInfo* info, RecyclableObject* instance)
        {
            if (info)
            {
                info->SetInfoFlag(disablePrototypeCacheFlag);
            }
        }

        static bool PrototypeCacheDisabled(const PropertyValueInfo* info)
        {
            return (info != NULL) && !!info->IsInfoFlagSet(disablePrototypeCacheFlag);
        }

        static void DisableStoreFieldCache(PropertyValueInfo* info)
        {
            if (info)
            {
                info->ClearInfoFlag(enableStoreFieldCacheFlag);
            }
        }

        static bool IsStoreFieldCacheEnabled(const PropertyValueInfo* info)
        {
            return (info != NULL) && !!info->IsInfoFlagSet(enableStoreFieldCacheFlag);
        }

        bool IsStoreFieldCacheEnabled() const
        {
            return IsStoreFieldCacheEnabled(this);
        }

    };

    enum SideEffects : byte
    {
       SideEffects_None     = 0,
       SideEffects_MathFunc = 0x1,
       SideEffects_ValueOf  = 0x2,
       SideEffects_ToString = 0x4,
       SideEffects_Accessor = 0x8,

       SideEffects_ToPrimitive = SideEffects_ValueOf | SideEffects_ToString,
       SideEffects_Any      = SideEffects_MathFunc | SideEffects_ValueOf | SideEffects_ToString | SideEffects_Accessor
    };

    // int32 is used in JIT code to pass the flag
    // Used to tweak type system methods behavior.
    // Normally, use: PropertyOperation_None.
    enum PropertyOperationFlags : int32
    {
        PropertyOperation_None                          = 0x00,
        PropertyOperation_StrictMode                    = 0x01,
        PropertyOperation_Root                          = 0x02,  // Operation doesn't specify base

        // In particular, used by SetProperty/WithAttributes to throw, rather than return false, when then instance object is not extensible.
        PropertyOperation_ThrowIfNotExtensible          = 0x04,

        // Intent: avoid any checks and force the operation.
        // In particular, used by SetProperty/WithAttributes to force adding a property when an object is not extensible.
        PropertyOperation_Force                         = 0x08,

        // Initializing a property with a special internal value, which the user's code will never see.
        PropertyOperation_SpecialValue                  = 0x10,

        // Pre-initializing a property value before the user's code actually does.
        PropertyOperation_PreInit                       = 0x20,

        // Don't mark this fields as fixed in the type handler.
        PropertyOperation_NonFixedValue                 = 0x40,

        PropertyOperation_PreInitSpecialValue           = PropertyOperation_PreInit | PropertyOperation_SpecialValue,

        PropertyOperation_StrictModeRoot                = PropertyOperation_StrictMode | PropertyOperation_Root,

        // No need to check for undeclared let/const (as this operation is initializing the let/const)
        PropertyOperation_AllowUndecl                   = 0x80,

        // No need to check for undeclared let/const in case of console scope (as this operation is initializing the let/const)
        PropertyOperation_AllowUndeclInConsoleScope     = 0x100,

        PropertyOperation_ThrowIfNonWritable            = 0x200,

        // This will be passed during delete operation. This will make the delete operation throw when the property not configurable.
        PropertyOperation_ThrowOnDeleteIfNotConfig      = 0x400,
    };

    enum class PropertyQueryFlags : int
    {
        Property_NotFound                               = 0,
        Property_Found                                  = 1,
        Property_NotFound_NoProto                       = 2
    };

    class RecyclableObject : public FinalizableObject
    {
        friend class JavascriptOperators;
#if DBG
    public:
        DECLARE_VALIDATE_VTABLE_REGISTERED_NOBASE(RecyclableObject);
#endif
#if DBG || defined(PROFILE_TYPES)
    protected:
        RecyclableObject(DynamicType * type, ScriptContext * scriptContext);

    private:
        void RecordAllocation(ScriptContext * scriptContext);
#endif
    protected:
        Field(Type *) type;
        DEFINE_VTABLE_CTOR_NOBASE(RecyclableObject);

        virtual RecyclableObject* GetPrototypeSpecial();

    public:
        RecyclableObject(Type * type);
#if DBG_EXTRAFIELD
        // This dtor should only be call when OOM occurs and RecyclableObject ctor has completed
        // as the base class, or we have a stack instance
        ~RecyclableObject() { dtorCalled = true; }
#endif
        ScriptContext* GetScriptContext() const;
        TypeId GetTypeId() const;
        RecyclableObject* GetPrototype() const;
        JavascriptMethod GetEntryPoint() const;
        JavascriptLibrary* GetLibrary() const;
        Recycler* GetRecycler() const;
        void SetIsPrototype();

        // Is this object known to have only writable data properties
        // (i.e. no accessors or non-writable properties)?
        bool HasOnlyWritableDataProperties();

        bool HasAnySpecialProperties();

        void ClearWritableDataOnlyDetectionBit();
        bool IsWritableDataOnlyDetectionBitSet();

        inline Type * GetType() const { return type; }

        // In order to avoid a branch, every object has an entry point if it gets called like a
        // function - however, if it can't be called like a function, it's set to DefaultEntryPoint
        // which will emit an error.
        static Var DefaultEntryPoint(RecyclableObject* function, CallInfo callInfo, ...);

        BOOL HasItem(uint32 index);
        BOOL HasProperty(PropertyId propertyId);
        BOOL GetProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext);
        BOOL GetProperty(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext);
        BOOL GetPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext);
        BOOL GetItem(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext);
        BOOL GetItemReference(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext);

        virtual PropertyId GetPropertyId(PropertyIndex index) { return Constants::NoProperty; }
        virtual PropertyId GetPropertyId(BigPropertyIndex index) { return Constants::NoProperty; }
        virtual PropertyIndex GetPropertyIndex(PropertyId propertyId) { return Constants::NoSlot; }
        virtual int GetPropertyCount() { return 0; }
        virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId, _Inout_opt_ PropertyValueInfo* info);
        virtual BOOL HasOwnProperty( PropertyId propertyId);
        virtual BOOL HasOwnPropertyNoHostObject( PropertyId propertyId);
        virtual BOOL HasOwnPropertyCheckNoRedecl( PropertyId propertyId) { Assert(FALSE); return FALSE; }
        virtual BOOL UseDynamicObjectForNoHostObjectAccess() { return FALSE; }
        virtual DescriptorFlags GetSetter(PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) { return None; }
        virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) { return None; }
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext);
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext);
        virtual BOOL GetInternalProperty(Var instance, PropertyId internalPropertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext);
        _Check_return_ _Success_(return) virtual BOOL GetAccessors(PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter, ScriptContext * requestContext);
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext);
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info);
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info);
        virtual BOOL SetInternalProperty(PropertyId internalPropertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info);
        virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = NULL);
        virtual BOOL InitPropertyInEval(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = NULL);
        virtual BOOL EnsureProperty(PropertyId propertyId);
        virtual BOOL EnsureNoRedeclProperty(PropertyId propertyId);
        virtual BOOL SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any);
        virtual BOOL InitPropertyScoped(PropertyId propertyId, Var value);
        virtual BOOL InitFuncScoped(PropertyId propertyId, Var value);
        virtual BOOL DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags);
        virtual BOOL DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags);
#if ENABLE_FIXED_FIELDS
        virtual BOOL IsFixedProperty(PropertyId propertyId);
#endif
        virtual PropertyQueryFlags HasItemQuery(uint32 index);
        virtual BOOL HasOwnItem(uint32 index);
        virtual PropertyQueryFlags GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext);
        virtual PropertyQueryFlags GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext);
        virtual DescriptorFlags GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext) { return None; }
        virtual BOOL SetItem(uint32 index, Var value, PropertyOperationFlags flags);
        virtual BOOL DeleteItem(uint32 index, PropertyOperationFlags flags);
        virtual BOOL GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, EnumeratorCache * enumeratorCache = nullptr);
        virtual BOOL ToPrimitive(JavascriptHint hint, Var* value, ScriptContext * requestContext);
        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None);
        virtual BOOL Equals(__in Var other, __out BOOL* value, ScriptContext* requestContext);
        virtual BOOL StrictEquals(__in Var other, __out BOOL* value, ScriptContext* requestContext);
        virtual BOOL IsWritable(PropertyId propertyId) { return false; }
        virtual BOOL IsConfigurable(PropertyId propertyId) { return false; }
        virtual BOOL IsEnumerable(PropertyId propertyId) { return false; }
        virtual BOOL IsExtensible() { return false; }
        virtual BOOL IsProtoImmutable() const { return false; }
        virtual BOOL PreventExtensions() { return false; };     // Sets [[Extensible]] flag of instance to false
        virtual void ThrowIfCannotDefineProperty(PropertyId propId, const PropertyDescriptor& descriptor);
        virtual BOOL Seal() { return false; }                   // Seals the instance, no additional property can be added or deleted
        virtual BOOL Freeze() { return false; }                 // Freezes the instance, no additional property can be added or deleted or written
        virtual BOOL IsSealed() { return false; }
        virtual BOOL IsFrozen() { return false; }
        virtual BOOL SetWritable(PropertyId propertyId, BOOL value) { return false; }
        virtual BOOL SetConfigurable(PropertyId propertyId, BOOL value) { return false; }
        virtual BOOL SetEnumerable(PropertyId propertyId, BOOL value) { return false; }
        virtual BOOL SetAttributes(PropertyId propertyId, PropertyAttributes attributes) { return false; }

        virtual BOOL GetSpecialPropertyName(uint32 index, JavascriptString ** propertyName, ScriptContext * requestContext) { return false; }
        virtual uint GetSpecialPropertyCount() const { return 0; }
        virtual PropertyId const * GetSpecialPropertyIds() const { return nullptr; }
        RecyclableObject * GetUnwrappedObject();
        virtual RecyclableObject* GetThisAndUnwrappedInstance(Var* thisVar) const; // Due to the withScope object there are times we need to unwrap

        virtual BOOL HasInstance(Var instance, ScriptContext* scriptContext, IsInstInlineCache* inlineCache = NULL);

        BOOL SkipsPrototype() const;
        BOOL IsExternal() const;
        // Used only in JsVarToExtension where it may be during dispose and the type is not available
        virtual BOOL IsExternalVirtual() const { return FALSE; }

        virtual RecyclableObject* GetConfigurablePrototype(ScriptContext * requestContext) { return GetPrototype(); }
        virtual Js::JavascriptString* GetClassName(ScriptContext * requestContext);
        virtual RecyclableObject* GetProxiedObjectForHeapEnum();

#if DBG
        virtual bool CanStorePropertyValueDirectly(PropertyId propertyId, bool allowLetConst) { Assert(false); return false; };
#endif

        virtual void RemoveFromPrototype(ScriptContext * requestContext, bool * allProtoCachesInvalidated) { AssertMsg(false, "Shouldn't call this implementation."); }
        virtual void AddToPrototype(ScriptContext * requestContext, bool * allProtoCachesInvalidated) { AssertMsg(false, "Shouldn't call this implementation."); }
        virtual void SetPrototype(RecyclableObject* newPrototype) { AssertMsg(false, "Shouldn't call this implementation."); }
        virtual bool ClearProtoCachesWereInvalidated() { AssertMsg(false, "Shouldn't call this implementation."); return false; }

        virtual BOOL ToString(Js::Var* value, Js::ScriptContext* scriptContext) { AssertMsg(FALSE, "Do not use this function."); return false; }

        // don't need cross-site: in HostDispatch it's IDispatchEx based; in CustomExternalObject we have marshalling code explicitly.
        virtual Var GetNamespaceParent(Js::Var aChild) { return nullptr; }
        virtual HRESULT QueryObjectInterface(REFIID riid, void **ppvObj);

        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext);
        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext);
        virtual RecyclableObject* ToObject(ScriptContext * requestContext);
        virtual Var GetTypeOfString(ScriptContext* requestContext);

        virtual BOOL GetRemoteTypeId(TypeId* typeId);

        // Only implemented by the HostDispatch object for cross-thread support
        // Only supports a subset of entry points to be called remotely.
        // For a list of supported entry points see the BuiltInOperation enum defined in JscriptInfo.idl
        virtual BOOL InvokeBuiltInOperationRemotely(JavascriptMethod entryPoint, Arguments args, Var* result) { return FALSE; };

        // don't need cross-site: only supported in HostDispatch.
        virtual DynamicObject* GetRemoteObject();

        // don't need cross-site: get the HostDispatch for global object/module root. don't need marshalling.
        virtual Var GetHostDispatchVar();

        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext);

        // If dtor is called, that means that OOM happened (mostly), then the vtable might not be initialized
        // to the base class', so we can't assert.
        virtual void Finalize(bool isShutdown) override {
#ifdef DBG_EXTRAFIELD
            AssertMsg(dtorCalled, "Can't allocate a finalizable object without implementing Finalize");
#endif
        }
        virtual void Dispose(bool isShutdown) override {
#ifdef DBG_EXTRAFIELD
            AssertMsg(dtorCalled, "Can't allocate a finalizable object without implementing Dispose");
#endif
        }
        virtual void Mark(Recycler *recycler) override { AssertMsg(false, "Mark called on object that isn't TrackableObject"); }

        static uint32 GetOffsetOfType() { return offsetof(RecyclableObject, type); }

        virtual void InvalidateCachedScope() { return; }
        virtual BOOL HasDeferredTypeHandler() const { return false; }
#if DBG
    public:
        // Used to Assert that the object may safely be cast to a DynamicObject
        virtual bool DbgIsDynamicObject() const { return false; }
        virtual BOOL DbgSkipsPrototype() const { return FALSE; }
#endif
#if defined(PROFILE_RECYCLER_ALLOC) && defined(RECYCLER_DUMP_OBJECT_GRAPH)
    public:
        static bool DumpObjectFunction(type_info const * typeinfo, bool isArray, void * objectAddress);
#endif

#if ENABLE_TTD
    public:
        //Do any additional marking that is needed for a TT snapshotable object
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
        {
            ;
        }

        //Do the path processing for our "core path" computation to find wellknown objects in a brute force manner.
        virtual void ProcessCorePaths()
        {
            ;
        }

        //Get the SnapObjectType tag that this object maps to
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const;

        //Do the extraction of the SnapObject for each of the kinds of objects in the heap
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc);
#endif

    private:

#if DBG_EXTRAFIELD
        bool dtorCalled;
#endif
        friend class LowererMD;
        friend class LowererMDArch;
        friend struct InlineCache;

#ifdef HEAP_ENUMERATION_VALIDATION
    private:
        UINT m_heapEnumValidationCookie;
    public:
        void SetHeapEnumValidationCookie(int cookie ) { m_heapEnumValidationCookie = cookie; }
        int GetHeapEnumValidationCookie() { return m_heapEnumValidationCookie; }
#endif
    };

    // DO specialize this method; DON'T call it directly (use VarIs instead)
    // Return whether the given RecyclableObject is of the template parameter's type.
    // Generally, subclasses of RecyclableObject should only need to provide
    // a specialization for VarIsImpl(RecyclableObject*), and the other conversion
    // functions should take care of themselves.
    template <typename T> bool VarIsImpl(RecyclableObject* obj);

    template <> inline bool VarIsImpl<RecyclableObject>(RecyclableObject* obj) { return true; }

    // Return whether the given Var is of the template parameter's type.
    template <typename T, typename U> bool VarIs(U* obj)
    {
        // ChakraFull can't include type_traits, but ChakraCore does include it for debug builds
#if DBG && !defined(NTBUILD)
        static_assert(!std::is_same<T, U>::value, "Check should be unnecessary - did you prematurely cast?");
        static_assert(std::is_base_of<U, T>::value, "VarIs/VarTo should only downcast!");
#endif
        return VarIsImpl<T>(obj);
    }

    // Return whether the given Var is of the template parameter's type.
    template <typename T> bool VarIs(Var aValue)
    {
        AssertMsg(aValue != nullptr, "VarIs: aValue is null");

#if INT32VAR
        bool isRecyclableObject = (((uintptr_t)aValue) >> VarTag_Shift) == 0;
#else
        bool isRecyclableObject = (((uintptr_t)aValue) & AtomTag) == AtomTag_Object;
#endif

        return isRecyclableObject && VarIsImpl<T>(reinterpret_cast<RecyclableObject*>(aValue));
    }

    // Validate that the object is actually the type that the type system thinks it is.
    // This should only be used for extremely defensive assertions; if you find code
    // relying on this behavior for correctness, then it's cause for concern.
    template <typename T> bool VarIsCorrectType(T* obj)
    {
        return VarIsImpl<T>(obj);
    }
    template <typename T> bool VarIsCorrectType(WriteBarrierPtr<T> obj)
    {
        return VarIsImpl<T>(obj);
    }

    CompileAssertMsg(AtomTag_Object == 0, "Ensure GC objects do not need to be marked");

    // Cast the input parameter to another type, or crash if the cast is invalid.
    template <typename T, typename U> T* VarTo(U* obj)
    {
        AssertOrFailFast(VarIs<T>(obj));
        return static_cast<T*>(obj);
    }

    // Cast the input parameter to another type, or crash if the cast is invalid.
    template <typename T> T* VarTo(Var aValue)
    {
        AssertOrFailFast(VarIs<T>(aValue));
        return reinterpret_cast<T*>(aValue);
    }

    // Cast the input parameter to another type. In debug builds only, assert that the cast is valid.
    template <typename T, typename U> T* UnsafeVarTo(U* obj)
    {
        Assert(VarIs<T>(obj));
        return static_cast<T*>(obj);
    }

    // Cast the input parameter to another type. In debug builds only, assert that the cast is valid.
    template <typename T> T* UnsafeVarTo(Var aValue)
    {
        Assert(VarIs<T>(aValue));
        return reinterpret_cast<T*>(aValue);
    }
}
