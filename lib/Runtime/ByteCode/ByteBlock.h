//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class ByteBlock
    {
        DECLARE_OBJECT(ByteBlock)

    private:
        Field(uint) m_contentSize = 0;     // Length of block, in bytes

        __declspec(align(4))    // Align the buffer to sizeof(uint32) to improve GetHashCode() perf.
            Field(byte*) m_content = nullptr;    // The block's content

        static ByteBlock* New(Recycler* alloc, const byte * initialContent, int initialContentSize, ScriptContext * requestContext);

    public:
        ByteBlock(uint size, byte * content)
            : m_contentSize(size), m_content(content)
        { }
        ByteBlock(uint size, Recycler *alloc) : m_contentSize(size), m_content(nullptr)
        {
            // The New function below will copy over a buffer into this so
            // we don't need to zero it out
            m_content = RecyclerNewArrayLeaf(alloc, byte, size);
        }

        ByteBlock(uint size, ArenaAllocator* alloc) : m_contentSize(size), m_content(nullptr)
        {
            m_content = AnewArray(alloc, byte, size);
        }

        static DWORD GetBufferOffset() { return offsetof(ByteBlock, m_content); }

        static ByteBlock* New(Recycler* alloc, const byte * initialContent, int initialContentSize);

        // This is needed just for the debugger since it allocates
        // the byte block on a separate thread, which the recycler doesn't like.
        // To remove when the recycler supports multi-threaded allocation.
        static ByteBlock* NewFromArena(ArenaAllocator* alloc, const byte * initialContent, int initialContentSize);

        uint GetLength() const;
        byte* GetBuffer();
        const byte* GetBuffer() const;
        byte operator[](uint itemIndex) const;
        byte& operator[] (uint itemIndex);
    };
} // namespace Js
