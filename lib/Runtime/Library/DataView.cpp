//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//  Implements typed array.
//----------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{

    Var DataView::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        Var newTarget = args.GetNewTarget();
        bool isCtorSuperCall = JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);
        uint32 byteLength = 0;
        uint32 mappedLength;
        int32 offset = 0;
        ArrayBufferBase* arrayBuffer = nullptr;
        DataView* dataView;

        //1.    If NewTarget is undefined, throw a TypeError exception.
        if (!(callInfo.Flags & CallFlags_New) || (newTarget && JavascriptOperators::IsUndefinedObject(newTarget)))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew, _u("DataView"));
        }

        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("buffer"));
        }

        // Currently the only reason we check for an external object is projection related, so it remains under conditional compilation.
        RecyclableObject* jsArraySource = NULL;
        if (JavascriptOperators::IsObject(args[1]) && JavascriptConversion::ToObject(args[1], scriptContext, &jsArraySource))
        {
            ArrayBuffer *ab = nullptr;
            HRESULT hr = scriptContext->GetHostScriptContext()->ArrayBufferFromExternalObject(jsArraySource, &ab);
            switch (hr)
            {
            case S_OK:
            case S_FALSE:
                arrayBuffer = ab;
                // Both of these cases will be handled by the arrayBuffer null check.
                break;

            default:
                // Any FAILURE HRESULT or unexpected HRESULT.
                JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_InvalidArgument, _u("buffer"));
                break;
            }
        }

        //2.    If Type(buffer) is not Object, throw a TypeError exception.
        //3.    If buffer does not have an [[ArrayBufferData]] internal slot, throw a TypeError exception.
        if (arrayBuffer == nullptr)
        {
            if (VarIs<ArrayBufferBase>(args[1]))
            {
                arrayBuffer = VarTo<ArrayBufferBase>(args[1]);
            }
            else
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject, _u("buffer"));
            }
        }

        //4.    Let offset be ToIndex(byteOffset).
        if (args.Info.Count > 2)
        {
            Var secondArgument = args[2];
            offset = ArrayBuffer::ToIndex(secondArgument, JSERR_ArrayLengthConstructIncorrect, scriptContext, ArrayBuffer::MaxArrayBufferLength, false);
        }

        //5.    If IsDetachedBuffer(buffer) is true, throw a TypeError exception.
        if (arrayBuffer->IsDetached())
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray);
        }

        //6.    Let bufferByteLength be the value of buffer's[[ArrayBufferByteLength]] internal slot.
        //7.   If offset > bufferByteLength, throw a RangeError exception.
        byteLength = arrayBuffer->GetByteLength();
        if ((uint32)offset > byteLength)
        {
            JavascriptError::ThrowRangeError(
                scriptContext, JSERR_DataView_InvalidArgument, _u("byteOffset"));
        }

        //8.   If byteLength is either not present or is undefined, then
        //      a.  Let viewByteLength be bufferByteLength - offset.
        //9.   Else,
        //      a.  Let viewByteLength be ToIndex(byteLength).
        //      b.  If offset + viewByteLength > bufferByteLength, throw a RangeError exception.
        if (args.Info.Count > 3 && !JavascriptOperators::IsUndefinedObject(args[3]))
        {
            Var thirdArgument = args[3];
            mappedLength = ArrayBuffer::ToIndex(thirdArgument, JSERR_ArrayLengthConstructIncorrect, scriptContext, ArrayBuffer::MaxArrayBufferLength, false);
            uint32 viewRange = mappedLength + offset;

            if (viewRange > byteLength || viewRange < mappedLength) // overflow indicates out-of-range
            {
                JavascriptError::ThrowRangeError(
                    scriptContext, JSERR_DataView_InvalidArgument, _u("byteLength"));
            }
        }
        else
        {
            mappedLength = byteLength - offset;
        }

        // Evaluation of the argument(s) above is reentrant and can detach the array.
        if (arrayBuffer->IsDetached())
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray);
        }

        //10.   Let O be OrdinaryCreateFromConstructor(NewTarget, "%DataViewPrototype%", [[DataView]], [[ViewedArrayBuffer]], [[ByteLength]], [[ByteOffset]]).
        //11.   Set O's[[DataView]] internal slot to true.
        //12.   Set O's[[ViewedArrayBuffer]] internal slot to buffer.
        //13.   Set O's[[ByteLength]] internal slot to viewByteLength.
        //14.   Set O's[[ByteOffset]] internal slot to offset.
        //15.   Return O.
        dataView = scriptContext->GetLibrary()->CreateDataView(arrayBuffer, offset, mappedLength);
        return isCtorSuperCall ?
            JavascriptOperators::OrdinaryCreateFromConstructor(VarTo<RecyclableObject>(newTarget), dataView, nullptr, scriptContext) :
            dataView;
    }

    DataView::DataView(ArrayBufferBase* arrayBuffer, uint32 byteoffset, uint32 mappedLength, DynamicType* type)
        : ArrayBufferParent(type, mappedLength, arrayBuffer),
          byteOffset(byteoffset)
    {
        buffer = arrayBuffer->GetBuffer() + byteoffset;
    }

    Var DataView::EntryGetInt8(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<DataView>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
        }
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        return dataView->template GetValue<int8>(args[1], _u("DataView.prototype.GetInt8"), FALSE);
    }

    Var DataView::EntryGetUint8(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<DataView>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
        }
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        return dataView->GetValue<uint8>(args[1], _u("DataView.prototype.GetUint8"), FALSE);
    }

    Var DataView::EntryGetInt16(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        BOOL isLittleEndian = FALSE;

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<DataView>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
        }
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
        }
        if (args.Info.Count > 2)
        {
            isLittleEndian = JavascriptConversion::ToBoolean(args[2], scriptContext);
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        return dataView->GetValue<int16>(args[1], _u("DataView.prototype.GetInt16"), isLittleEndian);
    }

    Var DataView::EntryGetUint16(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        BOOL isLittleEndian = FALSE;

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<DataView>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
        }
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
        }
        if (args.Info.Count > 2)
        {
            isLittleEndian = JavascriptConversion::ToBoolean(args[2], scriptContext);
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        return dataView->template GetValue<uint16>(args[1], _u("DataView.prototype.GetUint16"), isLittleEndian);
    }

    Var DataView::EntryGetUint32(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        BOOL isLittleEndian = FALSE;

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<DataView>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
        }
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
        }
        if (args.Info.Count > 2)
        {
            isLittleEndian = JavascriptConversion::ToBoolean(args[2], scriptContext);
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        return dataView->GetValue<uint32>(args[1], _u("DataView.prototype.GetUint32"), isLittleEndian);
    }

    Var DataView::EntryGetInt32(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        BOOL isLittleEndian = FALSE;

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<DataView>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
        }
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
        }
        if (args.Info.Count > 2)
        {
            isLittleEndian = JavascriptConversion::ToBoolean(args[2], scriptContext);
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        return dataView->GetValue<int32>(args[1], _u("DataView.prototype.GetInt32"), isLittleEndian);
    }

    Var DataView::EntryGetFloat32(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        BOOL isLittleEndian = FALSE;

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<DataView>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
        }
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
        }
        if (args.Info.Count > 2)
        {
            isLittleEndian = JavascriptConversion::ToBoolean(args[2], scriptContext);
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        return dataView->GetValueWithCheck<float>(args[1], _u("DataView.prototype.GetFloat32"), isLittleEndian);
    }

    Var DataView::EntryGetFloat64(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        BOOL isLittleEndian = FALSE;

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<DataView>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
        }
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
        }
        if (args.Info.Count > 2)
        {
            isLittleEndian = JavascriptConversion::ToBoolean(args[2], scriptContext);
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        return dataView->GetValueWithCheck<double>(args[1], _u("DataView.prototype.GetFloat64"), isLittleEndian);
    }

    Var DataView::EntrySetInt8(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        int8 value = 0;
        switch (args.Info.Count)
        {
        case 0:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
            break;
        case 1:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
            break;
        default:
            if (!VarIs<DataView>(args[0]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
                break;
            }
            if (args.Info.Count > 2)
            {
                value = JavascriptConversion::ToInt8(args[2], scriptContext);
            }
            break;
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        dataView->SetValue<int8>(args[1], value, _u("DataView.prototype.SetInt8"));
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var DataView::EntrySetUint8(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        uint8 value = 0;
        switch (args.Info.Count)
        {
        case 0:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
            break;
        case 1:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
            break;
        default:
            if (!VarIs<DataView>(args[0]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
                break;
            }
            if (args.Info.Count > 2)
            {
                value = JavascriptConversion::ToUInt8(args[2], scriptContext);
            }
            break;
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        dataView->SetValue<uint8>(args[1], value, _u("DataView.prototype.SetUint8"));
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var DataView::EntrySetInt16(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        BOOL isLittleEndian = FALSE;
        int16 value = 0;
        switch (args.Info.Count)
        {
        case 0:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
            break;
        case 1:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
            break;
        default:
            if (!VarIs<DataView>(args[0]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
                break;
            }
            if (args.Info.Count > 2)
            {
                if (!JavascriptOperators::IsUndefined(args[2]))
                {
                    value = JavascriptConversion::ToInt16(args[2], scriptContext);
                }
                if (args.Info.Count > 3)
                {
                    isLittleEndian = JavascriptConversion::ToBoolean(args[3], scriptContext);
                }
            }
            break;
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        dataView->SetValue<int16>(args[1], value, _u("DataView.prototype.SetInt16"), isLittleEndian);
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var DataView::EntrySetUint16(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        BOOL isLittleEndian = FALSE;
        uint16 value = 0;
        switch (args.Info.Count)
        {
        case 0:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
            break;
        case 1:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
            break;
        default:
            if (!VarIs<DataView>(args[0]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
                break;
            }
            if (args.Info.Count > 2)
            {
                if (!JavascriptOperators::IsUndefined(args[2]))
                {
                    value = JavascriptConversion::ToUInt16(args[2], scriptContext);
                }
                if (args.Info.Count > 3)
                {
                    isLittleEndian = JavascriptConversion::ToBoolean(args[3], scriptContext);
                }
            }
            break;
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        dataView->SetValue<uint16>(args[1], value, _u("DataView.prototype.SetUint16"), isLittleEndian);
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var DataView::EntrySetInt32(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        BOOL isLittleEndian = FALSE;
        int32 value = 0;
        switch (args.Info.Count)
        {
        case 0:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
            break;
        case 1:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
            break;
        default:
            if (!VarIs<DataView>(args[0]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
                break;
            }
            if (args.Info.Count > 2)
            {
                if (!JavascriptOperators::IsUndefined(args[2]))
                {
                    value = JavascriptConversion::ToInt32(args[2], scriptContext);
                }
                if (args.Info.Count > 3)
                {
                    isLittleEndian = JavascriptConversion::ToBoolean(args[3], scriptContext);
                }
            }
            break;
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        dataView->SetValue<int32>(args[1], value, _u("DataView.prototype.SetInt32"), isLittleEndian);
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var DataView::EntrySetUint32(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        BOOL isLittleEndian = FALSE;
        uint32 value = 0;
        switch (args.Info.Count)
        {
        case 0:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
            break;
        case 1:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
            break;
        default:
            if (!VarIs<DataView>(args[0]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
                break;
            }
            if (args.Info.Count > 2)
            {
                if (!JavascriptOperators::IsUndefined(args[2]))
                {
                    value = JavascriptConversion::ToUInt32(args[2], scriptContext);
                }
                if (args.Info.Count > 3)
                {
                    isLittleEndian = JavascriptConversion::ToBoolean(args[3], scriptContext);
                }
            }
            break;
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        dataView->SetValue<uint32>(args[1], value, _u("DataView.prototype.SetUint32"), isLittleEndian);
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var DataView::EntrySetFloat32(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        BOOL isLittleEndian = FALSE;
        float value = static_cast<float>(JavascriptNumber::NaN);
        switch (args.Info.Count)
        {
        case 0:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
            break;
        case 1:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
            break;
        default:
            if (!VarIs<DataView>(args[0]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
                break;
            }
            if (args.Info.Count > 2)
            {
                if (!JavascriptOperators::IsUndefined(args[2]))
                {
                    value = JavascriptConversion::ToFloat(args[2], scriptContext);
                }
                if (args.Info.Count > 3)
                {
                    isLittleEndian = JavascriptConversion::ToBoolean(args[3], scriptContext);
                }
            }
            break;
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        dataView->SetValue<float>(args[1], value, _u("DataView.prototype.SetFloat32"), isLittleEndian);
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var DataView::EntrySetFloat64(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        BOOL isLittleEndian = FALSE;
        double value = JavascriptNumber::NaN;
        switch (args.Info.Count)
        {
        case 0:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
            break;
        case 1:
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DataView_NeedArgument, _u("offset"));
            break;
        default:
            if (!VarIs<DataView>(args[0]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
                break;
            }
            if (args.Info.Count > 2)
            {
                if (!JavascriptOperators::IsUndefined(args[2]))
                {
                    value = JavascriptConversion::ToNumber(args[2], scriptContext);
                }
                if (args.Info.Count > 3)
                {
                    isLittleEndian = JavascriptConversion::ToBoolean(args[3], scriptContext);
                }
            }
            break;
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        dataView->SetValue<double>(args[1], value, _u("DataView.prototype.SetFloat64"), isLittleEndian);
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var DataView::EntryGetterBuffer(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<DataView>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        ArrayBufferBase* arrayBuffer = dataView->GetArrayBuffer();

        if (arrayBuffer == nullptr)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }
        return CrossSite::MarshalVar(scriptContext, arrayBuffer);
    }

    Var DataView::EntryGetterByteLength(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<DataView>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        ArrayBufferBase* arrayBuffer = dataView->GetArrayBuffer();

        if (arrayBuffer == nullptr)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }
        else if (arrayBuffer->IsDetached())
        {
            return TaggedInt::ToVarUnchecked(0);
        }

        return JavascriptNumber::ToVar(dataView->GetLength(), scriptContext);
    }

    Var DataView::EntryGetterByteOffset(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<DataView>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDataView);
        }

        DataView* dataView = VarTo<DataView>(args[0]);
        ArrayBufferBase* arrayBuffer = dataView->GetArrayBuffer();

        if (arrayBuffer == nullptr)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }
        else if (arrayBuffer->IsDetached())
        {
            return TaggedInt::ToVarUnchecked(0);
        }

        return JavascriptNumber::ToVar(dataView->GetByteOffset(), scriptContext);
    }

    BOOL DataView::SetItemWithAttributes(uint32 index, Var value, PropertyAttributes attributes)
    {
        AssertMsg(false, "We don't need a DataView to serve as object's internal array");
        return FALSE;
    }

    void DataView::ClearLengthAndBufferOnDetach()
    {
        AssertMsg(this->GetArrayBuffer()->IsDetached(), "Array buffer should be detached if we're calling this method");

        this->length = 0;
#if INT32VAR
        this->buffer = (BYTE*)TaggedInt::ToVarUnchecked(0);
#else
        this->buffer = nullptr;
#endif
    }

#ifdef _M_ARM
    // Provide template specialization (only) for memory access at unaligned float/double address which causes data alignment exception otherwise.
    template<>
    Var DataView::GetValueWithCheck<float>(Var offset, const char16 *funcName, BOOL isLittleEndian)
    {
        return this->GetValueWithCheck<float, float UNALIGNED*>(offset, isLittleEndian, funcName);
    }

    template<>
    Var DataView::GetValueWithCheck<double>(Var offset, const char16 *funcName, BOOL isLittleEndian)
    {
        return this->GetValueWithCheck<double, double UNALIGNED*>(offset, isLittleEndian, funcName);
    }

    template<>
    void DataView::SetValue<float>(Var offset, float value, const char16 *funcName, BOOL isLittleEndian)
    {
        this->SetValue<float, float UNALIGNED*>(offset, value, isLittleEndian, funcName);
    }

    template<>
    void DataView::SetValue<double>(Var offset, double value, const char16 *funcName, BOOL isLittleEndian)
    {
        this->SetValue<double, double UNALIGNED*>(offset, value, isLittleEndian, funcName);
    }
#endif

}
