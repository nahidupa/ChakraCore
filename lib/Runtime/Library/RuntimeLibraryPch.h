//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifndef IsJsDiag
#include "Parser.h"
#include "RegexCommon.h"
#include "Runtime.h"

#include "Base/EtwTrace.h"

#include "Library/JavascriptNumberObject.h"
#include "Library/JavascriptStringObject.h"
#include "Library/JavascriptBooleanObject.h"
#include "Library/JavascriptBigIntObject.h"

#include "Library/ObjectPrototypeObject.h"

#include "Common/ByteSwap.h"
#include "Library/DataView.h"

#include "Library/LazyJSONString.h"
#include "Library/JSONStringBuilder.h"
#include "Library/JSONStringifier.h"
#include "Library/ProfileString.h"
#include "Library/SingleCharString.h"
#include "Library/SubString.h"
#include "Library/BufferStringBuilder.h"

#include "Library/BoundFunction.h"
#include "Library/JavascriptGeneratorFunction.h"
#include "Library/JavascriptAsyncFunction.h"
#include "Library/JavascriptAsyncGeneratorFunction.h"

#include "Library/RegexHelper.h"
#include "Library/JavascriptRegularExpression.h"
#include "Library/JavascriptRegExpConstructor.h"
#include "Library/JavascriptRegularExpressionResult.h"

#include "Library/JavascriptAsyncFromSyncIterator.h"
#include "Library/JavascriptVariantDate.h"
#include "Library/JavascriptPromise.h"
#include "Library/JavascriptSymbolObject.h"
#ifdef _CHAKRACOREBUILD
#include "Library/CustomExternalWrapperObject.h"
#endif
#include "Library/JavascriptProxy.h"
#include "Library/JavascriptReflect.h"
#include "Library/JavascriptGenerator.h"
#include "Library/JavascriptAsyncGenerator.h"

#include "Library/SameValueComparer.h"
#include "Library/MapOrSetDataList.h"
#include "Library/JavascriptMap.h"
#include "Library/JavascriptSet.h"
#include "Library/JavascriptWeakMap.h"
#include "Library/JavascriptWeakSet.h"

#include "Types/UnscopablesWrapperObject.h"
#include "Types/PropertyIndexRanges.h"
#include "Types/DictionaryPropertyDescriptor.h"
#include "Types/DictionaryTypeHandler.h"
#include "Types/ES5ArrayTypeHandler.h"
#include "Library/ES5Array.h"

#include "Library/JavascriptArrayIndexEnumeratorBase.h"
#include "Library/JavascriptArrayIndexEnumerator.h"
#include "Library/JavascriptArrayIndexSnapshotEnumerator.h"
#include "Library/JavascriptArrayIndexStaticEnumerator.h"
#include "Library/ES5ArrayIndexEnumerator.h"
#include "Library/ES5ArrayIndexStaticEnumerator.h"
#include "Library/TypedArrayIndexEnumerator.h"
#include "Library/JavascriptStringEnumerator.h"
#include "Library/JavascriptRegExpEnumerator.h"

#include "Library/JavascriptIterator.h"
#include "Library/JavascriptArrayIterator.h"
#include "Library/JavascriptMapIterator.h"
#include "Library/JavascriptSetIterator.h"
#include "Library/JavascriptStringIterator.h"
#include "Library/JavascriptListIterator.h"

#include "Library/UriHelper.h"
#include "Library/HostObjectBase.h"

#include "Library/DateImplementation.h"
#include "Library/JavascriptDate.h"

#include "Library/ModuleRoot.h"
#include "Library/ArgumentsObject.h"
// SIMD
#include "Language/SimdOps.h"

#include "Library/WebAssemblyInstance.h"

#include "Language/JavascriptStackWalker.h"
#include "Language/CacheOperators.h"
#include "Types/TypePropertyCache.h"
// .inl files
#include "Library/JavascriptString.inl"
#include "Library/ConcatString.inl"
#include "Language/CacheOperators.inl"

#endif // !IsJsDiag

#ifdef IsJsDiag
#define JS_DIAG_INLINE inline
#else
#define JS_DIAG_INLINE
#endif
