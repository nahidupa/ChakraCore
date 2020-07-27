//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"
#include "Common.h"
#include <process.h>

namespace PlatformAgnostic
{
    Thread::ThreadHandle Thread::Create(unsigned stack_size,
                                        unsigned ( __stdcall *start_address )( void * ),
                                        void* arg_list,
                                        ThreadInitFlag init_flag,
                                        const char16* description)
    {
        unsigned int flag = 0;

        switch (init_flag)
        {
        case ThreadInitRunImmediately:
            flag = 0;
            break;
        case ThreadInitCreateSuspended:
            flag = CREATE_SUSPENDED;
            break;
        case ThreadInitStackSizeParamIsAReservation:
            flag = STACK_SIZE_PARAM_IS_A_RESERVATION;
            break;
        default:
            Assert(false);
        }

        uintptr_t handle = _beginthreadex(nullptr, stack_size, start_address, arg_list, flag, nullptr);

        if (handle == 0)
        {
            return InvalidHandle;
        }

        if (description != nullptr)
        {
            Kernel32Library::Instance->SetThreadDescription((HANDLE) handle, description);
        }

        return handle;
    }
} // namespace PlatformAgnostic
