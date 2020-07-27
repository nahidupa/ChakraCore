//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// Turn this on to enable magic constants in byte code (useful for debugging)
//#define BYTE_CODE_MAGIC_CONSTANTS

#include "ByteCode/ByteCodeSerializeFlags.h"

namespace Js
{
    // Some things are obscured by xor. This helps catch cases in which, for example, indirect property ids
    // are mistakenly mixed with actual property IDs.
#if DBG & VALIDATE_SERIALIZED_BYTECODE
    #define SERIALIZER_OBSCURE_PROPERTY_ID 0xdef00000
    #define SERIALIZER_OBSCURE_NONBUILTIN_PROPERTY_ID 0xdeb00000
    #define SERIALIZER_OBSCURE_LITERAL_OBJECT_ID 0xded00000
#else
    #define SERIALIZER_OBSCURE_PROPERTY_ID 0x00000000
    #define SERIALIZER_OBSCURE_NONBUILTIN_PROPERTY_ID 0x00000000
    #define SERIALIZER_OBSCURE_LITERAL_OBJECT_ID 0x00000000
#endif

    class ByteCodeBufferReader;

    enum SerializedAuxiliaryKind : byte
    {
        sakVarArrayIntCount = 1,
        sakVarArrayVarCount = 2,
        sakPropertyIdArray = 3,
        sakFuncInfoArray = 4,
        sakIntArray = 5,
        sakFloatArray = 6
    };

// Tightly pack serialized structures
#pragma pack(push, 1)

    // Describes the kind of auxiliary
    struct SerializedAuxiliary
    {
#ifdef BYTE_CODE_MAGIC_CONSTANTS
        int auxMagic; // magicStartOfAux
#endif
        uint offset;
        SerializedAuxiliaryKind kind;
        SerializedAuxiliary(uint offset, SerializedAuxiliaryKind kind);
    };

    // The in-memory layout of the serialized analog of VarArray
    struct SerializedVarArray : SerializedAuxiliary
    {
#ifdef BYTE_CODE_MAGIC_CONSTANTS
        int magic; // magicStartOfAuxVarArray
#endif
        int varCount;
        SerializedVarArray(uint offset, bool isVarCount, int varCount);
    };

    struct SerializedIntArray : SerializedAuxiliary
    {
#ifdef BYTE_CODE_MAGIC_CONSTANTS
        int magic; // magicStartOfAuxIntArray
#endif
        int intCount;
        SerializedIntArray(uint offset, int intCount);
    };

    struct SerializedFloatArray : SerializedAuxiliary
    {
#ifdef BYTE_CODE_MAGIC_CONSTANTS
        int magic; // magicStartOfAuxFltArray
#endif
        int floatCount;
        SerializedFloatArray(uint offset, int floatCount);
    };

    // The in-memory layout of the serialized analog of PropertyIdArray
    struct SerializedPropertyIdArray : SerializedAuxiliary
    {
#ifdef BYTE_CODE_MAGIC_CONSTANTS
        int magic; // magicStartOfAuxPropIdArray
#endif
        int propertyCount;
        byte extraSlots;
        bool hadDuplicates;
        bool has__proto__;
        SerializedPropertyIdArray(uint offset, int propertyCount, byte extraSlots, bool hadDuplicates, bool has__proto__);
    };

    // The in-memory layout of the serialized analog of FuncInfoArray
    struct SerializedFuncInfoArray : SerializedAuxiliary
    {
#ifdef BYTE_CODE_MAGIC_CONSTANTS
        int magic; // magicStartOfAuxFuncInfoArray
#endif
        int count;
        SerializedFuncInfoArray(uint offset, int count);
    };

    typedef uint LocalScopeInfoId;

#pragma pack(pop)

    // Holds information about the deserialized bytecode cache. Contains fast inline functions
    // for the lookup hit case. The slower deserialization of VarArray, etc are in the .cpp.
    class ByteCodeCache
    {
        ByteCodeBufferReader * reader;
        const byte * raw;
        PropertyId * propertyIds;
        int propertyCount;
        int builtInPropertyCount;
        uint scopeInfoCount;
        const byte** scopeInfoRelativeOffsets;

        typedef JsUtil::BaseDictionary<Js::LocalFunctionId, FunctionInfo*, ArenaAllocator> LocalFunctionIdToFunctionInfoMap;
        LocalFunctionIdToFunctionInfoMap* localFunctionIdToFunctionInfoMap;
        typedef JsUtil::BaseDictionary<LocalScopeInfoId, ScopeInfo*, ArenaAllocator> LocalScopeInfoIdToScopeInfoMap;
        LocalScopeInfoIdToScopeInfoMap* localScopeInfoIdToScopeInfoMap;

    private:
        LocalFunctionIdToFunctionInfoMap * EnsureLocalFunctionIdToFunctionInfoMap(ScriptContext * scriptContext);
        LocalScopeInfoIdToScopeInfoMap * EnsureLocalScopeInfoIdToScopeInfoMap(ScriptContext * scriptContext);

    public:
        ByteCodeCache(ScriptContext * scriptContext, int builtInPropertyCount);
        ByteCodeCache(ScriptContext * scriptContext, ByteCodeBufferReader * reader, int builtInPropertyCount);
        void PopulateLookupPropertyId(ScriptContext * scriptContext, int realArrayOffset);
        void SetReader(ScriptContext * scriptContext, ByteCodeBufferReader * reader);
        void Initialize(ScriptContext * scriptContext);

        void RegisterFunctionIdToFunctionInfo(ScriptContext * scriptContext, LocalFunctionId functionId, FunctionInfo* functionInfo);
        FunctionInfo* LookupFunctionInfo(ScriptContext * scriptContext, LocalFunctionId functionId);
        ScopeInfo* LookupScopeInfo(ScriptContext * scriptContext, LocalScopeInfoId scopeInfoId);

        ByteCodeBufferReader* GetReader()
        {
            return reader;
        }

        // Convert a serialized propertyID into a real one.
        inline PropertyId LookupPropertyId(PropertyId obscuredIdInCache) const
        {
            auto unobscured = obscuredIdInCache ^ SERIALIZER_OBSCURE_PROPERTY_ID;
            if (unobscured < builtInPropertyCount || unobscured==/*nil*/0xffffffff)
            {
                return unobscured; // This is a built in property id
            }
            auto realOffset = unobscured - builtInPropertyCount;
            Assert(realOffset<propertyCount);
            Assert(propertyIds[realOffset]!=-1);
            return propertyIds[realOffset];
        }

        // Convert a serialized propertyID into a real one.
        inline PropertyId LookupNonBuiltinPropertyId(PropertyId obscuredIdInCache) const
        {
            auto realOffset = obscuredIdInCache ^ SERIALIZER_OBSCURE_NONBUILTIN_PROPERTY_ID;
            Assert(realOffset<propertyCount);
            Assert(propertyIds[realOffset]!=-1);
            return propertyIds[realOffset];
        }

        // Get the raw byte code buffer.
        inline const byte * GetBuffer() const
        {
            return raw;
        }
    };

    // Methods for serializing and deserializing function bodies.
    struct ByteCodeSerializer
    {
        // Serialize a function body.
        static HRESULT SerializeToBuffer(ScriptContext * scriptContext, ArenaAllocator * alloc, DWORD sourceByteLength, LPCUTF8 utf8Source, FunctionBody * function, SRCINFO const* srcInfo, byte ** buffer, DWORD * bufferBytes, DWORD dwFlags = 0);

        // Deserialize a function body. The content of utf8Source must be the same as was originally passed to SerializeToBuffer
        static HRESULT DeserializeFromBuffer(ScriptContext * scriptContext, uint32 scriptFlags, LPCUTF8 utf8Source, SRCINFO const * srcInfo, byte * buffer, NativeModule *nativeModule, Field(FunctionBody*)* function, uint sourceIndex = Js::Constants::InvalidSourceIndex);
        static HRESULT DeserializeFromBuffer(ScriptContext * scriptContext, uint32 scriptFlags, ISourceHolder* sourceHolder, SRCINFO const * srcInfo, byte * buffer, NativeModule *nativeModule, Field(FunctionBody*)* function, uint sourceIndex = Js::Constants::InvalidSourceIndex);

        static FunctionBody* DeserializeFunction(ScriptContext* scriptContext, DeferDeserializeFunctionInfo* deferredFunction);

        // Deserialize a string from the string table based on the stringId.
        // Note: Returns the count of characters (not bytes) of the string via the stringLength argument.
        static LPCWSTR DeserializeString(const DeferredFunctionStub* deferredStub, uint stringId, uint32& stringLength);

        // This lib doesn't directly depend on the generated interfaces. Ensure the same codes with a C_ASSERT
        static const HRESULT CantGenerate = 0x80020201L;
        static const HRESULT InvalidByteCode = 0x80020202L;

        static void ReadSourceInfo(const DeferDeserializeFunctionInfo* deferredFunction, int& lineNumber, int& columnNumber, bool& m_isEval, bool& m_isDynamicFunction);

    private:
        static HRESULT DeserializeFromBufferInternal(ScriptContext * scriptContext, uint32 scriptFlags, LPCUTF8 utf8Source, ISourceHolder* sourceHolder, SRCINFO const * srcInfo, byte * buffer, NativeModule *nativeModule, Field(FunctionBody*)* function, uint sourceIndex = Js::Constants::InvalidSourceIndex);
    };
}
