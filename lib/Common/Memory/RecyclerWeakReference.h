//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    template <typename TStrongRef> class RemoteWeakReference;
};

namespace Memory
{
// Forward declarations
template <typename SizePolicy> class WeakReferenceHashTable;
class Recycler;

///
/// This class is represents a weak reference handle object
/// It's a proxy to the actual strong reference itself
/// This is an entry in the weak reference hash table.
/// When a weak reference is created, the strong reference is set to
/// point to an object.
/// When the referenced object is collected, the strong reference is set to NULL
///
/// Clients should not use this class but instead use RecyclerWeakReference<type>
/// which provides for automatic type conversion of the underlying reference
class RecyclerWeakReferenceBase
{
public:
    friend class Recycler;

    template <typename SizePolicy> friend class WeakReferenceHashTable;
    template <typename TStrongRef> friend class Js::RemoteWeakReference;

protected:

    FieldNoBarrier(char*) strongRef;
    FieldNoBarrier(HeapBlock *) strongRefHeapBlock;
    FieldNoBarrier(HeapBlock *) weakRefHeapBlock;
    FieldNoBarrier(RecyclerWeakReferenceBase*) next;
#if DBG
#if ENABLE_RECYCLER_TYPE_TRACKING
    FieldNoBarrier(type_info const *) typeInfo;
#endif

#if defined TRACK_ALLOC && defined(PERF_COUNTERS)
    FieldNoBarrier(PerfCounter::Counter *) counter;
#endif
#endif
};

/// Wrapper class template that can be used to acquire the underlying strong reference from the weak reference
template<typename T>
class RecyclerWeakReference: public RecyclerWeakReferenceBase
{
public:
    // Fast get of the strong reference- this might return a wrong result if the recycler is in sweep so callers
    // should never call this during sweep
    inline T* FastGet() const
    {
        return ((T*) strongRef);
    }

    inline T* Get() const
    {
        char * ref = this->strongRef;
        return (T*)ref;
    }

    inline T** GetAddressOfStrongRef()
    {
        return (T**)&strongRef;
    }

};

template <class T>
class WeakReferenceCache
{
private:
    RecyclerWeakReference<T> * weakReference;
public:
    WeakReferenceCache() : weakReference(nullptr) {};
    RecyclerWeakReference<T> * GetWeakReference(Recycler * recycler)
    {
        RecyclerWeakReference<T> * weakRef = this->weakReference;
        if (weakRef == nullptr)
        {
            weakRef = recycler->CreateWeakReferenceHandle((T*)this);
            this->weakReference = weakRef;
        }
        return weakRef;
    }
};

///
/// Hashtable class that maps strong references to weak references
/// This is slightly unique in that the weak reference is a complete entry in the hashtable
/// but is treated as the value for the client. The key is the strong reference.
/// The hash table is a standard closed-addressing hash table where the strong references are
/// hashed into buckets, and then stored in that buckets corresponding doubly linked list
/// The hash-table is resized when its load factor exceeds a constant
/// The buckets are allocated using the HeapAllocator. Individual entries are recycler allocated.
///
template <typename SizePolicy>
class WeakReferenceHashTable
{
    static const int MaxAverageChainLength = 1;

    HeapAllocator* allocator;
    RecyclerWeakReferenceBase** buckets;
    uint count;
    uint size;
    RecyclerWeakReferenceBase* freeList;
    int modFunctionIndex;

public:
    WeakReferenceHashTable(uint size, HeapAllocator* allocator):
        count(0),
        size(0),
        modFunctionIndex(UNKNOWN_MOD_INDEX),
        allocator(allocator),
        freeList(nullptr)
    {
        this->size = SizePolicy::GetSize(size, &modFunctionIndex);
        buckets = AllocatorNewArrayZ(HeapAllocator, allocator, RecyclerWeakReferenceBase*, this->size);
    }

    ~WeakReferenceHashTable()
    {
        AllocatorDeleteArray(HeapAllocator, allocator,  size, buckets);
    }

    RecyclerWeakReferenceBase* Add(char* strongReference, Recycler * recycler)
    {
        uint targetBucket = HashKeyToBucket(strongReference, size);
        RecyclerWeakReferenceBase* entry = FindEntry(strongReference, targetBucket);
        if (entry != nullptr)
        {
            return entry;
        }

        return Create(strongReference, targetBucket, recycler);
    }

    bool FindOrAdd(char* strongReference, Recycler *recycler, RecyclerWeakReferenceBase **ppWeakRef)
    {
        Assert(ppWeakRef);

        uint targetBucket = HashKeyToBucket(strongReference, size);
        RecyclerWeakReferenceBase* entry = FindEntry(strongReference, targetBucket);
        if (entry != nullptr)
        {
            *ppWeakRef = entry;
            return false;
        }

        entry = Create(strongReference, targetBucket, recycler);
        *ppWeakRef = entry;
        return true;
    }

#ifdef RECYCLER_TRACE_WEAKREF
    void DumpNode(RecyclerWeakReferenceBase* node)
    {
        Output::Print(_u("[ 0x%08x { 0x%08x, 0x%08x }]"), node, node->strongRef, mode->next);
    }

    void Dump()
    {
        RecyclerWeakReferenceBase *current;
        Output::Print(_u("HashTable with %d buckets and %d nodes\n"), this->size, this->count);

        for (uint i=0;i<size;i++) {
            Output::Print(_u("Bucket %d (0x%08x) ==> "), i, &buckets[i]);
            for (current = buckets[i] ; current != nullptr; current = current->next) {
                DumpNode(current);
            }
            Output::Print(_u("\n"));
        }
    }
#endif

    bool TryGetValue(char* strongReference, RecyclerWeakReferenceBase** weakReference)
    {
        RecyclerWeakReferenceBase* current = FindEntry(strongReference, HashKeyToBucket(strongReference, size));
        if (current != nullptr)
        {
            *weakReference = current;
            return true;
        }
        return false;
    }

    void Remove(char* key, RecyclerWeakReferenceBase** pOut)
    {
        uint val = HashKeyToBucket(key, size);
        RecyclerWeakReferenceBase ** pprev = &buckets[val];
        RecyclerWeakReferenceBase *current = *pprev;
        while (current)
        {
            if (DefaultComparer<char*>::Equals(key, current->strongRef))
            {
                 *pprev = current->next;
                if (pOut != nullptr)
                {
                    (*pOut) = current;
                }
                count--;

#ifdef RECYCLER_TRACE_WEAKREF
                Output::Print(_u("Remove 0x%08x to bucket %d, count is %d\n"), current, val, count);
#endif
                break;
            }
            pprev = &current->next;
            current = *pprev;
        }
    }

    uint Count()
    {
        return count;
    }

    void Remove(char* key)
    {
        Remove(key, nullptr);
    }

    template <class Func>
    void Map(Func fn)
    {
        uint removed = 0;
#if DEBUG
        uint countedEntries = 0;
#endif

        for (uint i=0;i<size;i++)
        {
            RecyclerWeakReferenceBase ** pprev = &buckets[i];
            RecyclerWeakReferenceBase *current = *pprev;
            while (current)
            {
                if (fn(current))
                {
                    pprev = &current->next;
#if DEBUG
                    countedEntries++;
#endif
                }
                else
                {
                    // remove
                    *pprev = current->next;
                    removed++;
                }
                current = *pprev;
            }
        }

        Assert(removed <= count);
        count -= removed;

#if DEBUG
        Assert(countedEntries == count);
#endif
    }

private:
    // If density is a compile-time constant, then we can optimize (avoids division)
    // Sometimes the compiler can also make this optimization, but this way is guaranteed.
    template< uint density > bool IsDenserThan() const
    {
        return count > (size * density);
    }

    RecyclerWeakReferenceBase * FindEntry(char* strongReference, uint targetBucket)
    {
        for (RecyclerWeakReferenceBase * current = buckets[targetBucket] ; current != nullptr; current = current->next)
        {
            if (DefaultComparer<char*>::Equals(strongReference, current->strongRef))
            {
                return current;
            }
        }

        return nullptr;
    }

    hash_t HashKeyToBucket(char* strongReference, int size)
    {
        hash_t hashCode = DefaultComparer<char*>::GetHashCode(strongReference);
        return SizePolicy::GetBucket(hashCode, size, modFunctionIndex);
    }

    void AddEntry(RecyclerWeakReferenceBase* entry, RecyclerWeakReferenceBase** bucket)
    {
        RecyclerWeakReferenceBase* first = (*bucket);

        entry->next = first;
        (*bucket) = entry;
    }

    void Resize(int newSize)
    {
#if DEBUG
        uint copiedEntries = 0;
#endif
        RecyclerWeakReferenceBase** newBuckets = AllocatorNewArrayZ(HeapAllocator, allocator, RecyclerWeakReferenceBase*, newSize);

        for (uint i=0; i < size; i++)
        {
            RecyclerWeakReferenceBase* current = buckets[i];
            while (current != nullptr)
            {
                int targetBucket = HashKeyToBucket(current->strongRef, newSize);
                RecyclerWeakReferenceBase* next = current->next; // Cache the next pointer

                AddEntry(current, &newBuckets[targetBucket]);
#if DEBUG
                copiedEntries++;
#endif
                current = next;
            }
        }

        AllocatorDeleteArray(HeapAllocator, allocator, size, buckets);
        size = newSize;
        buckets = newBuckets;
#if DEBUG
        Assert(this->count == copiedEntries);
#endif
    }

    RecyclerWeakReferenceBase* Create(char* strongReference, uint targetBucket, Recycler * recycler)
    {
        Assert(HashKeyToBucket(strongReference, size) == targetBucket);
        Assert(!FindEntry(strongReference, targetBucket));

        if (IsDenserThan<MaxAverageChainLength>())
        {
#ifdef RECYCLER_TRACE_WEAKREF
            Output::Print(_u("Count is %d\n"), this->count);
#endif
            Resize(SizePolicy::GetSize(size*2, &modFunctionIndex));
            // After resize - we will need to recalculate the bucket
            targetBucket = HashKeyToBucket(strongReference, size);
        }

        RecyclerWeakReferenceBase* entry;
        entry = AllocatorNewBase(Recycler, recycler, AllocWeakReferenceEntry, RecyclerWeakReferenceBase);
        entry->strongRef = strongReference;
        entry->strongRefHeapBlock = recycler->FindHeapBlock(strongReference);
        Assert(entry->strongRefHeapBlock != nullptr);

        HeapBlock * weakRefHeapBlock = recycler->FindHeapBlock(entry);
        Assert(!weakRefHeapBlock->IsLargeHeapBlock() || ((LargeHeapBlock*)weakRefHeapBlock)->InPageHeapMode());
        entry->weakRefHeapBlock = weakRefHeapBlock;

#ifdef RECYCLER_TRACE_WEAKREF
        Output::Print(_u("Add WeakRef 0x%08x for StrongRef %p to bucket %d\n"), entry, strongReference, targetBucket);
#endif
        AddEntry(entry, &buckets[targetBucket]);
        count++;
#if DBG
#if ENABLE_RECYCLER_TYPE_TRACKING
        entry->typeInfo = nullptr;
#endif
#if defined(TRACK_ALLOC) && defined(PERF_COUNTERS)
        entry->counter = nullptr;
#endif
#endif
        return entry;
    }
};
}
