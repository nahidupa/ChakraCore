//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class ArgumentsObject : public DynamicObject
    {
    private:
        static PropertyId specialPropertyIds[];

    protected:
        DEFINE_VTABLE_CTOR_ABSTRACT(ArgumentsObject, DynamicObject);
    public:
        ArgumentsObject(DynamicType * type) : DynamicObject(type)
        {
            Assert(type->GetTypeId() == TypeIds_Arguments);
        }

        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, EnumeratorCache * enumeratorCache = nullptr) override;

        virtual uint32 GetNumberOfArguments() const = 0;
        virtual uint32 GetNextFormalArgIndex(uint32 index, BOOL enumNonEnumerable = FALSE, PropertyAttributes* attributes = nullptr) const = 0;
        virtual Var GetHeapArguments() = 0;
        virtual void SetHeapArguments(HeapArgumentsObject *args) = 0;
        virtual BOOL AdvanceWalkerToArgsFrame(JavascriptStackWalker *walker) = 0;
    };

    template <> inline bool VarIsImpl<ArgumentsObject>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_Arguments;
    }

    class ES5HeapArgumentsObject;

    // The arguments passed to a function are cached as follows:
    //   1) any formal (i.e. named) arguments are copied to the ActivationObject hung off
    //      a HeapArgumentObject where they can be referenced by name
    //   2) any additional actual arguments are stored as an index property on that HeapArgumentObject
    //
    // See JavascriptOperators::LoadHeapArguments for details of HeapArgumentObject creation.
    //
    // If a function has named arguments, the array elements of the Arguments object are synonymous for
    // the local variables that hold the function arguments. This no longer holds true, however, if an
    // array element of the Argument object is deleted.

    class HeapArgumentsObject : public ArgumentsObject
    {
        friend class ::ActiveScriptProfilerHeapEnum;
    protected:
        DEFINE_VTABLE_CTOR(HeapArgumentsObject, ArgumentsObject);

    private:
        // We currently support only 2^24 arguments
        Field(uint32)              numOfArguments:31;
        Field(uint32)              callerDeleted:1;

        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(HeapArgumentsObject);

    protected:
        Field(uint32)              formalCount;
        Field(ActivationObject*)   frameObject;
        Field(BVSparse<Recycler>*) deletedArgs;

    public:
        HeapArgumentsObject(DynamicType * type);
        HeapArgumentsObject(Recycler *recycler, ActivationObject* obj, uint32 formalCount, DynamicType * type);
        void SetNumberOfArguments(uint32 len);

        static HeapArgumentsObject* As(Var aValue);

        BOOL HasItemAt(uint32 index);
        virtual BOOL GetItemAt(uint32 index, Var *value, ScriptContext * scriptContext);
        virtual BOOL SetItemAt(uint32 index, Var value);
        virtual BOOL DeleteItemAt(uint32 index);

        virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId, _Inout_opt_ PropertyValueInfo* info) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual PropertyQueryFlags HasItemQuery(uint32 index) override;
        virtual PropertyQueryFlags GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual BOOL SetItem(uint32 index, Var value, PropertyOperationFlags flags) override;
        virtual BOOL DeleteItem(uint32 index, PropertyOperationFlags flags) override;

        virtual uint32 GetNumberOfArguments() const override;
        virtual uint32 GetNextFormalArgIndex(uint32 index, BOOL enumNonEnumerable = FALSE, PropertyAttributes* attributes = nullptr) const override;
        virtual Var GetHeapArguments() { return this; }
        virtual void SetHeapArguments(HeapArgumentsObject *args)
        {
            AssertMsg(false, "Should never get here");
        }
        virtual BOOL AdvanceWalkerToArgsFrame(JavascriptStackWalker *walker) override;

        virtual BOOL SetConfigurable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetEnumerable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetWritable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override;
        virtual BOOL SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override;
        virtual BOOL PreventExtensions() override;
        virtual BOOL Seal() override;
        virtual BOOL Freeze() override;

        uint32 GetFormalCount() const
        {
            return this->formalCount;
        }

        void SetFormalCount(uint32 value)
        {
            this->formalCount = value;
        }

        ES5HeapArgumentsObject* ConvertToUnmappedArgumentsObject(bool overwriteArgsUsingFrameObject = true);
        const ActivationObject* GetFrameObject() { return frameObject; }
        void SetFrameObject(ActivationObject * value)
        {
            AssertMsg(frameObject == nullptr, "Setting the frame object again?");
            Assert(!value || VarIsCorrectType(value));
            frameObject = value;
        }

    private:
        ES5HeapArgumentsObject* ConvertToES5HeapArgumentsObject(bool overwriteArgsUsingFrameObject = true);

    protected:
        BOOL IsFormalArgument(uint32 index);
        BOOL IsFormalArgument(PropertyId propertyId);
        BOOL IsFormalArgument(PropertyId propertyId, uint32* pIndex);    // Checks whether property is numeric, and on success sets that index.
        BOOL IsArgumentDeleted(uint32 index) const;

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;

        template <TTD::NSSnapObjects::SnapObjectType argsKind>
        void ExtractSnapObjectDataInto_Helper(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc);

        ES5HeapArgumentsObject* ConvertToES5HeapArgumentsObject_TTD();
#endif
    };

    // ES5 version of the HeapArgumentsObject: support for attributes on formal arguments.
    //   - Unless the user deals with SetWritable/Enumerable/Configurable/Accessors, regular HeapArgumentsObject is used.
    //   - When SetWritable/Enumerable/Configurable/Accessors is called, convert HeapArgumentsObject  to ES5HeapArgumentsObject by swapping vtable.
    //   - Override GetItemAt/SetItemAt -- for disconnected items force GetItem/SetItem to use "this" rather than from frameObject.
    //   - Override GetEnumerator -- take care of enumerable = false arguments.
    //   - The "disconnect" happens when (as ES5 spec says) setting writable to false and setting accessors.
    // Note: This implementation depends on v-table swapping so that HeapArgumentsObject instance can be
    //       converted to ES5HeapArgumentsObject at runtime when ES5 attribute/getter/setter support is needed.
    //       Thus, CAUTION: as a result, this class can't add any new instance fields,
    //       as the size of the instance must be same.
    class ES5HeapArgumentsObject : public HeapArgumentsObject
    {
        friend struct AutoObjectArrayItemExistsValidator;
        friend class ES5ArgumentsObjectEnumerator;

        // Helper class to make sure that object array item exists for ES5HeapArgumentsObject
        // and remove the item to roll back to original state if something fails/throws.
        struct AutoObjectArrayItemExistsValidator
        {
            ES5HeapArgumentsObject* m_args;
            bool m_isReleaseItemNeeded;
            uint32 m_index;

            AutoObjectArrayItemExistsValidator(ES5HeapArgumentsObject* args, uint32 index);
            ~AutoObjectArrayItemExistsValidator();
        };

    private:
        DEFINE_VTABLE_CTOR(ES5HeapArgumentsObject, HeapArgumentsObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ES5HeapArgumentsObject);
        uint32 GetNextFormalArgIndexHelper(uint32 index, BOOL enumNonEnumerable, PropertyAttributes* attributes = nullptr) const;

    public:
        ES5HeapArgumentsObject(Recycler *recycler, ActivationObject* obj, uint32 formalCount, DynamicType * type)
            : HeapArgumentsObject(recycler, obj, formalCount, type)
        {
        }

        virtual BOOL SetConfigurable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetEnumerable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetWritable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags) override;
        virtual BOOL SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override;
        virtual BOOL GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, EnumeratorCache * enumeratorCache = nullptr) override;
        virtual BOOL PreventExtensions() override;
        virtual BOOL Seal() override;
        virtual BOOL Freeze() override;

        virtual uint32 GetNextFormalArgIndex(uint32 index, BOOL enumNonEnumerable = FALSE, PropertyAttributes* attributes = nullptr) const override;
        virtual BOOL GetItemAt(uint32 index, Var *value, ScriptContext * scriptContext) override;
        virtual BOOL SetItemAt(uint32 index, Var value) override;
        virtual BOOL DeleteItemAt(uint32 index) override;

        void DisconnectFormalFromNamedArgument(uint32 index);
        BOOL IsFormalDisconnectedFromNamedArgument(uint32 index);
        BOOL IsEnumerableByIndex(uint32 index);

        BOOL SetConfigurableForFormal(uint32 index, PropertyId propertyId, BOOL value);
        BOOL SetEnumerableForFormal(uint32 index, PropertyId propertyId, BOOL value);
        BOOL SetWritableForFormal(uint32 index, PropertyId propertyId, BOOL value);
        BOOL SetAccessorsForFormal(uint32 index, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None);
        BOOL SetPropertyWithAttributesForFormal(uint32 index, PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any);

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
   };
}
