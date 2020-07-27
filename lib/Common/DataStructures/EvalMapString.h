//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    template <bool fastHash>
    struct EvalMapStringInternal
    {
        FinalizableObject* owningVar; // This is the Var that originally owns the character buffer corresponding to this EvalMap key.
        JsUtil::CharacterBuffer<char16> str;
        hash_t hash;
        ModuleID moduleID;
        BOOL strict;
        BOOL isLibraryCode;

        EvalMapStringInternal() : owningVar(nullptr), str(), moduleID(0), strict(FALSE), isLibraryCode(FALSE), hash(0) {};
        EvalMapStringInternal(FinalizableObject* obj, __in_ecount(charLength) char16 const* content, int charLength, ModuleID moduleID, BOOL strict, BOOL isLibraryCode)
            : owningVar(obj), str(content, charLength), moduleID(moduleID), strict(strict), isLibraryCode(isLibraryCode)
        {
            // NOTE: this hash is not equivalent to the character buffer hash
            // Don't use a CharacteBuffer to do a map lookup on the EvalMapString.
            if (fastHash)
            {
                hash = TAGHASH(str.FastHash());
            }
            else
            {
                hash = TAGHASH((hash_t)str);
            }
        };

        EvalMapStringInternal& operator=(void * str)
        {
            Assert(str == null);
            memset(this, 0, sizeof(*this));
            return (*this);
        }

        inline ModuleID GetModuleID() const
        {
            return moduleID;
        }

        inline BOOL IsStrict() const
        {
            return strict;
        }

        // Equality and hash function
        bool operator==(EvalMapStringInternal const& other) const
        {
             return this->str == other.str &&
                this->GetModuleID() == other.GetModuleID() &&
                this->IsStrict() == other.IsStrict() &&
                this->isLibraryCode == other.isLibraryCode;
        }

        operator hash_t() const
        {
            return UNTAGHASH(hash);
        }
    };
    typedef EvalMapStringInternal<true> FastEvalMapString;
    typedef EvalMapStringInternal<false> EvalMapString;

    void ConvertKey(const FastEvalMapString& src, EvalMapString& dest);
};
