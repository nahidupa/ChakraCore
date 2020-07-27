//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace UnifiedRegex
{
    enum RegexFlags : uint8;

    class RegexKey
    {
    private:
        Field(const char16 *) source;
        Field(int) length;
        Field(RegexFlags) flags;

    public:
        RegexKey() : source(nullptr), length(0), flags(static_cast<RegexFlags>(0))
        {
        }

        RegexKey(const char16 *const source, const int length, const RegexFlags flags)
            : source(source), length(length), flags(flags)
        {
            Assert(source);
            Assert(length >= 0);
        }

        RegexKey(const RegexKey& other)
            : source(other.source), length(other.length), flags(other.flags)
        {}

        RegexKey &operator =(const void *const nullValue)
        {
            // Needed to support KeyValueEntry::Clear for dictionaries
            Assert(!nullValue);

            source = nullptr;
            length = 0;
            flags = static_cast<RegexFlags>(0);
            return *this;
        }

        const char16 *Source() const
        {
            return source;
        }

        int Length() const
        {
            return length;
        }

        RegexFlags Flags() const
        {
            return flags;
        }
    };

    struct RegexKeyComparer
    {
        inline static bool Equals(const RegexKey &key1, const RegexKey &key2)
        {
            return
                Js::InternalStringComparer::Equals(
                    Js::InternalString(key1.Source(), key1.Length()),
                    Js::InternalString(key2.Source(), key2.Length())) &&
                key1.Flags() == key2.Flags();
        }

        inline static hash_t GetHashCode(const RegexKey &key)
        {
            return Js::InternalStringComparer::GetHashCode(Js::InternalString(key.Source(), key.Length()));
        }
    };
}

template<>
struct DefaultComparer<UnifiedRegex::RegexKey> : public UnifiedRegex::RegexKeyComparer
{
};
