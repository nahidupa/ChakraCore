//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "Core/CommonTypedefs.h"
#include "ChakraICU.h"
#include "sal.h"

namespace PlatformAgnostic
{
    namespace UnicodeText
    {
        // This structure is used by a subset of APIs where
        // errors are expected. In that case, ApiError is the last out
        // parameter.
        enum ApiError
        {
            NoError,
            InvalidParameter,
            InvalidUnicodeText,
            InsufficientBuffer,
            OutOfMemory,
            UntranslatedError
        };

        // The form to use for NormalizeString
        // Intentionally compatible with backing Normalization Kind enums if available
        // enum NormalizationForm
        // {
        //     C,     // Each base plus combining characters to the canonical precomposed equivalent.
        //     D,     // Each precomposed character to its canonical decomposed equivalent.
        //     KC,    // Each base plus combining characters to the canonical precomposed
        //            //   equivalents and all compatibility characters to their equivalents.
        //     KD,    // Each precomposed character to its canonical decomposed equivalent
        //            //   and all compatibility characters to their equivalents.
        //     Other, // Not supported
        // };
#if !defined(HAS_ICU) && !_WIN32
        enum NormalizationForm
        {
            C,
            D,
            KC,
            KD,
            Other
        };
#elif !defined(HAS_ICU) && _WIN32
        enum NormalizationForm
        {
            C = NORM_FORM::NormalizationC,
            D = NORM_FORM::NormalizationD,
            KC = NORM_FORM::NormalizationKC,
            KD = NORM_FORM::NormalizationKD,
            Other = NORM_FORM::NormalizationOther
        };
#else
        // ICU does not have specific enums for KC and KD
        // Instead, they have a string argument, "nfc" or "nfkc",
        // coupled with the COMPOSE or DECOMPOSE enum variant
        // ICU does not have an explicit "other", but the static_asserts
        // in UnicodeText.ICU.cpp ensure that Other is different from C/D/KC/KD

        static const char * const ICU_NORMALIZATION_NFC = "nfc";
        static const char * const ICU_NORMALIZATION_NFKC = "nfkc";
        enum NormalizationForm
        {
            C = UNORM2_COMPOSE,
            D = UNORM2_DECOMPOSE,
            KC = INT_MIN + UNORM2_COMPOSE,
            KD = INT_MIN + UNORM2_DECOMPOSE,
            Other = INT_MAX
        };
#endif

        // Mapping of a unicode codepoint to a class of characters
        // Used by the legacy API
        enum class CharacterClassificationType
        {
            Invalid,
            Letter,
            Whitespace,
            NewLine,
            DigitOrPunct
        };

        // Used by the legacy API for mapping
        // a codepoint to a set of flags
        // This remains as an enum rather than an enum class
        // because the global names are referenced widely in CharClassifier
        // so leaving this as is to make the code more readable
        enum CharacterTypeFlags : byte
        {
            UnknownChar = 0x0,
            IdChar = 0x01,
            IdLeadChar = 0x02,
            HexChar = 0x04,
            DecimalChar = 0x08,
            SpaceChar = 0x10,
            LineFeedChar = 0x20,

            LineCharGroup = SpaceChar | LineFeedChar,
            LetterCharGroup = IdChar | IdLeadChar,
            HexCharGroup = IdChar | IdLeadChar | HexChar,
            DecimalCharGroup = IdChar | DecimalChar,
        };

        // Parameters for APIs to change a strings case
        enum CaseFlags
        {
            CaseFlagsUpper,
            CaseFlagsLower
        };

        // Subset of the unicode general category listed in Table 4-9 of the Unicode 8.0 Spec
        // We map the unicode categories into "category classes" at the granularity that the
        // user of this data needs to discriminate the category at.
        enum UnicodeGeneralCategoryClass
        {
            CategoryClassLetter,
            CategoryClassDigit,
            CategoryClassLineSeparator,
            CategoryClassParagraphSeparator,
            CategoryClassSpaceSeparator,
            CategoryClassSpacingCombiningMark,
            CategoryClassNonSpacingMark,
            CategoryClassConnectorPunctuation,
            CategoryClassOther
        };

        //
        // This method normalizes the characters of a given UTF16 string according to the rules of Unicode 4.0 TR#15
        // This is needed for implementation of the ES6 method String.prototoype.normalize
        //
        // Params:
        //   normalizationForm: the Unicode Normalization Form
        //   sourceString: The string to normalize
        //   sourceLength: The number of characters in the source string. This must be provided, the function does not assume null-termination etc. Length should be greater than 0.
        //   destString:   Optional pointer to the destination string buffer. It can be null if destLength is 0.
        //   destLength:   Size in characters of the destination buffer, or 0 if the function shuld just return the required character count for the dest buffer.
        //   pErrorOut:    Set to NoError, or the actual error if one occurred.
        //
        // Return Value:
        //   length of the normalized string in the destination buffer
        //   If the return value is less than or equal to 0, then see the value of pErrorOut to understand the error
        //
        int32 NormalizeString(NormalizationForm normalizationForm, const char16* sourceString, uint32 sourceLength, char16* destString, int32 destLength, ApiError* pErrorOut);

        //
        // This method verifies that a given UTF16 string is normalized according to the rules of Unicode 4.0 TR#15.
        //
        // Params:
        //   normalizationForm: the Unicode Normalization Form
        //   testString: The string to test
        //   testStringLength: The number of characters in the test string. If the string is null-terminated, and the API should calculate the length, set testStringLength to 0.
        //
        // Return Value:
        //   true if the input string is already normalized, false if it isn't
        //   No error codes are returned since they're not used by the caller.
        //
        bool IsNormalizedString(NormalizationForm normalizatingForm, const char16* testString, int32 testStringLength);

        //
        // This method lets the caller know if an external Unicode helper library is being used by the PAL
        // For example, if we're using ICU/Windows.Globalization.dll/JsIntl.dll
        //
        // Return Value:
        //   true if Windows.Globalization or the ICU is available and being used
        //   false otherwise
        //
        bool IsExternalUnicodeLibraryAvailable();

        //
        // Return if a codepoint is considered a whitespace character according to the Unicode spec
        //
        bool IsWhitespace(codepoint_t ch);

        //
        // Return if a codepoint is in the ID_START class according to the Unicode Standard Annex 31.
        // These characters are considered valid to start identifiers for programming languages.
        //
        bool IsIdStart(codepoint_t ch);

        //
        // Return if a codepoint is in the ID_CONTINUE class according to the Unicode Standard Annex 31.
        // These characters are considered valid to be in identifiers for programming languages
        // These are also called identifier characters as they are a superset of ID_Start characters
        //
        bool IsIdContinue(codepoint_t ch);

        //
        // Return the General Category of the unicode code point based on Chapter 4, Section 4.5 of the Unicode 8 Spec
        //
        UnicodeGeneralCategoryClass GetGeneralCategoryClass(codepoint_t ch);

        //
        // Change the case of a string using linguistic rules
        // Params:
        //   sourceString: The string to convert
        //   sourceLength: The number of characters in the source string. This must be provided, the function does not assume null-termination. Length should be greater than 0.
        //   destString:   Optional pointer to the destination string buffer. It can be null if destLength is 0, if you want the required buffer size
        //   destLength:   Size in characters of the destination buffer, or 0 if the function shuld just return the required character count for the dest buffer.
        //   pErrorOut:    Set to NoError, or the actual error if one occurred.
        //
        // Return Value:
        //   The length required to convert sourceString to the given case, even if destString was not large enough to hold it, including the null terminator
        //
        template<bool toUpper, bool useInvariant>
        charcount_t ChangeStringLinguisticCase(_In_count_(sourceLength) const char16* sourceString, _In_ charcount_t sourceLength, _Out_writes_(destLength) char16* destString, _In_ charcount_t destLength, _Out_ ApiError* pErrorOut);

        //
        // Return the classification type of the character using Unicode 2.0 rules
        // Used for ES5 compat
        //
        CharacterClassificationType GetLegacyCharacterClassificationType(char16 character);

        //
        // Return the flags associated with the character using Unicode 2.0 rules
        // Used for ES5 compat
        //
        CharacterTypeFlags GetLegacyCharacterTypeFlags(char16 character);

        //
        // Compares two unicode strings but numbers are compared
        // numerically rather than as text.
        // For example, test2 comes before test11
        //
        // Return Value:
        //     0  - The two strings are equal
        //     -1 - string1 is greater than string2
        //     +1 - string1 is lesser than string2
        //
        int LogicalStringCompare(const char16* string1, int str1size, const char16* string2, int str2size);
    };
};
