//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace JsUtil
{
    template <class TKey, class TValue> struct KeyValuePair
    {
    private:
        TKey key;
        TValue value;

    public:
        KeyValuePair()
        {
        }

        KeyValuePair(TKey key, TValue value)
        {
            this->key = key;
            this->value = value;
        }

        KeyValuePair(const KeyValuePair& other)
            : key(other.key), value(other.value)
        {}

        TKey Key() { return key; }
        TKey Key() const { return key; }

        TValue Value() { return value; }
        TValue Value() const { return value; }
    };

}
