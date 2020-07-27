//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    struct FuncCacheEntry
    {
        Field(ScriptFunction *) func;
        Field(DynamicType *) type;
    };

    class ActivationObject : public DynamicObject
    {
    protected:
        DEFINE_VTABLE_CTOR(ActivationObject, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ActivationObject);
    public:
        ActivationObject(DynamicType * type) : DynamicObject(type)
        {}

        virtual BOOL HasOwnPropertyCheckNoRedecl(PropertyId propertyId) override;
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetInternalProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL EnsureProperty(PropertyId propertyId) override;
        virtual BOOL EnsureNoRedeclProperty(PropertyId propertyId) override;
        virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = NULL) override;
        virtual BOOL InitPropertyScoped(PropertyId propertyId, Var value) override;
        virtual BOOL InitFuncScoped(PropertyId propertyId, Var value) override;
        virtual BOOL DeleteItem(uint32 index, PropertyOperationFlags flags) override;
        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> bool VarIsImpl<ActivationObject>(RecyclableObject* instance);

    // A block-ActivationObject is a scope for an ES6 block that should only receive block-scoped inits,
    // including function, let, and const.
    class BlockActivationObject : public ActivationObject
    {
    private:
        DEFINE_VTABLE_CTOR(BlockActivationObject, ActivationObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(BlockActivationObject);
    public:
        BlockActivationObject(DynamicType * type) : ActivationObject(type) {}

        virtual BOOL EnsureProperty(PropertyId propertyId) override;
        virtual BOOL EnsureNoRedeclProperty(PropertyId propertyId) override;
        virtual BOOL InitPropertyScoped(PropertyId propertyId, Var value) override;
        virtual BOOL InitFuncScoped(PropertyId propertyId, Var value) override;

        BlockActivationObject* Clone(ScriptContext *scriptContext);

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> bool VarIsImpl<BlockActivationObject>(RecyclableObject* instance);

    // A pseudo-ActivationObject is a scope like a "catch" scope that shouldn't receive var inits.
    class PseudoActivationObject : public ActivationObject
    {
    private:
        DEFINE_VTABLE_CTOR(PseudoActivationObject, ActivationObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(PseudoActivationObject);
    public:
        PseudoActivationObject(DynamicType * type) : ActivationObject(type) {}

        virtual BOOL EnsureProperty(PropertyId propertyId) override;
        virtual BOOL EnsureNoRedeclProperty(PropertyId propertyId) override;
        virtual BOOL InitFuncScoped(PropertyId propertyId, Var value) override;
        virtual BOOL InitPropertyScoped(PropertyId propertyId, Var value) override;

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> bool VarIsImpl<PseudoActivationObject>(RecyclableObject* instance);

    class ConsoleScopeActivationObject : public ActivationObject
    {
    private:
        DEFINE_VTABLE_CTOR(ConsoleScopeActivationObject, ActivationObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ConsoleScopeActivationObject);
    public:
        ConsoleScopeActivationObject(DynamicType * type) : ActivationObject(type) {}

        // A dummy function to have a different vtable
        virtual void DummyVirtualFunc(void)
        {
            AssertMsg(false, "ConsoleScopeActivationObject::DummyVirtualFunc function should never be called");
        }

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> bool VarIsImpl<ConsoleScopeActivationObject>(RecyclableObject* instance);

    class ActivationObjectEx : public ActivationObject
    {
    private:
        DEFINE_VTABLE_CTOR(ActivationObjectEx, ActivationObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ActivationObjectEx);

        void GetPropertyCore(PropertyValueInfo *info, ScriptContext *requestContext);
    public:
        ActivationObjectEx(
            DynamicType * type, ScriptFunction *func, uint cachedFuncCount, uint firstFuncSlot, uint lastFuncSlot)
            : ActivationObject(type),
              parentFunc(func),
              cachedFuncCount(cachedFuncCount),
              firstFuncSlot(firstFuncSlot),
              lastFuncSlot(lastFuncSlot),
              committed(false)
        {
            if (cachedFuncCount != 0)
            {
                cache[0].func = nullptr;
            }
        }

        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var *value, PropertyValueInfo *info, ScriptContext *requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var *value, PropertyValueInfo *info, ScriptContext *requestContext) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var *value, PropertyValueInfo *info, ScriptContext *requestContext) override;
        virtual void InvalidateCachedScope() override sealed;

        bool IsCommitted() const { return committed; }
        void SetCommit(bool set) { committed = set; }
        ScriptFunction *GetParentFunc() const { return parentFunc; }
        uint GetFirstFuncSlot() const { return firstFuncSlot; }
        uint GetLastFuncSlot() const { return lastFuncSlot; }
        bool HasCachedFuncs() const { return cachedFuncCount != 0 && cache[0].func != nullptr; }

        void SetCachedFunc(uint i, ScriptFunction *func);

        FuncCacheEntry *GetFuncCacheEntry(uint i)
        {
            Assert(i < cachedFuncCount);
            return &cache[i];
        }

        static uint32 GetOffsetOfCache() { return offsetof(ActivationObjectEx, cache); }
        static uint32 GetOffsetOfCommitFlag() { return offsetof(ActivationObjectEx, committed); }
        static uint32 GetOffsetOfParentFunc() { return offsetof(ActivationObjectEx, parentFunc); }

        static const PropertyId *GetCachedScopeInfo(const PropertyIdArray *propIds);

        // Cached scope info:
        // [0] - cached func count
        // [1] - first func slot
        // [2] - first var slot
        // [3] - literal object reference

        static PropertyId GetCachedFuncCount(const PropertyIdArray *propIds)
        {
            AssertOrFailFast(propIds->extraSlots > 0);
            return ActivationObjectEx::GetCachedScopeInfo(propIds)[0];
        }

        static PropertyId GetFirstFuncSlot(const PropertyIdArray *propIds)
        {
            AssertOrFailFast(propIds->extraSlots > 1);
            return ActivationObjectEx::GetCachedScopeInfo(propIds)[1];
        }

        static PropertyId GetFirstVarSlot(const PropertyIdArray *propIds)
        {
            AssertOrFailFast(propIds->extraSlots > 2);
            return ActivationObjectEx::GetCachedScopeInfo(propIds)[2];
        }

        static PropertyId GetLiteralObjectRef(const PropertyIdArray *propIds)
        {
            AssertOrFailFast(propIds->extraSlots > 3);
            return ActivationObjectEx::GetCachedScopeInfo(propIds)[3];
        }

        static byte ExtraSlotCount() { return 4; }

    private:
        Field(ScriptFunction *) parentFunc;
        Field(uint) cachedFuncCount;
        Field(uint) firstFuncSlot;
        Field(uint) lastFuncSlot;
        Field(bool) committed;
        Field(FuncCacheEntry) cache[1];

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> bool VarIsImpl<ActivationObjectEx>(RecyclableObject* instance);
};
