//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class UriHelper
    {
    public:
        // Flags used to mark properties of various characters for URI handling.
        enum
        {
            URINone      = 0x0,
            URIReserved  = 0x1,
            URIUnescaped = 0x2
        };

        static Var EncodeCoreURI(ScriptContext* scriptContext, Arguments& args, unsigned char flags);
        static Var Decode(JavascriptString* strURI, unsigned char reservedFlags, ScriptContext* scriptContext);
        static Var DecodeCoreURI(ScriptContext* scriptContext, Arguments& args, unsigned char reservedFlags);

        static unsigned char s_uriProps[128];

#if DBG
        // Validate the array of character properties for URI handling. Each entry is a
        // bitwise OR of the flags defined above.
        static void ValidateURIProps(void)
        {
            static bool fChecked = false;
            if (fChecked)
                return;

            for( int c = 0; c < 128; c++ )
            {
                if (isalnum(c))
                    Assert(s_uriProps[c] & URIUnescaped);
                switch(c)
                {
                case '-':
                case '_':
                case '.':
                case '!':
                case '~':
                case '*':
                case '\'':
                case '(':
                case ')':
                    Assert(s_uriProps[c] & URIUnescaped);
                    break;
                case ';':
                case '/':
                case '?':
                case ':':
                case '@':
                case '&':
                case '=':
                case '+':
                case '$':
                case ',':
                case '#':
                    Assert(s_uriProps[c] & URIReserved);
                    break;
                default:
                    if (!isalnum(c))
                        Assert(0 == s_uriProps[c]);
                }
            }
            fChecked = true;
        }
#else
        static inline void ValidateURIProps(void) {}
#endif
        static inline BOOL InURISet( char16 c, unsigned char flags )
        {
            //static unsigned char *uriProps = GetURIProps();
            ValidateURIProps();
            return c <= 0x7f && (s_uriProps[c] & flags);
        }

        static const uint32 MaxUTF8Len = 4;
        static uint32 ToUTF8( uint32 uVal, BYTE bUTF8[MaxUTF8Len]);
        static uint32 FromUTF8( BYTE bUTF8[MaxUTF8Len], uint32 uLen );
        static Var Encode(JavascriptString* strURI, unsigned char unescapedFlags, ScriptContext* scriptContext );

    private:
        static bool DecodeByteFromHex(const char16 digit1, const char16 digit2, unsigned char &value);
    };
}
