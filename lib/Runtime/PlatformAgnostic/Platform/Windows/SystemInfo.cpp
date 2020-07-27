//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"
#include "Common.h"
#include "ChakraPlatform.h"

namespace PlatformAgnostic
{
    bool SystemInfo::GetMaxVirtualMemory(size_t *totalAS)
    {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        *totalAS = (size_t) info.lpMaximumApplicationAddress;
        return true;
    }

}
