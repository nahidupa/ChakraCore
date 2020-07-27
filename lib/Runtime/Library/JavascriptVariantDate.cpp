//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    Var JavascriptVariantDate::GetTypeOfString(ScriptContext* requestContext)
    {
        return requestContext->GetLibrary()->GetVariantDateTypeDisplayString();
    }

    JavascriptString* JavascriptVariantDate::GetValueString(ScriptContext* scriptContext)
    {
        return DateImplementation::ConvertVariantDateToString(this->value, scriptContext);
    }

    BOOL JavascriptVariantDate::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        BOOL ret;

        ENTER_PINNED_SCOPE(JavascriptString, resultString);
        resultString = DateImplementation::ConvertVariantDateToString(this->value, GetScriptContext());
        if (resultString != nullptr)
        {
            stringBuilder->Append(resultString->GetString(), resultString->GetLength());
            ret = TRUE;
        }
        else
        {
            ret = FALSE;
        }

        LEAVE_PINNED_SCOPE();

        return ret;
    }

    BOOL JavascriptVariantDate::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Date")); // For whatever reason in IE8 jscript, typeof returns "date"
                                                  // while the debugger displays "Date" for the type
        return TRUE;
    }

    RecyclableObject * JavascriptVariantDate::CloneToScriptContext(ScriptContext* requestContext)
    {
        return requestContext->GetLibrary()->CreateVariantDate(value);
    }

    RecyclableObject* JavascriptVariantDate::ToObject(ScriptContext* requestContext)
    {
        // WOOB 1124298: Just return a new object when converting to object.
        return requestContext->GetLibrary()->CreateObject(true);
    }

    PropertyQueryFlags JavascriptVariantDate::GetPropertyQuery(Js::Var originalInstance, Js::PropertyId propertyId, Js::Var* value, PropertyValueInfo* info, Js::ScriptContext* requestContext)
    {
        if (requestContext->GetThreadContext()->RecordImplicitException())
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_Property_VarDate, requestContext->GetPropertyName(propertyId)->GetBuffer());
        }
        *value = nullptr;
        return PropertyQueryFlags::Property_Found;
    };

    PropertyQueryFlags JavascriptVariantDate::GetPropertyQuery(Js::Var originalInstance, Js::JavascriptString* propertyNameString, Js::Var* value, PropertyValueInfo* info, Js::ScriptContext* requestContext)
    {
        if (requestContext->GetThreadContext()->RecordImplicitException())
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_Property_VarDate, propertyNameString);
        }
        *value = nullptr;
        return PropertyQueryFlags::Property_Found;
    };

    PropertyQueryFlags JavascriptVariantDate::GetPropertyReferenceQuery(Js::Var originalInstance, Js::PropertyId propertyId, Js::Var* value, PropertyValueInfo* info, Js::ScriptContext* requestContext)
    {
        if (requestContext->GetThreadContext()->RecordImplicitException())
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_Property_VarDate, requestContext->GetPropertyName(propertyId)->GetBuffer());
        }
        *value = nullptr;
        return PropertyQueryFlags::Property_Found;
    };

    BOOL JavascriptVariantDate::SetProperty(Js::PropertyId propertyId, Js::Var value, Js::PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_VarDate, scriptContext->GetPropertyName(propertyId)->GetBuffer());
    };

    BOOL JavascriptVariantDate::SetProperty(Js::JavascriptString* propertyNameString, Js::Var value, Js::PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_VarDate, propertyNameString->GetSz());
    };

    BOOL JavascriptVariantDate::InitProperty(Js::PropertyId propertyId, Js::Var value, PropertyOperationFlags flags, Js::PropertyValueInfo* info)
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_VarDate, scriptContext->GetPropertyName(propertyId)->GetBuffer());
    };

    BOOL JavascriptVariantDate::DeleteProperty(Js::PropertyId propertyId, Js::PropertyOperationFlags flags)
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_VarDate, scriptContext->GetPropertyName(propertyId)->GetBuffer());
    };

    BOOL JavascriptVariantDate::DeleteProperty(JavascriptString *propertyNameString, Js::PropertyOperationFlags flags)
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_VarDate, propertyNameString->GetString());
    };

    PropertyQueryFlags JavascriptVariantDate::GetItemReferenceQuery(Js::Var originalInstance, uint32 index, Js::Var* value, Js::ScriptContext * scriptContext)
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_VarDate, JavascriptNumber::ToStringRadix10(index, scriptContext)->GetSz());
    };

    PropertyQueryFlags JavascriptVariantDate::GetItemQuery(Js::Var originalInstance, uint32 index, Js::Var* value, Js::ScriptContext * scriptContext)
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_VarDate, JavascriptNumber::ToStringRadix10(index, scriptContext)->GetSz());
    };

    BOOL JavascriptVariantDate::SetItem(uint32 index, Js::Var value, Js::PropertyOperationFlags flags)
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_VarDate, JavascriptNumber::ToStringRadix10(index, scriptContext)->GetSz());
    };

    BOOL JavascriptVariantDate::ToPrimitive(JavascriptHint hint, Var* result, ScriptContext * requestContext)
    {
        if (hint == JavascriptHint::HintString)
        {
            JavascriptString* resultString = this->GetValueString(requestContext);
            if (resultString != nullptr)
            {
                (*result) = resultString;
                return TRUE;
            }
            Assert(false);
        }
        else if (hint == JavascriptHint::HintNumber)
        {
            *result = JavascriptNumber::ToVarNoCheck(DateImplementation::JsUtcTimeFromVarDate(value, requestContext), requestContext);
            return TRUE;
        }
        else
        {
            Assert(hint == JavascriptHint::None);
            *result = this;
            return TRUE;
        }
        return FALSE;
    }

    BOOL JavascriptVariantDate::Equals(Var other, BOOL *value, ScriptContext * requestContext)
    {
        // Calling .Equals on a VT_DATE variant at least gives the "[property name] is null or not An object error"
        *value = FALSE;
        return TRUE;
    }
}
