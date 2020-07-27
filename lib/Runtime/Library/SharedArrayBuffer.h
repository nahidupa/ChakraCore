//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//  Implements SharedArrayBuffer according to http://tc39.github.io/ecmascript_sharedmem/shmem.html
//----------------------------------------------------------------------------

#pragma once
namespace Js
{

    class WaiterList;
    typedef JsUtil::List<DWORD_PTR, HeapAllocator> SharableAgents;
    typedef JsUtil::BaseDictionary<uint, WaiterList *, HeapAllocator> IndexToWaitersMap;

    class SharedContents
    {
    public:
        BYTE  *buffer;             // Points to a heap allocated RGBA buffer, can be null
        IndexToWaitersMap *indexToWaiterList;  // Map of agents waiting on a particular index.
        uint32 bufferLength;       // Number of bytes allocated
        uint32 maxBufferLength = 0; // Maximum number of bytes to allocate (only used by WebAssemblySharedArrayBuffer)
        bool isWebAssembly = false;
    private:
        // Addref/release counter for current buffer, this is needed as the current buffer will be shared among different workers
        long refCount;

    public:
        long AddRef();
        long Release();
        bool IsWebAssembly() const { return isWebAssembly; }
        void SetIsWebAssembly() { isWebAssembly = true; }
        static int GetBufferOffset() { return offsetof(SharedContents, buffer); }
        static int GetBufferLengthOffset() { return offsetof(SharedContents, bufferLength); }

#if DBG
        // This is mainly used for validation purpose as the wait/wake APIs should be used on the agents (Workers) among which this buffer is shared.
        SharableAgents *allowedAgents;
        CriticalSection csAgent;
        void AddAgent(DWORD_PTR agent);
        bool IsValidAgent(DWORD_PTR agent);
#endif

        void Cleanup();

        SharedContents(BYTE* b, uint32 l, uint32 m)
            : buffer(b), bufferLength(l), maxBufferLength(m), refCount(1), indexToWaiterList(nullptr)
#if DBG
            , allowedAgents(nullptr)
#endif
        {
        }
    };

    class SharedArrayBuffer : public ArrayBufferBase
    {
    public:
        DEFINE_VTABLE_CTOR_ABSTRACT(SharedArrayBuffer, ArrayBufferBase);
        SharedArrayBuffer(DynamicType * type);
        SharedArrayBuffer(SharedContents *contents, DynamicType * type);

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo Slice;
            static FunctionInfo GetterByteLength;
            static FunctionInfo GetterSymbolSpecies;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySlice(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterByteLength(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...);

        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        virtual uint32 GetByteLength() const override;
        virtual BYTE* GetBuffer() const override;

        static int GetByteLengthOffset() { Assert(false); return 0; }
        static int GetBufferOffset() { Assert(false); return 0; }
        static int GetSharedContentsOffset() { return offsetof(SharedArrayBuffer, sharedContents); }
        virtual bool IsArrayBuffer() override { return false; }
        virtual bool IsSharedArrayBuffer() override { return true; }
        virtual ArrayBuffer * GetAsArrayBuffer() { return nullptr; }
        virtual SharedArrayBuffer * GetAsSharedArrayBuffer() override;

        WaiterList *GetWaiterList(uint index);
        SharedContents *GetSharedContents() { return sharedContents; }

#if defined(TARGET_64)
        //maximum 2G -1  for amd64
        static const uint32 MaxSharedArrayBufferLength = 0x7FFFFFFF;
#else
        // maximum 1G to avoid arithmetic overflow.
        static const uint32 MaxSharedArrayBufferLength = 1 << 30;
#endif
        virtual bool IsValidVirtualBufferLength(uint length) const;

    protected:
        // maxLength is necessary only for WebAssemblySharedArrayBuffer to know how much it can grow
        // Must call after constructor of child class is completed. Required to be able to make correct virtual calls
        void Init(uint32 length, uint32 maxLength);
        virtual BYTE* AllocBuffer(uint32 length, uint32 maxLength);
        virtual void FreeBuffer(BYTE* buffer, uint32 length, uint32 maxLength);

        FieldNoBarrier(SharedContents *) sharedContents;

        static CriticalSection csSharedArrayBuffer;
    };

    template <> inline bool VarIsImpl<SharedArrayBuffer>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_SharedArrayBuffer;
    }

    class JavascriptSharedArrayBuffer : public SharedArrayBuffer
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptSharedArrayBuffer, SharedArrayBuffer);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptSharedArrayBuffer);

    public:
        static JavascriptSharedArrayBuffer* Create(uint32 length, DynamicType * type);
        static JavascriptSharedArrayBuffer* Create(SharedContents *sharedContents, DynamicType * type);
        virtual void Dispose(bool isShutdown) override;
        virtual void Finalize(bool isShutdown) override;

    protected:
        JavascriptSharedArrayBuffer(DynamicType * type);
        JavascriptSharedArrayBuffer(SharedContents *sharedContents, DynamicType * type);

    };

#ifdef ENABLE_WASM_THREADS
    class WebAssemblySharedArrayBuffer : public JavascriptSharedArrayBuffer
    {
    protected:
        DEFINE_VTABLE_CTOR(WebAssemblySharedArrayBuffer, JavascriptSharedArrayBuffer);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(WebAssemblySharedArrayBuffer);

    public:
        static WebAssemblySharedArrayBuffer* Create(uint32 length, uint32 maxLength, DynamicType * type);
        static WebAssemblySharedArrayBuffer* Create(SharedContents *sharedContents, DynamicType * type);

        virtual bool IsValidVirtualBufferLength(uint length) const override;
        virtual bool IsWebAssemblyArrayBuffer() override { return true; }
        _Must_inspect_result_ bool GrowMemory(uint32 newBufferLength);

    protected:
        virtual BYTE* AllocBuffer(uint32 length, uint32 maxLength) override;
        virtual void FreeBuffer(BYTE* buffer, uint32 length, uint32 maxLength) override;

    private:
        WebAssemblySharedArrayBuffer(DynamicType * type);
        WebAssemblySharedArrayBuffer(SharedContents *sharedContents, DynamicType * type);
        void ValidateBuffer();
    };

    template <> inline bool VarIsImpl<WebAssemblySharedArrayBuffer>(RecyclableObject* obj)
    {
        return VarIs<SharedArrayBuffer>(obj) && UnsafeVarTo<SharedArrayBuffer>(obj)->IsWebAssemblyArrayBuffer();
    }
#endif

    // An agent can be viewed as a worker
    struct AgentOfBuffer
    {
    public:
        AgentOfBuffer() :identity(NULL), event(NULL) {}
        AgentOfBuffer(DWORD_PTR agent, HANDLE e) :identity(agent), event(e) {}
        static bool AgentCanSuspend(ScriptContext *scriptContext);

        DWORD_PTR identity;
        HANDLE event;
    };

    typedef JsUtil::List<AgentOfBuffer, HeapAllocator> Waiters;
    class WaiterList
    {
    public:

        WaiterList();
        void Cleanup();

        bool _Requires_lock_held_(csForAccess.cs) AddAndSuspendWaiter(DWORD_PTR waiter, uint32 timeout);
        void RemoveWaiter(DWORD_PTR waiter);
        uint32 RemoveAndWakeWaiters(int32 count);

        CriticalSection * GetCriticalSectionForAccess() { return &csForAccess; }

    private:
        void InitWaiterList();
        bool Contains(DWORD_PTR agent);

        Waiters * m_waiters;

        // Below CS is used for synchronizing access in wait/wake API
        CriticalSection csForAccess;
    };
}

