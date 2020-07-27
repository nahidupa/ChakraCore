//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//
// AllocationPolicyManager allows a caller/host to disallow new page allocations
// and to track current page usage.
//

// NOTE: For now, we are only tracking reserved page count.
// Consider whether we should also (or maybe only) track committed page count.

class AllocationPolicyManager
{
public:
    enum MemoryAllocateEvent
    {
        MemoryAllocate = 0,
        MemoryFree = 1,
        MemoryFailure = 2,
        MemoryMax = 2,
    };
    typedef bool (__stdcall * PageAllocatorMemoryAllocationCallback)(__in LPVOID context,
        __in AllocationPolicyManager::MemoryAllocateEvent allocationEvent,
        __in size_t allocationSize);


private:
    size_t memoryLimit;
    size_t currentMemory;
    bool supportConcurrency;
    CriticalSection cs;
    void * context;
    PageAllocatorMemoryAllocationCallback memoryAllocationCallback;

public:
    AllocationPolicyManager(bool needConcurrencySupport) :
        memoryLimit((size_t)-1),
        currentMemory(0),
        supportConcurrency(needConcurrencySupport),
        context(NULL),
        memoryAllocationCallback(NULL)
    {
        Js::Number limitMB = Js::Configuration::Global.flags.AllocPolicyLimit;
        if (limitMB > 0)
        {
            memoryLimit = (size_t)limitMB * 1024 * 1024;
        }
    }

    ~AllocationPolicyManager()
    {
        Assert(currentMemory == 0);
    }

    size_t GetUsage()
    {
        return currentMemory;
    }

    size_t GetLimit()
    {
        return memoryLimit;
    }

    void SetLimit(size_t newLimit)
    {
        memoryLimit = newLimit;
    }

    bool RequestAlloc(DECLSPEC_GUARD_OVERFLOW size_t byteCount, bool externalAlloc = false)
    {
        if (supportConcurrency)
        {
            AutoCriticalSection auto_cs(&cs);
            return RequestAllocImpl(byteCount, externalAlloc);
        }
        else
        {
            return RequestAllocImpl(byteCount, externalAlloc);
        }
    }


    void ReportFailure(size_t byteCount)
    {
        if (supportConcurrency)
        {
            AutoCriticalSection auto_cs(&cs);
            ReportFreeImpl(MemoryAllocateEvent::MemoryFailure, byteCount);
        }
        else
        {
            ReportFreeImpl(MemoryAllocateEvent::MemoryFailure, byteCount);
        }

    }

    void ReportFree(size_t byteCount)
    {
        if (supportConcurrency)
        {
            AutoCriticalSection auto_cs(&cs);
            ReportFreeImpl(MemoryAllocateEvent::MemoryFree, byteCount);
        }
        else
        {
            ReportFreeImpl(MemoryAllocateEvent::MemoryFree, byteCount);
        }
    }

    void SetMemoryAllocationCallback(LPVOID newContext, PageAllocatorMemoryAllocationCallback callback)
    {
        this->memoryAllocationCallback = callback;

        if (callback == NULL)
        {
            // doesn't make sense to have non-null context when the callback is NULL.
            this->context = NULL;
        }
        else
        {
            this->context = newContext;
        }
    }

private:
    inline bool RequestAllocImpl(size_t byteCount, bool externalAlloc = false)
    {
        size_t newCurrentMemory = currentMemory + byteCount;

        if (newCurrentMemory < currentMemory ||
            newCurrentMemory > memoryLimit ||
            (memoryAllocationCallback != NULL && !memoryAllocationCallback(context, MemoryAllocateEvent::MemoryAllocate, byteCount)))
        {
            // oopjit number allocator allocated pages, we can't stop it from allocating so just increase the usage number
            if (externalAlloc)
            {
                currentMemory = newCurrentMemory;
                return true;
            }

            if (memoryAllocationCallback != NULL)
            {
                memoryAllocationCallback(context, MemoryAllocateEvent::MemoryFailure, byteCount);
            }

            return false;
        }
        else
        {
            currentMemory = newCurrentMemory;
            return true;
        }
    }

    inline void ReportFreeImpl(MemoryAllocateEvent allocationEvent, size_t byteCount)
    {
        Assert(currentMemory >= byteCount);
        byteCount = min(byteCount, currentMemory);

        currentMemory = currentMemory - byteCount;

        if (memoryAllocationCallback != NULL)
        {
            // The callback should be minimal, with no possibility of calling back to us.
            // Note that this can be called both in script or out of script.
            memoryAllocationCallback(context, allocationEvent, byteCount);
        }
    }
};
