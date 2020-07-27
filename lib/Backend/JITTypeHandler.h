//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITTypeHandler
{
public:
    JITTypeHandler(TypeHandlerIDL * data);

    bool IsObjectHeaderInlinedTypeHandler() const;
    bool IsLocked() const;
    bool IsPrototype() const;

    uint16 GetInlineSlotCapacity() const;
    uint16 GetOffsetOfInlineSlots() const;

    int GetSlotCapacity() const;

    static bool IsTypeHandlerCompatibleForObjectHeaderInlining(const JITTypeHandler * oldTypeHandler, const JITTypeHandler * newTypeHandler);
    static bool NeedSlotAdjustment(const JITTypeHandler * oldTypeHandler, const JITTypeHandler * newTypeHandler, int *oldCount, int *newCount, Js::PropertyIndex *oldInlineSlotCapacity, Js::PropertyIndex *newInlineSlotCapacity);
private:
    TypeHandlerIDL m_data;
};
