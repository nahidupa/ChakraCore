//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    template<
        bool CheckLocal,
        bool CheckProto,
        bool CheckAccessor,
        bool CheckMissing,
        bool ReturnOperationInfo,
        bool OutputExistence /*When set, propertyValue represents whether the property exists on the instance, not its actual value*/>
    bool InlineCache::TryGetProperty(
        Var const instance,
        RecyclableObject *const propertyObject,
        const PropertyId propertyId,
        Var *const propertyValue,
        ScriptContext *const requestContext,
        PropertyCacheOperationInfo *const operationInfo)
    {
        CompileAssert(CheckLocal || CheckProto || CheckAccessor || CheckMissing);
        Assert(!ReturnOperationInfo || operationInfo);
        CompileAssert(!ReturnOperationInfo || (CheckLocal && CheckProto && CheckAccessor));
        Assert(instance);
        Assert(propertyObject);
        Assert(propertyId != Constants::NoProperty);
        Assert(propertyValue);
        Assert(requestContext);
        DebugOnly(VerifyRegistrationForInvalidation(this, requestContext, propertyId));

        Type *const type = propertyObject->GetType();

        if (CheckLocal && type == u.local.type)
        {
            OutputPropertyValueAndOperationInfo<OutputExistence, false /*IsMissing*/, ReturnOperationInfo, CacheType_Local, SlotType_Inline>(
                instance, propertyObject, propertyId, propertyValue, requestContext, operationInfo);
            return true;
        }

        if (CheckLocal && TypeWithAuxSlotTag(type) == u.local.type)
        {
            OutputPropertyValueAndOperationInfo<OutputExistence, false /*IsMissing*/, ReturnOperationInfo, CacheType_Local, SlotType_Aux>(
                instance, propertyObject, propertyId, propertyValue, requestContext, operationInfo);
            return true;
        }

        if (CheckProto && type == u.proto.type && !this->u.proto.isMissing)
        {
            OutputPropertyValueAndOperationInfo<OutputExistence, false /*IsMissing*/, ReturnOperationInfo, CacheType_Proto, SlotType_Inline>(
                instance, propertyObject, propertyId, propertyValue, requestContext, operationInfo);
            return true;
        }

        if (CheckProto && TypeWithAuxSlotTag(type) == u.proto.type && !this->u.proto.isMissing)
        {
            OutputPropertyValueAndOperationInfo<OutputExistence, false /*IsMissing*/, ReturnOperationInfo, CacheType_Proto, SlotType_Aux>(
                instance, propertyObject, propertyId, propertyValue, requestContext, operationInfo);
            return true;
        }

        if (CheckAccessor && type == u.accessor.type)
        {
            OutputPropertyValueAndOperationInfo<OutputExistence, false /*IsMissing*/, ReturnOperationInfo, CacheType_Getter, SlotType_Inline>(
                instance, propertyObject, propertyId, propertyValue, requestContext, operationInfo);
            return true;
        }

        if (CheckAccessor && TypeWithAuxSlotTag(type) == u.accessor.type)
        {
            OutputPropertyValueAndOperationInfo<OutputExistence, false /*IsMissing*/, ReturnOperationInfo, CacheType_Getter, SlotType_Aux>(
                instance, propertyObject, propertyId, propertyValue, requestContext, operationInfo);
            return true;
        }

        if (CheckMissing && type == u.proto.type && this->u.proto.isMissing)
        {
            OutputPropertyValueAndOperationInfo<OutputExistence, true /*IsMissing*/, ReturnOperationInfo, CacheType_Proto, SlotType_Inline>(
                instance, propertyObject, propertyId, propertyValue, requestContext, operationInfo);

#ifdef MISSING_PROPERTY_STATS
            if (PHASE_STATS1(MissingPropertyCachePhase))
            {
                requestContext->RecordMissingPropertyHit();
            }
#endif

            return true;
        }

        if (CheckMissing && TypeWithAuxSlotTag(type) == u.proto.type && this->u.proto.isMissing)
        {
            OutputPropertyValueAndOperationInfo<OutputExistence, true /*IsMissing*/, ReturnOperationInfo, CacheType_Proto, SlotType_Aux>(
                instance, propertyObject, propertyId, propertyValue, requestContext, operationInfo);

#ifdef MISSING_PROPERTY_STATS
            if (PHASE_STATS1(MissingPropertyCachePhase))
            {
                requestContext->RecordMissingPropertyHit();
            }
#endif

            return true;
        }

        return false;
    }

    template<> inline void InlineCache::OutputOperationInfo<true>(PropertyCacheOperationInfo *const operationInfo, CacheType cacheType, SlotType slotType)
    {
        operationInfo->cacheType = cacheType;
        operationInfo->slotType = slotType;
    }
    template<> inline void InlineCache::OutputOperationInfo<false>(PropertyCacheOperationInfo *const operationInfo, CacheType cacheType, SlotType slotType)
    {
    }

    template<> inline DynamicObject* InlineCache::GetSourceObject<CacheType::CacheType_Local>(RecyclableObject *const propertyObject)
    {
        return UnsafeVarTo<DynamicObject>(propertyObject);
    }
    template<> inline DynamicObject* InlineCache::GetSourceObject<CacheType::CacheType_Proto>(RecyclableObject *const propertyObject)
    {
        return u.proto.prototypeObject;
    }

    template<> inline RecyclableObject* InlineCache::GetSourceObjectForScriptContext<CacheType::CacheType_Local>(RecyclableObject *const propertyObject)
    {
        return propertyObject;
    }
    template<> inline RecyclableObject* InlineCache::GetSourceObjectForScriptContext<CacheType::CacheType_Proto>(RecyclableObject *const propertyObject)
    {
        return u.proto.prototypeObject;
    }
    template<> inline RecyclableObject* InlineCache::GetSourceObjectForScriptContext<CacheType::CacheType_Getter>(RecyclableObject *const propertyObject)
    {
        return propertyObject;
    }

    template<> inline int InlineCache::GetSlotIndex<CacheType::CacheType_Local>()
    {
        return u.local.slotIndex;
    }
    template<> inline int InlineCache::GetSlotIndex<CacheType::CacheType_Proto>()
    {
        return u.proto.slotIndex;
    }

    template<> inline Var InlineCache::GetPropertyValue<SlotType::SlotType_Inline>(DynamicObject* sourceObject, int slotIndex)
    {
        return sourceObject->GetInlineSlot(slotIndex);
    }
    template<> inline Var InlineCache::GetPropertyValue<SlotType::SlotType_Aux>(DynamicObject* sourceObject, int slotIndex)
    {
        return sourceObject->GetAuxSlot(slotIndex);
    }

    template<
        bool CheckLocal,
        bool CheckLocalTypeWithoutProperty,
        bool CheckAccessor,
        bool ReturnOperationInfo>
    bool InlineCache::TrySetProperty(
        RecyclableObject *const object,
        const PropertyId propertyId,
        Var propertyValue,
        ScriptContext *const requestContext,
        PropertyCacheOperationInfo *const operationInfo,
        const PropertyOperationFlags propertyOperationFlags)
    {
        CompileAssert(CheckLocal || CheckLocalTypeWithoutProperty || CheckAccessor);
        Assert(!ReturnOperationInfo || operationInfo);
        CompileAssert(!ReturnOperationInfo || (CheckLocal && CheckLocalTypeWithoutProperty && CheckAccessor));
        Assert(object);
        Assert(propertyId != Constants::NoProperty);
        Assert(requestContext);
        DebugOnly(VerifyRegistrationForInvalidation(this, requestContext, propertyId));

#if DBG
        const bool isRoot = (propertyOperationFlags & PropertyOperation_Root) != 0;

        bool canSetField; // To verify if we can set a field on the object
        Var setterValue = nullptr;
        {
            // We need to disable implicit call to ensure the check doesn't cause unwanted side effects in debug
            // code Save old disableImplicitFlags and implicitCallFlags and disable implicit call and exception.
            ThreadContext * threadContext = requestContext->GetThreadContext();
            ThreadContext::AutoRestoreImplicitFlags autoRestoreImplicitFlags(threadContext, threadContext->GetImplicitCallFlags(), threadContext->GetDisableImplicitFlags());
            threadContext->ClearImplicitCallFlags();
            threadContext->SetDisableImplicitFlags(DisableImplicitCallAndExceptionFlag);

            DescriptorFlags flags = DescriptorFlags::None;
            canSetField = !JavascriptOperators::CheckPrototypesForAccessorOrNonWritablePropertySlow(object, propertyId, &setterValue, &flags, isRoot, requestContext);
            if (threadContext->GetImplicitCallFlags() != Js::ImplicitCall_None)
            {
                canSetField = true; // If there was an implicit call, inconclusive. Disable debug check.
                setterValue = nullptr;
            }
            else if ((flags & Accessor) == Accessor)
            {
                Assert(setterValue != nullptr);
            }

            // ImplicitCallFlags and DisableImplicitFlags restored by AutoRestoreImplicitFlags' destructor.
        }
#endif

        Type *const type = object->GetType();

        if (CheckLocal && type == u.local.type)
        {
            Assert(object->GetScriptContext() == requestContext); // we never cache a type from another script context
            Assert(isRoot || object->GetPropertyIndex(propertyId) == VarTo<DynamicObject>(object)->GetTypeHandler()->InlineOrAuxSlotIndexToPropertyIndex(u.local.slotIndex, true));
            Assert(!isRoot || VarTo<RootObjectBase>(object)->GetRootPropertyIndex(propertyId) == VarTo<DynamicObject>(object)->GetTypeHandler()->InlineOrAuxSlotIndexToPropertyIndex(u.local.slotIndex, true));
            Assert(object->CanStorePropertyValueDirectly(propertyId, isRoot));
            UnsafeVarTo<DynamicObject>(object)->SetInlineSlot(SetSlotArgumentsRoot(propertyId, isRoot, u.local.slotIndex, propertyValue));
            if (ReturnOperationInfo)
            {
                operationInfo->cacheType = CacheType_Local;
                operationInfo->slotType = SlotType_Inline;
            }
            Assert(canSetField);
            return true;
        }

        if (CheckLocal && TypeWithAuxSlotTag(type) == u.local.type)
        {
            Assert(object->GetScriptContext() == requestContext); // we never cache a type from another script context
            Assert(isRoot || object->GetPropertyIndex(propertyId) == VarTo<DynamicObject>(object)->GetTypeHandler()->InlineOrAuxSlotIndexToPropertyIndex(u.local.slotIndex, false));
            Assert(!isRoot || VarTo<RootObjectBase>(object)->GetRootPropertyIndex(propertyId) == VarTo<DynamicObject>(object)->GetTypeHandler()->InlineOrAuxSlotIndexToPropertyIndex(u.local.slotIndex, false));
            Assert(object->CanStorePropertyValueDirectly(propertyId, isRoot));
            UnsafeVarTo<DynamicObject>(object)->SetAuxSlot(SetSlotArgumentsRoot(propertyId, isRoot, u.local.slotIndex, propertyValue));
            if (ReturnOperationInfo)
            {
                operationInfo->cacheType = CacheType_Local;
                operationInfo->slotType = SlotType_Aux;
            }
            Assert(canSetField);
            return true;
        }

        if (CheckLocalTypeWithoutProperty && type == u.local.typeWithoutProperty)
        {
            // CAREFUL! CheckIfPrototypeChainHasOnlyWritableDataProperties may do allocation that triggers GC and
            // clears this cache, so save any info that is needed from the cache before calling those functions.
            Type *const typeWithProperty = u.local.type;
            const PropertyIndex propertyIndex = u.local.slotIndex;

#if DBG
            uint16 newAuxSlotCapacity = u.local.requiredAuxSlotCapacity;
#endif
            Assert(object->GetScriptContext() == requestContext); // we never cache a type from another script context
            Assert(typeWithProperty);
            Assert(DynamicType::Is(typeWithProperty));
            Assert(((DynamicType*)typeWithProperty)->GetIsShared());
            Assert(((DynamicType*)typeWithProperty)->GetTypeHandler()->IsPathTypeHandler());
            AssertMsg(!((DynamicType*)u.local.typeWithoutProperty)->GetTypeHandler()->GetIsPrototype(), "Why did we cache a property add for a prototype?");
            Assert(((DynamicType*)typeWithProperty)->GetTypeHandler()->CanStorePropertyValueDirectly((const DynamicObject*)object, propertyId, isRoot));

            DynamicObject *const dynamicObject = UnsafeVarTo<DynamicObject>(object);

            // If we're adding a property to an inlined slot, we should never need to adjust auxiliary slot array size.
            Assert(newAuxSlotCapacity == 0);

            dynamicObject->type = typeWithProperty;

            Assert(isRoot || object->GetPropertyIndex(propertyId) == VarTo<DynamicObject>(object)->GetTypeHandler()->InlineOrAuxSlotIndexToPropertyIndex(propertyIndex, true));
            Assert(!isRoot || VarTo<RootObjectBase>(object)->GetRootPropertyIndex(propertyId) == VarTo<DynamicObject>(object)->GetTypeHandler()->InlineOrAuxSlotIndexToPropertyIndex(propertyIndex, true));

            dynamicObject->SetInlineSlot(SetSlotArgumentsRoot(propertyId, isRoot, propertyIndex, propertyValue));

            if (ReturnOperationInfo)
            {
                operationInfo->cacheType = CacheType_LocalWithoutProperty;
                operationInfo->slotType = SlotType_Inline;
            }
            return true;
        }

        if (CheckLocalTypeWithoutProperty && TypeWithAuxSlotTag(type) == u.local.typeWithoutProperty)
        {
            // CAREFUL! CheckIfPrototypeChainHasOnlyWritableDataProperties or AdjustSlots may do allocation that triggers GC and
            // clears this cache, so save any info that is needed from the cache before calling those functions.
            Type *const typeWithProperty = TypeWithoutAuxSlotTag(u.local.type);
            const PropertyIndex propertyIndex = u.local.slotIndex;
            uint16 newAuxSlotCapacity = u.local.requiredAuxSlotCapacity;

            Assert(object->GetScriptContext() == requestContext); // we never cache a type from another script context
            Assert(typeWithProperty);
            Assert(DynamicType::Is(typeWithProperty));
            Assert(((DynamicType*)typeWithProperty)->GetIsShared());
            Assert(((DynamicType*)typeWithProperty)->GetTypeHandler()->IsPathTypeHandler());
            AssertMsg(!((DynamicType*)TypeWithoutAuxSlotTag(u.local.typeWithoutProperty))->GetTypeHandler()->GetIsPrototype(), "Why did we cache a property add for a prototype?");
            Assert(((DynamicType*)typeWithProperty)->GetTypeHandler()->CanStorePropertyValueDirectly((const DynamicObject*)object, propertyId, isRoot));

            DynamicObject *const dynamicObject = UnsafeVarTo<DynamicObject>(object);

            if (newAuxSlotCapacity > 0)
            {
                DynamicTypeHandler::AdjustSlots(
                    dynamicObject,
                    static_cast<DynamicType *>(typeWithProperty)->GetTypeHandler()->GetInlineSlotCapacity(),
                    newAuxSlotCapacity);
            }

            dynamicObject->type = typeWithProperty;

            Assert(isRoot || object->GetPropertyIndex(propertyId) == VarTo<DynamicObject>(object)->GetTypeHandler()->InlineOrAuxSlotIndexToPropertyIndex(propertyIndex, false));
            Assert(!isRoot || VarTo<RootObjectBase>(object)->GetRootPropertyIndex(propertyId) == VarTo<DynamicObject>(object)->GetTypeHandler()->InlineOrAuxSlotIndexToPropertyIndex(propertyIndex, false));

            dynamicObject->SetAuxSlot(SetSlotArgumentsRoot(propertyId, isRoot, propertyIndex, propertyValue));

            if (ReturnOperationInfo)
            {
                operationInfo->cacheType = CacheType_LocalWithoutProperty;
                operationInfo->slotType = SlotType_Aux;
            }
            return true;
        }

        if (CheckAccessor && type == u.accessor.type)
        {
            Assert(object->GetScriptContext() == requestContext); // we never cache a type from another script context
            Assert(u.accessor.flags & InlineCacheSetterFlag);

            RecyclableObject * function;
            if (u.accessor.isOnProto)
            {
                function = UnsafeVarTo<RecyclableObject>(u.accessor.object->GetInlineSlot(u.accessor.slotIndex));
            }
            else
            {
                function = UnsafeVarTo<RecyclableObject>(VarTo<DynamicObject>(object)->GetInlineSlot(u.accessor.slotIndex));
            }

            Assert(setterValue == nullptr || setterValue == function);

            if (!JavascriptError::ThrowIfStrictModeUndefinedSetter(propertyOperationFlags, function, requestContext) &&
                !JavascriptError::ThrowIfNotExtensibleUndefinedSetter(propertyOperationFlags, function, requestContext))
            {
                Js::JavascriptOperators::CallSetter(function, object, propertyValue, requestContext);
            }

            if (ReturnOperationInfo)
            {
                operationInfo->cacheType = CacheType_Setter;
                operationInfo->slotType = SlotType_Inline;
            }
            return true;
        }

        if (CheckAccessor && TypeWithAuxSlotTag(type) == u.accessor.type)
        {
            Assert(object->GetScriptContext() == requestContext); // we never cache a type from another script context
            Assert(u.accessor.flags & InlineCacheSetterFlag);

            RecyclableObject * function;
            if (u.accessor.isOnProto)
            {
                function = UnsafeVarTo<RecyclableObject>(u.accessor.object->GetAuxSlot(u.accessor.slotIndex));
            }
            else
            {
                function = UnsafeVarTo<RecyclableObject>(VarTo<DynamicObject>(object)->GetAuxSlot(u.accessor.slotIndex));
            }

            Assert(setterValue == nullptr || setterValue == function);

            if (!JavascriptError::ThrowIfStrictModeUndefinedSetter(propertyOperationFlags, function, requestContext) &&
                !JavascriptError::ThrowIfNotExtensibleUndefinedSetter(propertyOperationFlags, function, requestContext))
            {
                Js::JavascriptOperators::CallSetter(function, object, propertyValue, requestContext);
            }

            if (ReturnOperationInfo)
            {
                operationInfo->cacheType = CacheType_Setter;
                operationInfo->slotType = SlotType_Aux;
            }
            return true;
        }

        return false;
    }

    template<
        bool CheckLocal,
        bool CheckProto,
        bool CheckAccessor>
    void PolymorphicInlineCache::CloneInlineCacheToEmptySlotInCollision(Type * const type, uint inlineCacheIndex)
    {
        if (CheckLocal && (inlineCaches[inlineCacheIndex].u.local.type == type || inlineCaches[inlineCacheIndex].u.local.type == TypeWithAuxSlotTag(type)))
        {
            return;
        }
        if (CheckProto && (inlineCaches[inlineCacheIndex].u.proto.type == type || inlineCaches[inlineCacheIndex].u.proto.type == TypeWithAuxSlotTag(type)))
        {
            return;
        }
        if (CheckAccessor && (inlineCaches[inlineCacheIndex].u.accessor.type == type || inlineCaches[inlineCacheIndex].u.accessor.type == TypeWithAuxSlotTag(type)))
        {
            return;
        }

        if (this->IsFull())
        {
            // If the cache is full, we won't find an empty slot to move the contents of the colliding inline cache to.
            return;
        }

        // Collision is with a cache having a different type.

        uint tryInlineCacheIndex = GetNextInlineCacheIndex(inlineCacheIndex);

        // Iterate over the inline caches in the polymorphic cache, stop when:
        //   1. an empty inline cache is found, or
        //   2. a cache already populated with the incoming type is found, or
        //   3. all the inline caches have been looked at.
        while (!inlineCaches[tryInlineCacheIndex].IsEmpty() && tryInlineCacheIndex != inlineCacheIndex)
        {
            if (CheckLocal && (inlineCaches[tryInlineCacheIndex].u.local.type == type || inlineCaches[tryInlineCacheIndex].u.local.type == TypeWithAuxSlotTag(type)))
            {
                break;
            }
            if (CheckProto && (inlineCaches[tryInlineCacheIndex].u.proto.type == type || inlineCaches[tryInlineCacheIndex].u.proto.type == TypeWithAuxSlotTag(type)))
            {
                Assert(GetInlineCacheIndexForType(inlineCaches[tryInlineCacheIndex].u.proto.type) == inlineCacheIndex);
                break;
            }
            if (CheckAccessor && (inlineCaches[tryInlineCacheIndex].u.accessor.type == type || inlineCaches[tryInlineCacheIndex].u.accessor.type == TypeWithAuxSlotTag(type)))
            {
                Assert(GetInlineCacheIndexForType(inlineCaches[tryInlineCacheIndex].u.accessor.type) == inlineCacheIndex);
                break;
            }
            tryInlineCacheIndex = GetNextInlineCacheIndex(tryInlineCacheIndex);
        }
        if (tryInlineCacheIndex != inlineCacheIndex)
        {
            if (inlineCaches[inlineCacheIndex].invalidationListSlotPtr != nullptr)
            {
                Assert(*(inlineCaches[inlineCacheIndex].invalidationListSlotPtr) == &inlineCaches[inlineCacheIndex]);
                if (inlineCaches[tryInlineCacheIndex].invalidationListSlotPtr != nullptr)
                {
                    Assert(*(inlineCaches[tryInlineCacheIndex].invalidationListSlotPtr) == &inlineCaches[tryInlineCacheIndex]);
                }
                else
                {
                    inlineCaches[tryInlineCacheIndex].invalidationListSlotPtr = inlineCaches[inlineCacheIndex].invalidationListSlotPtr;
                    *(inlineCaches[tryInlineCacheIndex].invalidationListSlotPtr) = &inlineCaches[tryInlineCacheIndex];
                    inlineCaches[inlineCacheIndex].invalidationListSlotPtr = nullptr;
                }
            }
            inlineCaches[tryInlineCacheIndex].u = inlineCaches[inlineCacheIndex].u;
            UpdateInlineCachesFillInfo(tryInlineCacheIndex, true /*set*/);
            // Let's clear the cache slot on which we had the collision.
            inlineCaches[inlineCacheIndex].RemoveFromInvalidationListAndClear(type->GetScriptContext()->GetThreadContext());
            Assert((this->inlineCachesFillInfo & (1 << inlineCacheIndex)) != 0);
            UpdateInlineCachesFillInfo(inlineCacheIndex, false /*set*/);
        }
    }

#ifdef CLONE_INLINECACHE_TO_EMPTYSLOT
    template <typename TDelegate>
    bool PolymorphicInlineCache::CheckClonedInlineCache(uint inlineCacheIndex, TDelegate mapper)
    {
        bool success = false;
        uint tryInlineCacheIndex = GetNextInlineCacheIndex(inlineCacheIndex);
        do
        {
            if (inlineCaches[tryInlineCacheIndex].IsEmpty())
            {
                break;
            }
            success = mapper(tryInlineCacheIndex);
            if (success)
            {
                Assert(inlineCaches[inlineCacheIndex].invalidationListSlotPtr == nullptr || *inlineCaches[inlineCacheIndex].invalidationListSlotPtr == &inlineCaches[inlineCacheIndex]);
                Assert(inlineCaches[tryInlineCacheIndex].invalidationListSlotPtr == nullptr || *inlineCaches[tryInlineCacheIndex].invalidationListSlotPtr == &inlineCaches[tryInlineCacheIndex]);

                // Swap inline caches, including their invalidationListSlotPtrs.
                InlineCache temp = inlineCaches[tryInlineCacheIndex];
                inlineCaches[tryInlineCacheIndex] = inlineCaches[inlineCacheIndex];
                inlineCaches[inlineCacheIndex] = temp;

                // Fix up invalidationListSlotPtrs to point to their owners.
                if (inlineCaches[inlineCacheIndex].invalidationListSlotPtr != nullptr)
                {
                    *inlineCaches[inlineCacheIndex].invalidationListSlotPtr = &inlineCaches[inlineCacheIndex];
                }
                if (inlineCaches[tryInlineCacheIndex].invalidationListSlotPtr != nullptr)
                {
                    *inlineCaches[tryInlineCacheIndex].invalidationListSlotPtr = &inlineCaches[tryInlineCacheIndex];
                }

                break;
            }
            tryInlineCacheIndex = GetNextInlineCacheIndex(tryInlineCacheIndex);

        } while (tryInlineCacheIndex != inlineCacheIndex);

        return success;
    }
#endif

    template<
        bool CheckLocal,
        bool CheckProto,
        bool CheckAccessor,
        bool CheckMissing,
        bool IsInlineCacheAvailable,
        bool ReturnOperationInfo,
        bool OutputExistence /*When set, propertyValue is true or false, representing whether the property exists on the instance not its actual value*/>
    bool PolymorphicInlineCache::TryGetProperty(
        Var const instance,
        RecyclableObject *const propertyObject,
        const PropertyId propertyId,
        Var *const propertyValue,
        ScriptContext *const requestContext,
        PropertyCacheOperationInfo *const operationInfo,
        InlineCache *const inlineCacheToPopulate)
    {
        Assert(!IsInlineCacheAvailable || inlineCacheToPopulate);
        Assert(!ReturnOperationInfo || operationInfo);

        Type * const type = propertyObject->GetType();
        uint inlineCacheIndex = GetInlineCacheIndexForType(type);
        InlineCache *cache = &inlineCaches[inlineCacheIndex];

#ifdef INLINE_CACHE_STATS
        bool isEmpty = false;
        if (PHASE_STATS1(Js::PolymorphicInlineCachePhase))
        {
            isEmpty = cache->IsEmpty();
        }
#endif
        bool result = cache->TryGetProperty<CheckLocal, CheckProto, CheckAccessor, CheckMissing, ReturnOperationInfo, OutputExistence>(
            instance, propertyObject, propertyId, propertyValue, requestContext, operationInfo);

#ifdef CLONE_INLINECACHE_TO_EMPTYSLOT
        if (!result && !cache->IsEmpty())
        {
            result = CheckClonedInlineCache(inlineCacheIndex, [&](uint tryInlineCacheIndex) -> bool
            {
                cache = &inlineCaches[tryInlineCacheIndex];
                return cache->TryGetProperty<CheckLocal, CheckProto, CheckAccessor, CheckMissing, ReturnOperationInfo, OutputExistence>(
                    instance, propertyObject, propertyId, propertyValue, requestContext, operationInfo);
            });
        }
#endif

        if (IsInlineCacheAvailable && result)
        {
            cache->CopyTo(propertyId, requestContext, inlineCacheToPopulate);
        }

#ifdef INLINE_CACHE_STATS
        if (PHASE_STATS1(Js::PolymorphicInlineCachePhase))
        {
            bool collision = !result && !isEmpty;
            GetScriptContext()->LogCacheUsage(this, /*isGet*/ true, propertyId, result, collision);
        }
#endif

        return result;
    }

    template<
        bool CheckLocal,
        bool CheckLocalTypeWithoutProperty,
        bool CheckAccessor,
        bool IsInlineCacheAvailable,
        bool ReturnOperationInfo>
    bool PolymorphicInlineCache::TrySetProperty(
        RecyclableObject *const object,
        const PropertyId propertyId,
        Var propertyValue,
        ScriptContext *const requestContext,
        PropertyCacheOperationInfo *const operationInfo,
        InlineCache *const inlineCacheToPopulate,
        const PropertyOperationFlags propertyOperationFlags)
    {
        Assert(!IsInlineCacheAvailable || inlineCacheToPopulate);
        Assert(!ReturnOperationInfo || operationInfo);

        Type * const type = object->GetType();
        uint inlineCacheIndex = GetInlineCacheIndexForType(type);
        InlineCache *cache = &inlineCaches[inlineCacheIndex];

#ifdef INLINE_CACHE_STATS
        bool isEmpty = false;
        if (PHASE_STATS1(Js::PolymorphicInlineCachePhase))
        {
            isEmpty = cache->IsEmpty();
        }
#endif
        bool result = cache->TrySetProperty<CheckLocal, CheckLocalTypeWithoutProperty, CheckAccessor, ReturnOperationInfo>(
            object, propertyId, propertyValue, requestContext, operationInfo, propertyOperationFlags);

#ifdef CLONE_INLINECACHE_TO_EMPTYSLOT
        if (!result && !cache->IsEmpty())
        {
            result = CheckClonedInlineCache(inlineCacheIndex, [&](uint tryInlineCacheIndex) -> bool
            {
                cache = &inlineCaches[tryInlineCacheIndex];
                return cache->TrySetProperty<CheckLocal, CheckLocalTypeWithoutProperty, CheckAccessor, ReturnOperationInfo>(
                    object, propertyId, propertyValue, requestContext, operationInfo, propertyOperationFlags);
            });
        }
#endif

        if (IsInlineCacheAvailable && result)
        {
            cache->CopyTo(propertyId, requestContext, inlineCacheToPopulate);
        }

#ifdef INLINE_CACHE_STATS
        if (PHASE_STATS1(Js::PolymorphicInlineCachePhase))
        {
            bool collision = !result && !isEmpty;
            GetScriptContext()->LogCacheUsage(this, /*isGet*/ false, propertyId, result, collision);
        }
#endif

        return result;
    }
}
