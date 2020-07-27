//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptWeakSet : public DynamicObject
    {
    private:
#if ENABLE_WEAK_REFERENCE_REGIONS
        typedef JsUtil::WeakReferenceRegionKeyDictionary<RecyclableObject*, bool, RecyclerPointerComparer> KeySet;
        typedef const RecyclerWeakReferenceRegionItem<RecyclableObject*>& WeakType;
#else
        typedef JsUtil::WeaklyReferencedKeyDictionary<RecyclableObject, bool, RecyclerPointerComparer<const RecyclableObject*>> KeySet;
        typedef const RecyclerWeakReference<RecyclableObject>* WeakType;
#endif

        Field(KeySet) keySet;

        DEFINE_VTABLE_CTOR_MEMBER_INIT(JavascriptWeakSet, DynamicObject, keySet);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptWeakSet);

    public:
        JavascriptWeakSet(DynamicType* type);

        void Add(RecyclableObject* key);
        bool Delete(RecyclableObject* key);
        bool Has(RecyclableObject* key);

        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo Add;
            static FunctionInfo Delete;
            static FunctionInfo Has;

        };
        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAdd(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryDelete(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryHas(RecyclableObject* function, CallInfo callInfo, ...);

    public:
        // For diagnostics and heap enum provide size and allow enumeration of key value pairs
        int Size() { keySet.Clean(); return keySet.Count(); }
        template <typename Fn>
        void Map(Fn fn)
        {
            return keySet.Map([&](RecyclableObject* key, bool, WeakType)
            {
                fn(key);
            });
        }

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> inline bool VarIsImpl<JavascriptWeakSet>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_WeakSet;
    }
}
