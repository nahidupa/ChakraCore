//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// Must be a power of 2
#define TypePropertyCache_NumElements 16

namespace Js
{
    struct PropertyCacheOperationInfo;

    class TypePropertyCacheElement
    {
    private:
        Field(DynamicObject *) prototypeObjectWithProperty;

        // tag the bit fields to avoid false positive
        Field(bool) tag : 1;
        Field(bool) isInlineSlot : 1;
        Field(bool) isSetPropertyAllowed : 1;
        Field(bool) isMissing : 1;
        Field(PropertyIndex) index;
        Field(PropertyId) id;

    public:
        TypePropertyCacheElement();

    public:
        PropertyId Id() const;
        PropertyIndex Index() const;
        bool IsInlineSlot() const;
        bool IsSetPropertyAllowed() const;
        bool IsMissing() const;
        DynamicObject *PrototypeObjectWithProperty() const;

        void Cache(const PropertyId id, const PropertyIndex index, const bool isInlineSlot, const bool isSetPropertyAllowed);
        void Cache(
            const PropertyId id,
            const PropertyIndex index,
            const bool isInlineSlot,
            const bool isSetPropertyAllowed,
            const bool isMissing,
            DynamicObject *const prototypeObjectWithProperty,
            Type *const myParentType);
        void Clear();
    };

    class TypePropertyCache
    {
    private:
        Field(TypePropertyCacheElement) elements[TypePropertyCache_NumElements];

    private:
        static size_t ElementIndex(const PropertyId id);
        bool TryGetIndexForLoad(const bool checkMissing, const PropertyId id, PropertyIndex *const index, bool *const isInlineSlot, bool *const isMissing, DynamicObject * *const prototypeObjectWithProperty) const;
        bool TryGetIndexForStore(const PropertyId id, PropertyIndex *const index, bool *const isInlineSlot) const;

    public:
        template <bool OutputExistence /*When set, propertyValue represents whether the property exists on the instance, not its actual value*/>
        bool TryGetProperty(const bool checkMissing, RecyclableObject *const propertyObject, const PropertyId propertyId, Var *const propertyValue, ScriptContext *const requestContext, PropertyCacheOperationInfo *const operationInfo, PropertyValueInfo *const propertyValueInfo);
        bool TrySetProperty(RecyclableObject *const object, const PropertyId propertyId, Var propertyValue, ScriptContext *const requestContext, PropertyCacheOperationInfo *const operationInfo, PropertyValueInfo *const propertyValueInfo);

    public:
        void Cache(const PropertyId id, const PropertyIndex index, const bool isInlineSlot, const bool isSetPropertyAllowed);
        void Cache(const PropertyId id, const PropertyIndex index, const bool isInlineSlot, const bool isSetPropertyAllowed, const bool isMissing, DynamicObject *const prototypeObjectWithProperty, Type *const myParentType);
        void ClearIfPropertyIsOnAPrototype(const PropertyId id);
        void Clear(const PropertyId id);
    };
}
