//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

typedef uint hash_t;
#define TAGHASH(hash) ((static_cast<hash_t>(hash) << 1) | 1)
#define UNTAGHASH(hash) (static_cast<hash_t>(hash) >> 1)
// This comparer can create good hash codes and comparisons for all value,
// pointer and string types using template specialization.
template <typename T>
struct DefaultComparer
{
    inline static bool Equals(const T &x, const T &y)
    {
        return x == y;
    }

    inline static hash_t GetHashCode(const T &i)
    {
        return (hash_t)i;
    }
};

template <>
struct DefaultComparer<double>
{
    inline static bool Equals(double x, double y)
    {
        return x == y;
    }

    inline static hash_t GetHashCode(double d)
    {
        __int64 i64 = *(__int64*)&d;
        return (hash_t)((i64>>32) ^ (uint)i64);
    }
};

template <typename T>
struct DefaultComparer<T *>
{
    inline static bool Equals(T * x, T * y)
    {
        return x == y;
    }

    inline static hash_t GetHashCode(T * i)
    {
        // Shifting helps us eliminate any sameness due to our alignment strategy.
        // TODO: This works for Arena memory only. Recycler memory is 16 byte aligned.
        // Find a good universal hash for pointers.
        return (hash_t)(((size_t)i) >> ArenaAllocator::ObjectAlignmentBitShift);
    }
};

template <>
struct DefaultComparer<size_t>
{
    inline static bool Equals(size_t x, size_t y)
    {
        return x == y;
    }

    inline static hash_t GetHashCode(size_t i)
    {
#ifdef TARGET_64
        // For 64 bits we want all 64 bits of the pointer to be represented in the hash code.
        uint32 hi = ((UINT_PTR) i >> 32);
        uint32 lo = (uint32)i;
        hash_t hash = hi ^ lo;
#else
        hash_t hash = i;
#endif
        return hash;
    }

    static int Compare(size_t i1, size_t i2)
    {
        if (i1 < i2)
        {
            return -1;
        }
        else if (i1 > i2)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
};

// This specialization does a better job of creating hash codes
// for recycler pointers.
template <typename T>
struct RecyclerPointerComparer
{
    inline static bool Equals(T x, T y)
    {
        return x == y;
    }

    inline static hash_t GetHashCode(T i)
    {
        // Shifting helps us eliminate any sameness due to our alignment strategy.
        // TODO: This works for Recycler memory only. Arena memory is 8 byte aligned.
        // Find a good universal hash for pointers.
        return (hash_t)(((size_t)i) >> HeapConstants::ObjectAllocationShift);
    }
};

 // TODO: FNV is a proven hash especially for short strings, which is common case here.
 //       Still. it may be worth to consider a more recent block-based hash. 
 //       They tend to be faster, but it need to be examined against typical workloads.
 //
 //  FNV-1a hash -> https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 #define CC_HASH_OFFSET_VALUE 2166136261
 #define CC_HASH_LOGIC(hash, byte) \
    hash ^= byte;                  \
    hash *= 16777619

template <>
struct DefaultComparer<GUID>
{
    inline static bool Equals(GUID const& x, GUID const& y)
    {
        return x == y;
    }

     inline static hash_t GetHashCode(GUID const& guid)
     {
        char* p = (char*)&guid;
        hash_t hash = CC_HASH_OFFSET_VALUE;
        for (int i = 0; i < sizeof(GUID); i++)
        {
            CC_HASH_LOGIC(hash, (uint32)(p[i]));
        }
        return hash;
     }
};

template<typename T>
struct StringComparer
{
    inline static bool Equals(T str1, T str2)
    {
        return ::wcscmp(str1, str2) == 0;
    }

    inline static hash_t GetHashCode(T str)
    {
        hash_t hash = CC_HASH_OFFSET_VALUE;
        while (*str)
        {
            CC_HASH_LOGIC(hash, *str);
            str++;
        }
        return hash;
    }

    inline static int Compare(T str1, T str2)
    {
        return ::wcscmp(str1, str2);
    }
};

template<>
struct DefaultComparer<WCHAR*> : public StringComparer<WCHAR*> {};

template<>
struct DefaultComparer<const WCHAR*> : public StringComparer<const WCHAR*> {};

template <typename T, typename TComparer>
struct SpecializedComparer
{
    template <typename T> class TComparerType : public TComparer {};
};

namespace regex
{
    template <class T> struct Comparer
    {
    public:
        virtual bool Equals(T item1, T item2) = 0;
        virtual int GetHashCode(T item) = 0;
        virtual int Compare(T item1, T item2) = 0;
    };
}
