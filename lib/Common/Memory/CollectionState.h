//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Memory
{

enum CollectionState
{
    Collection_Mark                 = 0x00000001,
    Collection_Sweep                = 0x00000002,
    Collection_Exit                 = 0x00000004,
    Collection_PreCollection        = 0x00000008,

    // Mark related states
    Collection_ResetMarks           = 0x00000010,
    Collection_FindRoots            = 0x00000020,
    Collection_Rescan               = 0x00000040,
    Collection_FinishMark           = 0x00000080,

    // Sweep related states
#if ENABLE_CONCURRENT_GC
    Collection_ConcurrentSweepSetup = 0x00000100,
#endif
    Collection_TransferSwept        = 0x00000200,

    // State attributes
#if ENABLE_PARTIAL_GC
    Collection_Partial              = 0x00001000,
#endif
#if ENABLE_CONCURRENT_GC
    Collection_Concurrent           = 0x00002000,
    Collection_ExecutingConcurrent  = 0x00004000,
    Collection_FinishConcurrent     = 0x00008000,

    Collection_ConcurrentMark       = Collection_Concurrent | Collection_Mark,
    Collection_ConcurrentSweep      = Collection_Concurrent | Collection_Sweep,
#endif
    Collection_Parallel             = 0x00010000,
    
    Collection_PostCollectionCallback      = 0x00020000,
    Collection_PostSweepRedeferralCallback = 0x00040000,
    Collection_WrapperCallback             = 0x00080000,

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    // Please look at the documentation for PrepareForAllocationsDuringConcurrentSweep method for details on how the concurrent
    // sweep progresses when allocations are allowed during sweep.
    /* In Pass1 of the concurrent sweep we determine if a block is full or empty or needs to be swept. Leaf blocks will get
    swept immediately where as non-leaf blocks may get put onto the pendingSweepList. */
    Collection_ConcurrentSweepPass1 = 0x00100000,
    Collection_ConcurrentSweepPass1Wait = 0x00200000,
    /* In Pass2, all blocks will go through SweepPartialReusePages to determine if the page can be reused. Also, any blocks
    that were put in the pendingSweepList will be swept during this stage. */
    Collection_ConcurrentSweepPass2 = 0x00400000,
    Collection_ConcurrentSweepPass2Wait = 0x00800000,
#endif

    Collection_WeakRefMark          = 0x01000000,

    // Actual states
    CollectionStateNotCollecting          = 0,                                                                // not collecting
    CollectionStateResetMarks             = Collection_Mark | Collection_ResetMarks,                          // reset marks
    CollectionStateFindRoots              = Collection_Mark | Collection_FindRoots,                           // finding roots
    CollectionStateMark                   = Collection_Mark,                                                  // marking (in thread)
    CollectionStateSweep                  = Collection_Sweep,                                                 // sweeping (in thread)
    CollectionStateTransferSwept          = Collection_Sweep | Collection_TransferSwept,                      // transfer swept objects (after concurrent sweep)
    CollectionStateExit                   = Collection_Exit,                                                  // exiting concurrent thread


    // Generally, Rescan is available only when concurrent is enabled
    // But we need Rescan for mark on OOM too
    CollectionStateRescanFindRoots        = Collection_Mark | Collection_Rescan | Collection_FindRoots,       // rescan (after concurrent mark)
    CollectionStateRescanMark             = Collection_Mark | Collection_Rescan,                              // rescan (after concurrent mark)
#if ENABLE_CONCURRENT_GC
    CollectionStateConcurrentResetMarks   = Collection_ConcurrentMark | Collection_ResetMarks | Collection_ExecutingConcurrent,    // concurrent reset mark
    CollectionStateConcurrentFindRoots    = Collection_ConcurrentMark | Collection_FindRoots | Collection_ExecutingConcurrent,     // concurrent findroot
    CollectionStateConcurrentMark         = Collection_ConcurrentMark | Collection_ExecutingConcurrent,                            // concurrent marking
    CollectionStateRescanWait             = Collection_ConcurrentMark | Collection_FinishConcurrent,                               // rescan (after concurrent mark)
    CollectionStateConcurrentFinishMark   = Collection_ConcurrentMark | Collection_ExecutingConcurrent | Collection_FinishConcurrent,

    CollectionStateSetupConcurrentSweep   = Collection_Sweep | Collection_ConcurrentSweepSetup,               // setting up concurrent sweep
    CollectionStateConcurrentSweep        = Collection_ConcurrentSweep | Collection_ExecutingConcurrent,      // concurrent sweep
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    CollectionStateConcurrentSweepPass1 = Collection_ConcurrentSweep | Collection_ConcurrentSweepPass1 | Collection_ExecutingConcurrent,          // concurrent sweep Pass 1
    CollectionStateConcurrentSweepPass1Wait = Collection_ConcurrentSweep | Collection_ConcurrentSweepPass1Wait | Collection_FinishConcurrent,      // concurrent sweep wait state after Pass 1 has finished
    CollectionStateConcurrentSweepPass2 = Collection_ConcurrentSweep | Collection_ConcurrentSweepPass2 | Collection_ExecutingConcurrent,          // concurrent sweep Pass 2
    CollectionStateConcurrentSweepPass2Wait = Collection_ConcurrentSweep | Collection_ConcurrentSweepPass2Wait | Collection_FinishConcurrent,     // concurrent sweep wait state after Pass 2 has finished
#endif
    CollectionStateTransferSweptWait      = Collection_ConcurrentSweep | Collection_FinishConcurrent,         // transfer swept objects (after concurrent sweep)
#endif
    CollectionStateParallelMark           = Collection_Mark | Collection_Parallel,
#if ENABLE_CONCURRENT_GC
    CollectionStateBackgroundParallelMark = Collection_ConcurrentMark | Collection_ExecutingConcurrent | Collection_Parallel,
    CollectionStateConcurrentWrapperCallback = Collection_Concurrent | Collection_ExecutingConcurrent | Collection_WrapperCallback,
#endif
    CollectionStatePostSweepRedeferralCallback = Collection_PostSweepRedeferralCallback,
    CollectionStatePostCollectionCallback = Collection_PostCollectionCallback,

    CollectionStateConcurrentMarkWeakRef = Collection_ConcurrentMark | Collection_ExecutingConcurrent | Collection_WeakRefMark,
};

}
