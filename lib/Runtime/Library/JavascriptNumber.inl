//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{
#if FLOATVAR
    inline JavascriptNumber::JavascriptNumber(double value, StaticType*
#if DBG
        , bool oopJIT /*= false*/
#endif
    )
    {
        AssertMsg(!IsNan(value) || ToSpecial(value) == k_NegativeNan || ToSpecial(value) == 0x7FF8000000000000ull, "We should only produce a NaN with this value");
        SetSpecial(ToSpecial(value) ^ FloatTag_Value);
    }
#else
    inline JavascriptNumber::JavascriptNumber(double value, StaticType * type
#if DBG
        , bool oopJIT /*= false*/
#endif
    ) : RecyclableObject(type), m_value(value)
    {
        // for oopjit type will be pointing to address of StaticType on other proc, so don't dereference it
        Assert(oopJIT || type->GetTypeId() == TypeIds_Number);
    }
#endif

    __forceinline Var JavascriptNumber::ToVar(int32 nValue, ScriptContext* scriptContext)
    {
        if (!TaggedInt::IsOverflow(nValue))
        {
            return TaggedInt::ToVarUnchecked(nValue);
        }
        else
        {
            return JavascriptNumber::NewInlined((double) nValue, scriptContext);
        }
    }

#ifdef ENABLE_TEST_HOOKS
    __forceinline Var JavascriptNumber::ToVarFor32BitBytecode(int32 nValue, ScriptContext* scriptContext)
    {
        if ((1073741824 > nValue) && (nValue > -1073741824))
        {
            return TaggedInt::ToVarUnchecked(nValue);
        }
        return JavascriptNumber::NewInlined((double) nValue, scriptContext);
    }
#endif

#if defined(__clang__) && defined(_M_IX86)
    __forceinline Var JavascriptNumber::ToVar(intptr_t nValue, ScriptContext* scriptContext)
    {
        if (!TaggedInt::IsOverflow(nValue))
        {
            return TaggedInt::ToVarUnchecked(nValue);
        }
        else
        {
            return JavascriptNumber::NewInlined((double) nValue, scriptContext);
        }
    }
#endif

    inline Var JavascriptNumber::ToVar(uint32 nValue, ScriptContext* scriptContext)
    {
        return !TaggedInt::IsOverflow(nValue) ? TaggedInt::ToVarUnchecked(nValue) :
            JavascriptNumber::New((double) nValue,scriptContext);
    }

    inline Var JavascriptNumber::ToVar(int64 nValue, ScriptContext* scriptContext)
    {
        return !TaggedInt::IsOverflow(nValue) ?
                TaggedInt::ToVarUnchecked((int) nValue) :
                JavascriptNumber::New((double) nValue,scriptContext);
    }

    inline Var JavascriptNumber::ToVar(uint64 nValue, ScriptContext* scriptContext)
    {
        return !TaggedInt::IsOverflow(nValue) ?
                TaggedInt::ToVarUnchecked((int) nValue) :
                JavascriptNumber::New((double) nValue,scriptContext);
    }

    inline bool JavascriptNumber::TryToVarFast(int32 nValue, Var* result)
    {
        if (!TaggedInt::IsOverflow(nValue))
        {
            *result = TaggedInt::ToVarUnchecked(nValue);
            return true;
        }

#if FLOATVAR
        *result = JavascriptNumber::ToVar((double)nValue);
        return true;
#else
        return false;
#endif
    }

    inline bool JavascriptNumber::TryToVarFastWithCheck(double value, Var* result)
    {
#if FLOATVAR
        if (IsNan(value))
        {
            value = IsNegative(value) ? JavascriptNumber::NegativeNaN : JavascriptNumber::NaN;
        }

        *result = JavascriptNumber::ToVar(value);
        return true;
#else
        return false;
#endif
    }

#if FLOATVAR
    inline bool JavascriptNumber::Is(Var aValue)
    {
        return Is_NoTaggedIntCheck(aValue);
    }

    inline JavascriptNumber* JavascriptNumber::InPlaceNew(double value, ScriptContext* scriptContext, Js::JavascriptNumber *result)
    {
        AssertMsg( result != NULL, "Cannot use InPlaceNew without a value result location" );
        result = (JavascriptNumber*)ToVar(value);
        return result;
    }

    inline Var JavascriptNumber::New(double value, ScriptContext* scriptContext)
    {
        return ToVar(value);
    }

    inline Var JavascriptNumber::NewWithCheck(double value, ScriptContext* scriptContext)
    {
        if (IsNan(value))
        {
            value = IsNegative(value) ? JavascriptNumber::NegativeNaN : JavascriptNumber::NaN;
        }
        return ToVar(value);
    }

    inline Var JavascriptNumber::NewInlined(double value, ScriptContext* scriptContext)
    {
        return ToVar(value);
    }

#if ENABLE_NATIVE_CODEGEN
    inline Var JavascriptNumber::NewCodeGenInstance(double value, ScriptContext* scriptContext)
    {
        return ToVar(value);
    }
#endif

    inline Var JavascriptNumber::ToVar(double value)
    {
        const uint64 val = ToSpecial(value);
        AssertMsg(!IsNan(value) || ToSpecial(value) == k_NegativeNan || ToSpecial(value) == 0x7FF8000000000000ull, "We should only produce a NaN with this value");
        return reinterpret_cast<Var>(val ^ FloatTag_Value);
    }

#else
    inline bool JavascriptNumber::Is(Var aValue)
    {
        return !TaggedInt::Is(aValue) && Is_NoTaggedIntCheck(aValue);
    }

#if !defined(USED_IN_STATIC_LIB)
    inline bool JavascriptNumber::Is_NoTaggedIntCheck(Var aValue)
    {
        RecyclableObject* object = VarTo<RecyclableObject>(aValue);
        AssertMsg((object->GetTypeId() == TypeIds_Number) == VirtualTableInfo<JavascriptNumber>::HasVirtualTable(object), "JavascriptNumber has no unique VTABLE?");
        return VirtualTableInfo<JavascriptNumber>::HasVirtualTable(object);
    }
#endif

    inline JavascriptNumber* JavascriptNumber::FromVar(Var aValue)
    {
        AssertOrFailFastMsg(Is(aValue), "Ensure var is actually a 'JavascriptNumber'");

        return reinterpret_cast<JavascriptNumber *>(aValue);
    }

    inline JavascriptNumber* JavascriptNumber::UnsafeFromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptNumber'");

        return reinterpret_cast<JavascriptNumber *>(aValue);
    }

    inline double JavascriptNumber::GetValue(Var aValue)
     {
         AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptNumber'");

         return JavascriptNumber::FromVar(aValue)->GetValue();
     }

    inline JavascriptNumber* JavascriptNumber::InPlaceNew(double value, ScriptContext* scriptContext, Js::JavascriptNumber *result)
    {
        AssertMsg( result != NULL, "Cannot use InPlaceNew without a value result location" );
        Assume(result != NULL); // Encourage the compiler to omit a NULL check on the return from placement new
        return ::new(result) JavascriptNumber(value, scriptContext->GetLibrary()->GetNumberTypeStatic());
    }

    inline Var JavascriptNumber::New(double value, ScriptContext* scriptContext)
    {
        return scriptContext->GetLibrary()->CreateNumber(value, scriptContext->GetNumberAllocator());
    }

    inline Var JavascriptNumber::NewWithCheck(double value, ScriptContext* scriptContext)
    {
        return scriptContext->GetLibrary()->CreateNumber(value, scriptContext->GetNumberAllocator());
    }

    inline Var JavascriptNumber::NewInlined(double value, ScriptContext* scriptContext)
    {
        return scriptContext->GetLibrary()->CreateNumber(value, scriptContext->GetNumberAllocator());
    }

#if ENABLE_NATIVE_CODEGEN
    inline Var JavascriptNumber::NewCodeGenInstance(CodeGenNumberAllocator *alloc, double value, ScriptContext* scriptContext)
    {
        return scriptContext->GetLibrary()->CreateCodeGenNumber(alloc, value);
    }
#endif

#endif

    inline JavascriptString * JavascriptNumber::ToStringNan(ScriptContext* scriptContext)
    {
        return scriptContext->GetLibrary()->GetNaNDisplayString();
    }

    inline JavascriptString* JavascriptNumber::ToStringNanOrInfinite(double value, ScriptContext* scriptContext)
    {
        if(!NumberUtilities::IsFinite(value))
        {
            if(IsNan(value))
            {
                return ToStringNan(scriptContext);
            }

            if(IsPosInf(value))
            {
                return scriptContext->GetLibrary()->CreateStringFromCppLiteral(_u("Infinity"));
            }
            else
            {
                AssertMsg(IsNegInf(value), "bad handling of infinite number");
                return scriptContext->GetLibrary()->CreateStringFromCppLiteral(_u("-Infinity"));
            }
        }
        return nullptr;
    }

    inline Var JavascriptNumber::FormatDoubleToString( double value, Js::NumberUtilities::FormatType formatType, int formatDigits, ScriptContext* scriptContext )
    {
        static const int bufSize = 256;
        char16 szBuffer[bufSize] = _u("");
        char16 * psz = szBuffer;
        char16 * pszToBeFreed = NULL;
        int nOut;

        if ((nOut = Js::NumberUtilities::FDblToStr(value, formatType, formatDigits, szBuffer, bufSize)) > bufSize )
        {
            int nOut1;
            pszToBeFreed = psz = (char16 *)malloc(nOut * sizeof(char16));
            if(0 == psz)
            {
                Js::JavascriptError::ThrowOutOfMemoryError(scriptContext);
            }

            nOut1 = Js::NumberUtilities::FDblToStr(value, Js::NumberUtilities::FormatFixed, formatDigits, psz, nOut);
            Assert(nOut1 == nOut);
        }

        // nOut includes room for terminating NUL
        JavascriptString* result = JavascriptString::NewCopyBuffer(psz, nOut - 1, scriptContext);

        if(pszToBeFreed)
        {
            free(pszToBeFreed);
        }

        return result;
    }
}
