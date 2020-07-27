//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef _WIN32
#include <windows.h>
#include <wtypes.h>
#else
// TODO: Abstract out into it's own file
#include "pal.h"
#include "inc/rt/palrt.h"
#include <stdint.h>
#endif

// Utf8Codex.h needs to be self contained, so these type defs are duplicated from CommonTypeDefs.h
#ifdef _WIN32
typedef WCHAR char16;
#define _u(s) L##s
#else
#define _u(s) u##s
#endif

typedef char16 wchar;

#ifndef Unused
#define Unused(var) var
#endif

extern void CodexAssert(bool condition);
extern void CodexAssertOrFailFast(bool condition);

#ifdef _MSC_VER
//=============================
// Disabled Warnings
//=============================

#pragma warning(push)

#pragma warning(disable: 4127)  // constant expression for template parameter
#endif

#ifndef _WIN32
// Templates are defined here in order to avoid a dependency on C++
// <type_traits> header file,
// or on compiler-specific contructs.
extern "C++" {

    template <size_t S>
    struct _ENUM_FLAG_INTEGER_FOR_SIZE;

    template <>
    struct _ENUM_FLAG_INTEGER_FOR_SIZE<1>
    {
        typedef int8_t type;
    };

    template <>
    struct _ENUM_FLAG_INTEGER_FOR_SIZE<2>
    {
        typedef int16_t type;
    };

    template <>
    struct _ENUM_FLAG_INTEGER_FOR_SIZE<4>
    {
        typedef int32_t type;
    };

    // used as an approximation of std::underlying_type<T>
    template <class T>
        struct _ENUM_FLAG_SIZED_INTEGER
    {
        typedef typename _ENUM_FLAG_INTEGER_FOR_SIZE<sizeof(T)>::type
    type;
    };

}

#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) \
extern "C++" { \
inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a) | ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
inline ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type &)a) |= ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a) & ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
inline ENUMTYPE &operator &= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type &)a) &= ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
inline ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a)); } \
inline ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a) ^ ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
inline ENUMTYPE &operator ^= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type &)a) ^= ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
}

#endif

typedef unsigned __int32 uint32;
// charcount_t represents a count of characters in a String
// It is unsigned and the maximum value is (INT_MAX-1)
typedef uint32 charcount_t;

typedef BYTE utf8char_t;
typedef const utf8char_t CUTF8;
typedef utf8char_t* LPUTF8;
typedef const utf8char_t *LPCUTF8;

// Unicode 4.0, unknown char should be converted to replace mark, U+FFFD.
#define UNICODE_UNKNOWN_CHAR_MARK 0xFFFD
#define UNICODE_TCHAR_UKNOWN_CHAR_MARK _T('\xFFFD')

namespace utf8
{
    class InvalidWideCharException {};

    // Terminology -
    //   Code point      - A ordinal value mapped to a standard ideograph as defined by ISO/IEC 10646-1. Here
    //                     also referred to as a UCS code point but can also be often be referred to as a UNICODE
    //                     code point.
    //   UTF-8           - An encoding of UCS code points as defined by RFC-3629.
    //   UTF-16          - An encoding of UCS code points as defined by RFC-2781. Use as a synonym for UNICODE or
    //                     UCS-2. This is technically incorrect but usually harmless. This file assumes char16 *
    //                     maps to an UTF-16LE (little-endian) encoded sequence of words.
    //   Unit            - The unit of encoding. For UTF-8 it is a byte (octet). For UTF-16 it is a word (two octets).
    //   Valid           - A UTF-8 byte sequence conforming to RFC-3629.
    //   Well-formed     - A sequence of bytes that conform to the encoding pattern of UTF8 but might be too long or
    //                     otherwise invalid. For example C0 80 is a well-formed but invalid encoding of U+0000.
    //   Start byte      - A byte can start a well-formed UTF-8 sequence.
    //   Lead byte       - A byte can start a well-formed multi-unit sequence but not a single byte sequence.
    //   Trail byte      - A byte that can appear after a lead-byte in a well-formed multi-unit sequence.
    //   Surrogate pair  - A UTF-16 word pair to encode characters outside the Unicode base plain as defined by
    //                     RFC-2781. Two char16 values are used to encode one UCS code point.
    //   character index - The index into a UTF-16 sequence.
    //   byte index      - The index into a UTF-8 sequence.

    // Return the number of bytes needed to encode the given character (ignoring surrogate pairs)
    inline size_t EncodedSize(char16 ch)
    {
        if (ch < 0x0080) return 1;
        if (ch < 0x0800) return 2;
        return 3;
    }

    enum DecodeOptions
    {
        doDefault                   = 0x00,
        doAllowThreeByteSurrogates  = 0x01, // Allow invalid 3 byte encodings as would be encoded by CSEU-8
        doChunkedEncoding           = 0x02, // For sequences at the end of a buffer do not advance into incomplete sequences
                                            //   If incomplete UTF-8 sequence is encountered at the end of a buffer, this
                                            //   option will cause Decode() to not advance the ptr value and DecodeTail to
                                            //   move the pointer back one position so it again points to where c1 was read by
                                            //   Decode(). In effect, incomplete sequences are treated as if end pointed to the
                                            //   beginning incomplete sequence instead of in the middle of it.
        doSecondSurrogatePair       = 0x04, // A previous call to DecodeTail returned the first word of a UTF-16
                                            // surrogate pair. The second call will return the second word and reset
                                            // this 'option'.
        doAllowInvalidWCHARs        = 0x08, // Don't replace invalid wide chars with 0xFFFD
        doThrowOnInvalidWCHARs      = 0x10, // throw InvalidWideCharException if an invalid wide char is seen. Incompatible with doAllowInvalidWCHARs
    };
    DEFINE_ENUM_FLAG_OPERATORS(DecodeOptions);

    BOOL IsValidWideChar(char16 ch);

    const char16 WCH_UTF16_HIGH_FIRST = char16(0xd800);
    const char16 WCH_UTF16_HIGH_LAST = char16(0xdbff);
    const char16 WCH_UTF16_LOW_FIRST = char16(0xdc00);
    const char16 WCH_UTF16_LOW_LAST = char16(0xdfff);

    inline BOOL InRange(const char16 ch, const char16 chMin, const char16 chMax)
    {
        return (unsigned)(ch - chMin) <= (unsigned)(chMax - chMin);
    }

    inline BOOL IsHighSurrogateChar(char16 ch)
    {
        return InRange(ch, WCH_UTF16_HIGH_FIRST, WCH_UTF16_HIGH_LAST);
    }

    inline BOOL IsLowSurrogateChar(char16 ch)
    {
        return InRange(ch, WCH_UTF16_LOW_FIRST, WCH_UTF16_LOW_LAST);
    }

    // Decode the trail bytes after the UTF8 lead byte c1 but returning 0xFFFD if trail bytes are expected after end.
    _At_(ptr, _In_reads_(end - ptr) _Post_satisfies_(ptr >= _Old_(ptr) - 1 && ptr <= end))
    char16 DecodeTail(char16 c1, LPCUTF8& ptr, LPCUTF8 end, DecodeOptions& options, bool *chunkEndsAtTruncatedSequence = nullptr);

    // Decode the UTF8 sequence into a UTF16 encoding. Code points outside the Unicode base plain will generate
    // surrogate pairs, using the 'doSecondSurrogatePair' option to remember the first word has already been returned.
    // If ptr == end 0x0000 is emitted. If ptr < end but the lead byte of the UTF8 sequence
    // expects trail bytes past end then 0xFFFD are emitted until ptr == end.
    _At_(ptr, _In_reads_(end - ptr) _Post_satisfies_(ptr >= _Old_(ptr) && ptr <= end))
    inline char16 Decode(LPCUTF8& ptr, LPCUTF8 end, DecodeOptions& options, bool *chunkEndsAtTruncatedSequence = nullptr)
    {
        if (ptr >= end) return 0;
        utf8char_t c1 = *ptr++;
        if (c1 < 0x80) return static_cast<char16>(c1);
        return DecodeTail(c1, ptr, end, options, chunkEndsAtTruncatedSequence);
    }

    // Encode ch into a UTF8 sequence ignoring surrogate pairs (which are encoded as two
    // separate code points). Use Encode() instead of EncodeFull() directly because it
    // special cases ASCII to avoid a call the most common characters.
    template <bool countBytesOnly>
    LPUTF8 EncodeFull(char16 ch, __out_ecount(3) LPUTF8 ptr, const void * bufferEnd)
    {
        if (ch < 0x0080)
        {
            // One byte
            if (countBytesOnly)
            {
                ptr++;
            }
            else
            {
                CodexAssertOrFailFast(ptr < bufferEnd);
                *ptr++ = static_cast<utf8char_t>(ch);
            }
        }
        else if (ch < 0x0800)
        {
            // Two bytes   : 110yyyxx 10xxxxxx
            if (countBytesOnly)
            {
                ptr += 2;
            }
            else
            {
                CodexAssertOrFailFast(ptr + 2 <= bufferEnd);
                *ptr++ = static_cast<utf8char_t>(ch >> 6) | 0xc0;
                *ptr++ = static_cast<utf8char_t>(ch & 0x3F) | 0x80;
            }
        }
        else
        {
            // Three bytes : 1110yyyy 10yyyyxx 10xxxxxx
            if (countBytesOnly)
            {
                ptr += 3;
            }
            else
            {
                CodexAssertOrFailFast(ptr + 3 <= bufferEnd);
                *ptr++ = static_cast<utf8char_t>(ch >> 12) | 0xE0;
                *ptr++ = static_cast<utf8char_t>((ch >> 6) & 0x3F) | 0x80;
                *ptr++ = static_cast<utf8char_t>(ch & 0x3F) | 0x80;
            }
        }

        return ptr;
    }

    // Encode a surrogate pair into a utf8 sequence
    template <bool countBytesOnly>
        LPUTF8 EncodeSurrogatePair(char16 surrogateHigh, char16 surrogateLow, __out_ecount(4) LPUTF8 ptr)
    {
        // A unicode codepoint is encoded into a surrogate pair by doing the following:
        //  subtract 0x10000 from the codepoint
        //  Split the resulting value into the high-ten bits and low-ten bits
        //  Add 0xD800 to the high ten bits, and 0xDC00 to the low ten bits
        // Below, we want to decode the surrogate pair to its original codepoint
        // So we do the above process in reverse
        uint32 highTen = (surrogateHigh - 0xD800);
        uint32 lowTen = (surrogateLow - 0xDC00);
        uint32 codepoint = 0x10000 + ((highTen << 10) | lowTen);

        // This is the maximum valid unicode codepoint
        // This should be ensured anyway since you can't encode a value higher
        // than this as a surrogate pair, so we assert this here
        CodexAssert(codepoint <= 0x10FFFF);

        // Now we need to encode the code point into utf-8
        // Codepoints in the range that gets encoded into a surrogate pair
        // gets encoded into 4 bytes under utf8
        // Since the codepoint can be represented by 21 bits, the encoding
        // does the following: first 3 bits in the first byte, the next 6 in the
        // second, the next six in the third, and the last six in the 4th byte
        if (countBytesOnly) {
            ptr += 4;
        }
        else
        {
            *ptr++ = static_cast<utf8char_t>(codepoint >> 18) | 0xF0;
            *ptr++ = static_cast<utf8char_t>((codepoint >> 12) & 0x3F) | 0x80;
            *ptr++ = static_cast<utf8char_t>((codepoint >> 6) & 0x3F) | 0x80;
            *ptr++ = static_cast<utf8char_t>(codepoint & 0x3F) | 0x80;
        }

        return ptr;
    }

    // Encode ch into a UTF8 sequence ignoring surrogate pairs (which are encoded as two
    // separate code points).
    template <bool countBytesOnly>
    inline LPUTF8 Encode(char16 ch, _When_(!countBytesOnly, __out_ecount(3)) LPUTF8 ptr, const void * bufferEnd)
    {
        if (ch < 0x80)
        {
            if (!countBytesOnly)
            {
                CodexAssertOrFailFast(ptr < bufferEnd);
                *ptr = static_cast<utf8char_t>(ch);
            }
            return ptr + 1;
        }
        return EncodeFull<countBytesOnly>(ch, ptr, bufferEnd);
    }

    // Encode ch into a UTF8 sequence while being aware of surrogate pairs.
    template <bool countBytesOnly>
    inline LPUTF8 EncodeTrueUtf8(char16 ch, const char16** source, charcount_t* cch, _When_(!countBytesOnly, __out_ecount((*cch + 1) * 3)) LPUTF8 ptr, const void * bufferEnd)
    {
        if (ch < 0x80)
        {
            if (!countBytesOnly)
            {
                CodexAssertOrFailFast(ptr < bufferEnd);
                *ptr = static_cast<utf8char_t>(ch);
            }

            return ptr + 1;
        }
        else if (ch < 0xD800 || (ch >= 0xE000 && ch <= 0xFFFF))
        {
            return EncodeFull<countBytesOnly>(ch, ptr, bufferEnd);
        }

        // We're now decoding a surrogate pair. If the input is malformed (eg. low surrogate is absent)
        // we'll instead encode the unicode replacement character as utf8
        if (*cch > 0)
        {
            char16 surrogateHigh = ch;
            char16 surrogateLow = **source;

            // Validate that the surrogate code units are within the appropriate
            // ranges for high and low surrogates
            if ((surrogateHigh >= 0xD800 && surrogateHigh <= 0xDBFF) &&
                (surrogateLow >= 0xDC00 && surrogateLow <= 0xDFFF))
            {
                LPUTF8 retptr = EncodeSurrogatePair<countBytesOnly>(surrogateHigh, surrogateLow, ptr);

                // SAL analysis gets confused if we call EncodeSurrogatePair after
                // modifying cch

                // Consume the low surrogate
                *source = *source + 1;
                *cch = *cch - 1;

                return retptr;
            }
        }

        // Invalid input: insert the unicode replacement character instead
        if (!countBytesOnly)
        {
            CodexAssertOrFailFast(ptr + 3 <= bufferEnd);
            ptr[0] = 0xEF;
            ptr[1] = 0xBF;
            ptr[2] = 0xBD;
        }
        return ptr + 3;
    }

    // Return true if ch is a lead byte of a UTF8 multi-unit sequence.
    inline bool IsLeadByte(utf8char_t ch)
    {
        return ch >= 0xC0;
    }

    // Return true if ch is a byte that starts a well-formed UTF8 sequence (i.e. is an ASCII character or a valid UTF8 lead byte)
    inline bool IsStartByte(utf8char_t ch)
    {
        return ch < 0x80 || ch >= 0xC0;
    }

    // Returns true if ch is a UTF8 multi-unit sequence trail byte.
    inline bool IsTrailByte(utf8char_t ch)
    {
        return (ch & 0xC0) == 0x80;
    }

    // Returns true if ptr points to a well-formed UTF8
    inline bool IsCharStart(LPCUTF8 ptr)
    {
        return IsStartByte(*ptr);
    }

    // Return the start of the next well-formed UTF-8 sequence. Use NextChar() instead of
    // NextCharFull() since NextChar() avoid a call if ptr references a single byte sequence.
    LPCUTF8 NextCharFull(LPCUTF8 ptr);

    // Return the start of the next well-formed UTF-8 sequence.
    inline LPCUTF8 NextChar(LPCUTF8 ptr)
    {
        if (*ptr < 0x80) return ptr + 1;
        return NextCharFull(ptr);
    }

    // Return the start of the previous well-formed UTF-8 sequence prior to start or start if
    // if ptr is already start or no well-formed sequence starts a start. Use PrevChar() instead of
    // PrevCharFull() since PrevChar() avoids a call if the previous sequence is a single byte
    // sequence.
    LPCUTF8 PrevCharFull(LPCUTF8 ptr, LPCUTF8 start);

    // Return the start of the previous well-formed UTF-8 sequence prior to start or start if
    // if ptr is already start or no well-formed sequence starts a start.
    inline LPCUTF8 PrevChar(LPCUTF8 ptr, LPCUTF8 start)
    {
        if (ptr > start && *(ptr - 1) < 0x80) return ptr - 1;
        return PrevCharFull(ptr, start);
    }

    // Decode cb bytes from ptr to into buffer returning the number of characters converted and written to buffer
    _Ret_range_(0, pbEnd - _Old_(pbUtf8))
    size_t DecodeUnitsInto(_Out_writes_(pbEnd - pbUtf8) char16 *buffer, LPCUTF8& pbUtf8, LPCUTF8 pbEnd, DecodeOptions options = doDefault, bool *chunkEndsAtTruncatedSequence = nullptr);

    // Decode cb bytes from ptr to into buffer returning the number of characters converted and written to buffer (excluding the null terminator)
    size_t DecodeUnitsIntoAndNullTerminate(__out_ecount(pbEnd - pbUtf8 + 1) __nullterminated char16 *buffer, LPCUTF8& pbUtf8, LPCUTF8 pbEnd, DecodeOptions options = doDefault, bool *chunkEndsAtTruncatedSequence = nullptr);

    size_t DecodeUnitsIntoAndNullTerminateNoAdvance(__out_ecount(pbEnd - pbUtf8 + 1) __nullterminated char16 *buffer, LPCUTF8 pbUtf8, LPCUTF8 pbEnd, DecodeOptions options = doDefault, bool *chunkEndsAtTruncatedSequence = nullptr);

    // Encode a UTF-8 sequence into a UTF-8 sequence (which is just a memcpy). This is included for convenience in templates
    // when the character encoding is a template parameter.
    __range(cbSource, cbDest)
    __precond(cbDest == cbSource)
    inline size_t EncodeInto(
        _Out_writes_(cbDest) utf8char_t *dest,
        size_t cbDest,
        _In_reads_(cbSource) const utf8char_t *source,
        size_t cbSource)
    {
        memcpy_s(dest, cbDest * sizeof(utf8char_t), source, cbSource * sizeof(utf8char_t));
        return cbDest;
    }

    enum class Utf8EncodingKind
    {
        Cesu8,
        TrueUtf8
    };

    // Encode a UTF16-LE sequence of cchSource words (char16) into a UTF-8 sequence returning the number of bytes needed.
    // Since a UTF16 encoding can encode to at most 3 bytes (utf8char_t) per char16, cbDest (dest buffer size) can be
    // at most 3 * cchSource.
    // Returns the number of bytes copied into the dest buffer.
    template <Utf8EncodingKind encoding>
    __range(0, cchSource * 3)
    size_t EncodeInto(
        _Out_writes_(cbDest) utf8char_t *dest,
        __range(0, cchSource * 3) size_t cbDest,
        _In_reads_(cchSource) const char16 *source,
        __range(0, INT_MAX) charcount_t cchSource);

    // Like EncodeInto but ensures that dest[return value] == 0.
    template <Utf8EncodingKind encoding>
    __range(0, cchSource * 3)
    size_t EncodeIntoAndNullTerminate(
        _Out_writes_z_(cbDest) utf8char_t *dest,
        __range(1, cchSource * 3 + 1) size_t cbDest, // must be at least large enough to write null terminator
        _In_reads_(cchSource) const char16 *source,
        __range(0, INT_MAX) charcount_t cchSource);

    // Determine the number of UTF-8 bytes needed to represent a UTF16-LE sequence of cch * words (char16)
    __range(0, cch * 3)
    size_t CountTrueUtf8(__in_ecount(cch) const char16 *source, charcount_t cch);

    // Returns true if the pch refers to a UTF-16LE encoding of the given UTF-8 encoding bch.
    bool CharsAreEqual(LPCOLESTR pch, LPCUTF8 bch, LPCUTF8 end, DecodeOptions options = doDefault);

    // Convert the character index into a byte index.
    size_t CharacterIndexToByteIndex(__in_ecount(cbLength) LPCUTF8 pch, size_t cbLength, const charcount_t cchIndex, size_t cbStartIndex, charcount_t cchStartIndex, DecodeOptions options = doDefault);
    size_t CharacterIndexToByteIndex(__in_ecount(cbLength) LPCUTF8 pch, size_t cbLength, const charcount_t cchIndex, DecodeOptions options = doDefault);

    // Convert byte index into character index
    charcount_t ByteIndexIntoCharacterIndex(__in_ecount(cbIndex) LPCUTF8 pch, size_t cbIndex, DecodeOptions options = doDefault);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
