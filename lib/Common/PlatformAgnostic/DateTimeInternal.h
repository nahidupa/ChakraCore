//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "Core/CommonTypedefs.h"

namespace PlatformAgnostic
{
namespace DateTime
{
    struct YMD;

    #define DateTimeTicks_PerSecond 1000.0
    #define DateTimeTicks_PerMinute (60 * DateTimeTicks_PerSecond)
    #define DateTimeTicks_PerHour   (60 * DateTimeTicks_PerMinute)
    #define DateTimeTicks_PerDay (DateTimeTicks_PerHour * 24)
    #define DateTimeTicks_PerLargestTZOffset (DateTimeTicks_PerDay + 1)
    #define DateTimeTicks_PerNonLeapYear (DateTimeTicks_PerDay * 365)
    #define DateTimeTicks_PerSafeEndOfYear (DateTimeTicks_PerNonLeapYear - DateTimeTicks_PerLargestTZOffset)

#ifdef _WIN32
    class TimeZoneInfo // DateTime.cpp
    {
    public:
        double daylightDate;
        double standardDate;
        double january1;
        double nextJanuary1;

        int32 daylightBias;
        int32 standardBias;
        int32 bias;
        uint32 lastUpdateTickCount;

        bool isDaylightTimeApplicable;
        bool isJanuary1Critical;

        TimeZoneInfo();
        bool IsValid(const double time);
        void Update(const double time);
    };

    class DaylightTimeHelperPlatformData // DaylightHelper.cpp
    {
    public:
        TimeZoneInfo cache1, cache2;
        bool useFirstCache;

        DaylightTimeHelperPlatformData() :
            useFirstCache(true)
        {
        }
    };

    class UtilityPlatformData
    {
    public:
        TIME_ZONE_INFORMATION timeZoneInfo;
        uint32 lastTimeZoneUpdateTickCount;

        void UpdateTimeZoneInfo();
        UtilityPlatformData() : lastTimeZoneUpdateTickCount(0) { GetTimeZoneInformation(&timeZoneInfo); }
    };

    class HiresTimerPlatformData
    {
    public:
        double dBaseTime;
        double dLastTime;
        double dAdjustFactor;
        uint64 baseMsCount;
        uint64 freq;

        bool fReset;
        bool fInit;
        bool fHiResAvailable;

        HiresTimerPlatformData() :
            dBaseTime(0),
            dLastTime(0),
            dAdjustFactor(1),
            baseMsCount(0),
            freq(0),
            fReset(true),
            fInit(false),
            fHiResAvailable(true)
        {
        }

        void Reset() { fReset = true; }
    };

#else // ! _WIN32

    typedef void* DaylightTimeHelperPlatformData;

    #define __CC_PA_TIMEZONE_ABVR_NAME_LENGTH 32
    struct UtilityPlatformData
    {
        // cache always the last date's zone
        WCHAR standardName[__CC_PA_TIMEZONE_ABVR_NAME_LENGTH];
        size_t standardNameLength;
    };

    class HiresTimerPlatformData
    {
    public:
        double    cacheSysTime;
        ULONGLONG cacheTick;
        ULONGLONG previousDifference;

        HiresTimerPlatformData():cacheSysTime(0), cacheTick(-1), previousDifference(0) { }
        void Reset() { /* dummy method for interface compatiblity */ }
    };

#endif // ! _WIN32

} // namespace DateTime
} // namespace PlatformAgnostic
