//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    // - Data generated for jitting and runtime purposes. Data grows as different versions of the function body are jitted.
    // - Recycler-allocated, lifetime is from when a code gen work item is added to the jit queue, to when the function body
    //   becomes collectible
    class FunctionCodeGenRuntimeData sealed
    {
    private:
        Field(FunctionBody *const) functionBody;

        // These cloned inline caches are guaranteed to be kept alive for the life of the function body. They may be shared
        // by different versions of the same function body that have been or will be jitted. Cached data is not guaranteed to be
        // stable while jitting.
        Field(InlineCachePointerArray<InlineCache>) clonedInlineCaches;

        // There will be a non-null entry for each profiled call site where a function is to be inlined or has previously been
        // inlined
        Field(Field(FunctionCodeGenRuntimeData *)*) inlinees;

        // There will be a non-null entry for each  call site where a getter setter is to be inlined or has previously been
        // inlined
        Field(Field(FunctionCodeGenRuntimeData *)*) ldFldInlinees;

        // There will be a non-null entry for each call site where a function is passed in as an argument
        Field(Field(FunctionCodeGenRuntimeData *)*) callbackInlinees;

        Field(Field(FunctionCodeGenRuntimeData *)*) callApplyTargetInlinees;

        Field(FunctionCodeGenRuntimeData *) next;

    public:
        FunctionCodeGenRuntimeData(FunctionBody *const functionBody);
        void SetupRuntimeDataChain(FunctionCodeGenRuntimeData *nextRuntimeData) { this->next = nextRuntimeData; }

    public:
        FunctionBody *GetFunctionBody() const;
        FunctionCodeGenRuntimeData *GetNext() const { return next; };
        Field(FunctionCodeGenRuntimeData*)* GetInlinees() const { return inlinees; }
        Field(FunctionCodeGenRuntimeData*)* GetLdFldInlinees() const { return ldFldInlinees; }
        Field(FunctionCodeGenRuntimeData*)* GetCallbackInlinees() const { return callbackInlinees; }
        Field(FunctionCodeGenRuntimeData*)* GetCallApplyTargetInlinees() const { return callApplyTargetInlinees; }
        const FunctionCodeGenRuntimeData *GetForTarget(FunctionBody *targetFuncBody) const;
        const InlineCachePointerArray<InlineCache> *ClonedInlineCaches() const;
        InlineCachePointerArray<InlineCache> *ClonedInlineCaches();

        const FunctionCodeGenRuntimeData *GetInlinee(const ProfileId profiledCallSiteId) const;
        const FunctionCodeGenRuntimeData *GetInlineeForTargetInlinee(const ProfileId profiledCallSiteId, FunctionBody *inlineeFuncBody) const;

        FunctionCodeGenRuntimeData * EnsureInlineeCommon(
            Recycler *const recycler,
            const ProfileId profiledCallSiteId,
            FunctionBody *const inlinee,
            Field(Field(FunctionCodeGenRuntimeData *)*) & codeGenRuntimeData);

        FunctionCodeGenRuntimeData *EnsureInlinee(
            Recycler *const recycler,
            const ProfileId profiledCallSiteId,
            FunctionBody *const inlinee);

        void SetupRecursiveInlineeChain(
            Recycler *const recycler,
            const ProfileId profiledCallSiteId,
            FunctionBody *const inlinee);

        const FunctionCodeGenRuntimeData *GetLdFldInlinee(const InlineCacheIndex inlineCacheIndex) const;
        FunctionCodeGenRuntimeData *EnsureLdFldInlinee(
            Recycler *const recycler,
            const InlineCacheIndex inlineCacheIndex,
            FunctionBody *const inlinee);

        const FunctionCodeGenRuntimeData * GetCallbackInlinee(const ProfileId profiledCallSiteId) const;
        FunctionCodeGenRuntimeData * EnsureCallbackInlinee(
            Recycler *const recycler,
            const ProfileId profiledCallSiteId,
            FunctionBody *const inlinee);

        const FunctionCodeGenRuntimeData * GetCallApplyTargetInlinee(const ProfileId callApplyCallSiteId) const;
        FunctionCodeGenRuntimeData * EnsureCallApplyTargetInlinee(
            Recycler *const recycler,
            const ProfileId callApplyCallSiteId,
            FunctionBody *const inlinee);

        // This function walks all the chained jittimedata and returns the one which match the functionInfo.
        // This can return null, if the functionInfo doesn't match.
        const FunctionCodeGenRuntimeData *GetRuntimeDataFromFunctionInfo(FunctionInfo *polyFunctionInfo) const;

        template<class Fn>
        void MapInlineCaches(Fn fn) const
        {
            this->clonedInlineCaches.Map(fn, this->functionBody->GetInlineCacheCount());

            for (ProfileId iInlinee = 0; iInlinee < this->functionBody->GetProfiledCallSiteCount(); iInlinee++)
            {
                const FunctionCodeGenRuntimeData* runtimeData = this->GetInlinee(iInlinee);
                while (runtimeData)
                {
                    if (functionBody == runtimeData->GetFunctionBody())
                    {
                        break;
                    }
                    // Map for chained ones as well.
                    runtimeData->MapInlineCaches(fn);
                    runtimeData = runtimeData->next;
                }
            }
        }

        PREVENT_COPY(FunctionCodeGenRuntimeData)
    };
}
