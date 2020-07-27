//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
namespace Memory
{
enum IdleDecommitSignal
{
    IdleDecommitSignal_None,
    IdleDecommitSignal_NeedTimer,
    IdleDecommitSignal_NeedSignal
};

class IdleDecommitPageAllocator : public PageAllocator
{
public:
    IdleDecommitPageAllocator(AllocationPolicyManager * policyManager, PageAllocatorType type,
        Js::ConfigFlagsTable& flagTable,
        uint maxFreePageCount = 0,
        uint maxIdleFreePageCount = DefaultMaxFreePageCount,
        bool zeroPages = false,
#if ENABLE_BACKGROUND_PAGE_FREEING
        BackgroundPageQueue * backgroundPageQueue = nullptr,
#endif
        uint maxAllocPageCount = PageAllocator::DefaultMaxAllocPageCount,
        bool enableWriteBarrier = false);

    void EnterIdleDecommit();
    IdleDecommitSignal LeaveIdleDecommit(bool allowTimer);
    void Prime(uint primePageCount);

#ifdef IDLE_DECOMMIT_ENABLED
    DWORD IdleDecommit();
    void DecommitNow(bool all = true);
#endif

#if DBG
    virtual bool IsIdleDecommitPageAllocator() const override { return true; }
    virtual bool HasMultiThreadAccess() const override;
    void ShutdownIdleDecommit();
#endif

private:
    class AutoResetWaitingToEnterIdleDecommitFlag
    {
    public:
        AutoResetWaitingToEnterIdleDecommitFlag(IdleDecommitPageAllocator * pageAllocator)
        {
            this->pageAllocator = pageAllocator;
            pageAllocator->waitingToEnterIdleDecommit = true;
        }

        ~AutoResetWaitingToEnterIdleDecommitFlag()
        {
            pageAllocator->waitingToEnterIdleDecommit = false;
        }

    private:
        IdleDecommitPageAllocator * pageAllocator;
    };

#ifdef IDLE_DECOMMIT_ENABLED
#if DBG_DUMP
    virtual void DumpStats() const override sealed;
    size_t idleDecommitCount;
#endif
#endif
    uint maxIdleDecommitFreePageCount;
    uint maxNonIdleDecommitFreePageCount;
#ifdef IDLE_DECOMMIT_ENABLED
    bool hasDecommitTimer;
    bool hadDecommitTimer;
    DWORD decommitTime;
    uint idleDecommitTryEnterWaitFactor;
    CriticalSection cs;
    static const uint IdleDecommitTimeout = 1000;
#endif

    friend class PageAllocatorBase<VirtualAllocWrapper>;
#if ENABLE_NATIVE_CODEGEN
    friend class PageAllocatorBase<PreReservedVirtualAllocWrapper>;
#endif

#if IDLE_DECOMMIT_ENABLED && DBG
public:
    virtual void UpdateThreadContextHandle(ThreadContextId threadContextHandle) override sealed
    {
        PageAllocator::UpdateThreadContextHandle(threadContextHandle);
    }

    virtual void SetDisableThreadAccessCheck() override
    {
        PageAllocator::SetDisableThreadAccessCheck();
    }

    virtual void SetEnableThreadAccessCheck() override
    {
        // Can't re-enable thread access check for idle decommit page allocator
        Assert(false);
    }
#endif
};
}
