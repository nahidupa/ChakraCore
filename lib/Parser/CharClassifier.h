//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once


//Helpers
static inline BOOL FBigChar(codepoint_t ch) { return ch >= 128u; }
static inline BOOL BoolFromDbl(double dbl) { return !Js::NumberUtilities::IsNan(dbl) && (0 != dbl); }

enum CharTypes
{
    _C_UNK,    // Unknown grouping
    _C_ERR,    // illegal character
    _C_NUL,    // NUL character
    _C_LET,    // letter (A-Z,a-z)
    _C_DIG,    // digit (0-9)
    _C_WSP,    // white space
    _C_NWL,    // new line

    _C_DOL,    // $
    _C_BSL,    // \ (backslash)
    _C_BKQ,    // `
    _C_AT,     // @
    _C_SHP,    // #
    _C_BNG,    // !
    _C_QUO,    // "
    _C_APO,    // '
    _C_PCT,    // %
    _C_AMP,    // &
    _C_LPR,    // (
    _C_RPR,    // )
    _C_PLS,    // +
    _C_MIN,    // -
    _C_MUL,    // *
    _C_SLH,    // /
    _C_XOR,    // ^
    _C_CMA,    // ,
    _C_DOT,    // .
    _C_LT,     // <
    _C_EQ,     // =
    _C_GT,     // >
    _C_QUE,    // ?
    _C_LBR,    // [
    _C_RBR,    // ]
    _C_USC,    // _
    _C_LC,     // {
    _C_RC,     // }
    _C_BAR,    // |
    _C_TIL,    // ~
    _C_COL,    // :
    _C_SMC,    // ;
};

enum
{
    kchNUL = 0x00,
    kchNWL = 0x0A,
    kchRET = 0x0D,
    kchBSL = '\\',
    kchSHP = '#',
    kchBNG = '!',
    kchQUO = '"',
    kchAPO = '\'',
    kchPCT = '%',
    kchAMP = '&',
    kchLPR = '(',
    kchRPR = ')',
    kchPLS = '+',
    kchMIN = '-',
    kchMUL = '*',
    kchSLH = '/',
    kchXOR = '^',
    kchCMA = ',',
    kchDOT = '.',
    kchLT =  '<',
    kchEQ =  '=',
    kchGT =  '>',
    kchQUE = '?',
    kchLBR = '[',
    kchRBR = ']',
    kchUSC = '_',
    kchLC  = '{',
    kchRC  = '}',
    kchBAR = '|',
    kchTIL = '~',
    kchCOL = ':',
    kchSMC = ';',
    kchLS = 0x2028, //classifies as new line
    kchPS = 0x2029  //classifies as new line

};

namespace Js
{
    class WindowsGlobalizationAdapter;
    class DelayLoadWindowsGlobalization;

    typedef
    enum CharClassifierModes {
        ES5 = 1,
        ES6 = 2
    } CharClassifierModes;

    class CharClassifier
    {
    private:
#if ENABLE_UNICODE_API
        static bool BigCharIsWhitespaceDefault(codepoint_t ch, const CharClassifier *instance);
        static bool BigCharIsIdStartDefault(codepoint_t ch, const CharClassifier *instance);
        static bool BigCharIsIdContinueDefault(codepoint_t ch, const CharClassifier *instance);
#endif

        static bool BigCharIsWhitespaceES6(codepoint_t ch, const CharClassifier *instance);
        static bool BigCharIsIdStartES6(codepoint_t codePoint, const CharClassifier *instance);
        static bool BigCharIsIdContinueES6(codepoint_t codePoint, const CharClassifier *instance);

        static CharTypes GetBigCharTypeES6(codepoint_t ch, const CharClassifier *instance);
        static PlatformAgnostic::UnicodeText::CharacterTypeFlags GetBigCharFlagsES6(codepoint_t ch, const CharClassifier *instance);

        static CharTypes GetBigCharTypeES5(codepoint_t ch, const CharClassifier *instance);
        static PlatformAgnostic::UnicodeText::CharacterTypeFlags GetBigCharFlagsES5(codepoint_t ch, const CharClassifier *instance);

        static const OLECHAR* SkipWhiteSpaceSurrogate(LPCOLESTR psz, const CharClassifier *instance);
        static const OLECHAR* SkipWhiteSpaceSurrogateStartEnd(_In_reads_(pStrEnd - pStr) LPCOLESTR pStr, _In_ LPCOLESTR pStrEnd, const CharClassifier *instance);
        static const OLECHAR* SkipIdentifierSurrogate(LPCOLESTR psz, const CharClassifier *instance);
        static const LPCUTF8 SkipIdentifierSurrogateStartEnd(LPCUTF8 psz, LPCUTF8 end, const CharClassifier *instance);

        static const OLECHAR* SkipWhiteSpaceNonSurrogate(LPCOLESTR psz, const CharClassifier *instance);
        static const OLECHAR* SkipWhiteSpaceNonSurrogateStartEnd(_In_reads_(pStrEnd - pStr) LPCOLESTR pStr, _In_ LPCOLESTR pStrEnd, const CharClassifier *instance);
        static const OLECHAR* SkipIdentifierNonSurrogate(LPCOLESTR psz, const CharClassifier *instance);
        static const LPCUTF8 SkipIdentifierNonSurrogateStartEnd(LPCUTF8 psz, LPCUTF8 end, const CharClassifier *instance);

        CharTypes (*getBigCharTypeFunc)(codepoint_t ch, const CharClassifier *instance);
        PlatformAgnostic::UnicodeText::CharacterTypeFlags (*getBigCharFlagsFunc)(codepoint_t ch, const CharClassifier *instance);

        bool (*bigCharIsWhitespaceFunc)(codepoint_t ch, const CharClassifier *instance);
        bool (*bigCharIsIdStartFunc)(codepoint_t ch, const CharClassifier *instance);
        bool (*bigCharIsIdContinueFunc)(codepoint_t ch, const CharClassifier *instance);

        const OLECHAR* (*skipWhiteSpaceFunc)(LPCOLESTR psz, const CharClassifier* instance);
        const OLECHAR* (*skipWhiteSpaceStartEndFunc)(LPCOLESTR pStr, LPCOLESTR pStrEnd, const CharClassifier* instance);

        const OLECHAR* (*skipIdentifierFunc)(LPCOLESTR pcz, const CharClassifier* instance);
        const LPCUTF8 (*skipIdentifierStartEndFunc)(LPCUTF8 psz, LPCUTF8 end, const CharClassifier* instance);

    public:

        CharTypes GetCharType(codepoint_t ch) const;

#if ENABLE_UNICODE_API
        PlatformAgnostic::UnicodeText::CharacterTypeFlags GetCharFlags(codepoint_t ch) const;
#endif

        template <bool isBigChar>
        bool IsWhiteSpaceFast(codepoint_t ch) const;

        bool IsWhiteSpace(codepoint_t ch) const
        {
            return FBigChar(ch) ? IsWhiteSpaceFast<true>(ch) : IsWhiteSpaceFast<false>(ch);
        }

        bool IsBiDirectionalChar(codepoint_t ch) const;

        template<bool isBigChar>
        bool IsIdStartFast(codepoint_t ch) const;

        bool IsIdStart(codepoint_t ch) const
        {
            return FBigChar(ch) ? IsIdStartFast<true>(ch) : IsIdStartFast<false>(ch);
        }

        template<bool isBigChar>
        bool IsIdContinueFast(codepoint_t ch) const;

        bool IsIdContinue(codepoint_t ch) const
        {
            return FBigChar(ch) ? IsIdContinueFast<true>(ch) : IsIdContinueFast<false>(ch);
        }

        size_t SkipBiDirectionalChars(_In_z_bytecount_(2 * length) LPCOLESTR psz, _In_ size_t startIndex, _In_ size_t length) const
        {
            size_t count = 0;
            while (startIndex < length)
            {
                if (!IsBiDirectionalChar(psz[startIndex + count]))
                {
                    return count;
                }

                count++;
            }

            return count;
        }

        char16 SkipBiDirectionalChars(_In_z_ char16* &pszRef) const
        {
            while (*pszRef != '\0')
            {
                if (!IsBiDirectionalChar(*pszRef))
                {
                    return *pszRef;
                }

                pszRef++;
            }

            return '\0';
        }

        const OLECHAR* SkipWhiteSpace(LPCOLESTR psz) const
        {
            // Fast path for the case in which first character is not space
            char16 firstChar = *psz;
            if (firstChar == 0)
            {
                return psz;
            }
            if (!this->IsWhiteSpace(firstChar) &&
                (skipWhiteSpaceFunc != &SkipWhiteSpaceSurrogate
                || !Js::NumberUtilities::IsSurrogateLowerPart(firstChar)))
            {
                return psz;
            }
            return skipWhiteSpaceFunc(psz, this);
        }

        const OLECHAR* SkipWhiteSpace(_In_reads_(pStrEnd - pStr) LPCOLESTR pStr, _In_ LPCOLESTR pStrEnd) const
        {
            // Fast path for the case in which first character is not space
            if (pStr == pStrEnd)
            {
                return pStr;
            }
            char16 firstChar = *pStr;
            if (!this->IsWhiteSpace(firstChar) &&
                (skipWhiteSpaceStartEndFunc != &SkipWhiteSpaceSurrogateStartEnd
                || !Js::NumberUtilities::IsSurrogateLowerPart(firstChar)))
            {
                return pStr;
            }
            return skipWhiteSpaceStartEndFunc(pStr, pStrEnd, this);
        }

        const OLECHAR* SkipIdentifier(LPCOLESTR psz) const
        {
            return skipIdentifierFunc(psz, this);
        }

        LPCUTF8 SkipIdentifier(LPCUTF8 psz, LPCUTF8 end) const
        {
            return skipIdentifierStartEndFunc(psz, end, this);
        }

        const OLECHAR* SkipIdentifier(LPCOLESTR psz, LPCOLESTR end) const
        {
            return SkipIdentifier(psz);
        }

        CharClassifier(void);
    };

};
