//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------
    ///
    /// enum FlagTypes
    ///
    /// Different types of flags supported by the debug table
    ///
    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------


    enum FlagTypes
    {
        InvalidFlagType,
        FlagString,
        FlagPhases,
        FlagNumber,
        FlagBoolean,
        FlagNumberSet,
        FlagNumberPairSet,
        FlagNumberTrioSet,
        FlagNumberRange
    };

    // Shadow of enum SCRIPTHOSTTYPE defined in activscp.idl.
    enum HostType
    {
        HostTypeDefault = 0,        // Used to detect engines with uninitialized host type.
        HostTypeBrowser = 1,        // Currently this implies enabled legacy language features, use it for IE.
        HostTypeApplication = 2,    // Currently this implies legacy-free language features, use it for WWA.
        HostTypeWebview = 3,        // Webview in a WWA/XAML app with WinRT access.
        HostTypeMin = HostTypeBrowser,
        HostTypeMax = HostTypeWebview
    };

    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------
    ///
    /// enum Flag
    ///
    /// All the flags available. The list of flags are described in the file
    /// ConfigFlagsList.h
    ///
    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------


    enum Flag: unsigned short
    {
#define FLAG(type, name, ...) name##Flag,

#include "ConfigFlagsList.h"
        FlagCount,
        InvalidFlag,
        NoParentFlag,
        FlagMax = NoParentFlag
    } ;


    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------
    ///
    /// enum Phase
    ///
    /// Different Phases/Subphases of the backend. The list of phases is described
    /// in the file ConfigFlagsList.h
    ///
    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------


    enum Phase: unsigned short
    {
#define PHASE(name) name##Phase,
#include "ConfigFlagsList.h"
        PhaseCount,
        InvalidPhase
#undef PHASE
    };

    extern const char16* const FlagNames[FlagCount + 1];
    extern const char16* const PhaseNames[PhaseCount + 1];
    extern const Flag           FlagParents[FlagCount + 1];

    typedef     int             Number;
    typedef     bool            Boolean;

    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------
    ///
    /// class String
    ///
    /// A simple string wrapper for storing strings for the flags. It creates a
    /// copy of the string which it releases in the destructor. Simple operator
    /// fiddling to make this class as interoperable with strings as possible :-
    ///
    ///     1.  Single argument constructor takes care of initialization
    ///     2.  Assignment operator overloaded for char16*
    ///     3.  JavascriptConversion to const char16 * is defined. Making this constant ensures
    ///         that a non constant char16 * does not point to our buffer which can
    ///         potentially corrupt it.
    ///
    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------


    class String
    {
        PREVENT_COPYCONSTRUCT(String)


    // Data
    private:
        char16*           pszValue;

    // Construction
    public:
        inline String();
        inline String(__in_z_opt const char16* psz);
        inline ~String();


    // Methods
    public:

        ///----------------------------------------------------------------------------
        ///
        /// Assignment operator defined for easy access
        ///
        ///----------------------------------------------------------------------------

        String& operator=(__in_z_opt const char16* psz)
        {
            Set(psz);
            return *this;
        }

        String &operator =(const String &other)
        {
            return (*this = other.pszValue);
        }

        ///----------------------------------------------------------------------------
        ///
        /// Typecast operator defined so we can directly use it with strings
        ///
        ///----------------------------------------------------------------------------

        operator const char16* () const
        {
            return this->pszValue;
        }

    // Implementation
    private:
        void Set(__in_z_opt const char16* pszValue);
    };

    class NumberSet
    {
    public:
        NumberSet();
        void Add(uint32 x);
        bool Contains(uint32 x);
        bool Empty() const { return set.Count() == 0; }
    private:
        JsUtil::BaseHashSet<uint32, NoCheckHeapAllocator, PrimeSizePolicy> set;
    };

    class NumberPair
    {
    public:
        NumberPair(uint32 x, uint32 y) : x(x), y(y) {}
        NumberPair() : x((uint32)-1), y((uint32)-1) {}

        operator hash_t() const { return (x << 16) + y; }
        bool operator ==(const NumberPair &other) const { return x == other.x && y == other.y; }
    private:
        uint32 x;
        uint32 y;
    };

    class NumberPairSet
    {
    public:
        NumberPairSet();
        void Add(uint32 x, uint32 y);
        bool Contains(uint32 x, uint32 y);
        bool Empty() const { return set.Count() == 0; }
    private:
        JsUtil::BaseHashSet<NumberPair, NoCheckHeapAllocator, PrimeSizePolicy> set;
    };

    class NumberTrio
    {
    public:
        NumberTrio(uint32 x, uint32 y, uint32 z) : x(x), y(y), z(z) {}
        NumberTrio() : x((uint32)-1), y((uint32)-1) {}

        operator hash_t() const { return (x << 20) + (y << 10) + z; }
        bool operator ==(const NumberTrio &other) const { return x == other.x && y == other.y && z == other.z; }
    private:
        uint32 x;
        uint32 y;
        uint32 z;
    };

    class NumberTrioSet
    {
    public:
        NumberTrioSet();
        void Add(uint32 x, uint32 y, uint32 z);
        bool Contains(uint32 x, uint32 y, uint32 z);
        bool Empty() const { return set.Count() == 0; }
    private:
        JsUtil::BaseHashSet<NumberTrio, NoCheckHeapAllocator, PrimeSizePolicy> set;
    };

    struct SourceFunctionNode
    {
        uint sourceContextId;
        Js::LocalFunctionId functionId;
        SourceFunctionNode(uint sourceContextId, Js::LocalFunctionId functionId) : sourceContextId(sourceContextId), functionId(functionId) {}
        SourceFunctionNode() : sourceContextId(0), functionId(0){}
    };

    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------
    ///
    /// struct RangeUnit
    ///
    /// Used to store a pair of bounds
    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------

    template <typename TRangeUnitData>
    struct RangeUnit
    {
        TRangeUnitData i, j;
        RangeUnit() {}
        RangeUnit(TRangeUnitData x, TRangeUnitData y) :
            i(x),
            j(y)
        {}
    };

    template <typename TRangeUnitData>
    bool RangeUnitContains(RangeUnit<TRangeUnitData> unit, TRangeUnitData item)
    {
        return (item >= unit.i && item <= unit.j);
    }

    template <>
    bool RangeUnitContains<SourceFunctionNode>(RangeUnit<SourceFunctionNode> unit, SourceFunctionNode n);

    template <typename TRangeUnitData>
    RangeUnit<TRangeUnitData> GetFullRange()
    {
        RangeUnit<TRangeUnitData> unit;
        unit.i = INT_MIN;
        unit.j = INT_MAX;
        return unit;
    }

    template <>
    RangeUnit<SourceFunctionNode> GetFullRange();

    template <typename TRangeUnitData>
    TRangeUnitData GetPrevious(TRangeUnitData unit)
    {
        return unit - 1;
    }

    template <>
    SourceFunctionNode GetPrevious(SourceFunctionNode unit);

    template <typename TRangeUnitData>
    TRangeUnitData GetNext(TRangeUnitData unit)
    {
        return unit + 1;
    }

    template <>
    SourceFunctionNode GetNext(SourceFunctionNode unit);

    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------
    ///
    /// class Range
    ///
    /// Data structure to store a range of integers. (Can trivially be templatized
    /// but resisting the temptation for the sake of simplicity.)
    ///
    /// Sample:     1,3-11,15,20
    ///
    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------

    template <typename TRangeUnitData>
    class RangeBase
    {
    // Nested Classes
    public:
        typedef RangeUnit<TRangeUnitData> Unit;


    // Data
    protected:
        SList<Unit, NoCheckHeapAllocator>      range;

    // Construction
    public:
        RangeBase() : range(&NoCheckHeapAllocator::Instance) {}

    // Methods
    public:
        inline  bool            InRange(TRangeUnitData i);
        inline  bool            ContainsAll();
        inline  void            Add(TRangeUnitData i, RangeBase<TRangeUnitData>* oppositeRange = nullptr);
        inline  void            Add(TRangeUnitData i, TRangeUnitData j, RangeBase<TRangeUnitData>* oppositeRange = nullptr);
        inline  void            Clear();

#if DBG_RANGE
        template <typename TFunction>
        void Map(TFunction fn) const
        {
            range.Map(fn);
        }
#endif
    };

    class Range : public RangeBase<SourceFunctionNode> {};
    class NumberRange : public RangeBase<int> {};

    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------
    ///
    /// class Phases
    ///
    /// Maintains the list of all the phases. Each individual phase can be enabled
    /// and an optional range of numbers can be added to it
    ///
    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------


    class Phases
    {
    // Nested Types
    private:

        ///----------------------------------------------------------------------------
        ///----------------------------------------------------------------------------
        ///
        /// struct UnitPhase
        ///
        /// Stores information about a particular phase. Current information is :
        ///     1.  Is the phase enabled/disabled?
        ///     2.  A range of numbers for the given phase. This range can represent a
        ///         variety of information. For example, this range can be the function
        ///         numbers of all the functions where a particular optimization is
        ///         enabled or where opcode dump is enabled.
        ///
        ///
        ///  Its a tiny private struct used by Phases for simply storing data. So
        ///  making this a struct with all members public.
        ///
        ///----------------------------------------------------------------------------
        ///----------------------------------------------------------------------------

        struct UnitPhase
        {

        // Construction
        public:
            UnitPhase() : valid(false) { }

        // Data
        public:
                bool            valid;
                Range           range;

        };

    // Data
    private:
                UnitPhase       phaseList[PhaseCount];

    // Constructors
    public:
        Phases() {}

    // Methods
    public:

        void            Enable(Phase phase);
        void            Disable(Phase phase);
        bool            IsEnabled(Phase phase);
        bool            IsEnabled(Phase phase, uint sourceContextId, Js::LocalFunctionId functionId);
        bool            IsEnabledForAll(Phase phase);
        Range *         GetRange(Phase phase);
        Phase           GetFirstPhase();
    };

    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------
    ///
    /// class ConfigFlagsTable
    ///
    /// Maintains the list of all the flags. Individual flags can be accessed by :-
    ///
    ///     1.  flagTable->verbose;     // here verbose is the name of the flag
    ///                                 // It will be strongly typed with the
    ///                                 // type of the flag is as described in
    ///                                 // ConfigFlagsTable.h
    ///
    ///     2.  flagTable->GetAsBoolean(verboseFlag);
    ///                                 // this access is weekly typed. Even if the
    ///                                 // flag is not of type Boolean, it will be
    ///                                 // reinterpret_cast to Boolean *
    ///
    /// The former access should be used in the code for reading the flag values
    /// while the later is useful for adding values into the table.
    ///
    ///----------------------------------------------------------------------------
    ///----------------------------------------------------------------------------


    class ConfigFlagsTable
    {

    // Constructor
    public:
        ConfigFlagsTable();

    // Methods
    public:

        static  Flag            GetFlag(__in LPCWSTR str);
        static  Phase           GetPhase(__in LPCWSTR str);
        static  void            PrintUsageString();

        static  FlagTypes       GetFlagType(Flag flag);

                String*         GetAsString(Flag flag) const;
                Phases*         GetAsPhase(Flag flag) const;
                Flag            GetOppositePhaseFlag(Flag flag) const;
                Boolean*        GetAsBoolean(Flag flag) const;
                Number*         GetAsNumber(Flag flag) const;
                NumberSet*      GetAsNumberSet(Flag flag) const;
                NumberPairSet * GetAsNumberPairSet(Flag flag) const;
                NumberTrioSet * GetAsNumberTrioSet(Flag flag) const;
                NumberRange *   GetAsNumberRange(Flag flag) const;

                void            SetAsBoolean(Flag flag, Boolean value);

                Boolean         GetDefaultValueAsBoolean(Flag flag) const;

        // indicates whether a flag is a parent flag
                bool            IsParentFlag(Flag flag) const;

        // get the parent flag of a given flag, if any, otherwise returns InvalidFlag.
                Flag            GetParentFlag(Flag flag) const;

        // get the next child flag of a given parent flag, if any, otherwise returns InvalidFlag. Pass InvalidFlag as currentChildFlag if no current child flag to iterate from.
                Flag            GetNextChildFlag(Flag parentFlag, Flag currentChildFlag) const;

                void            Enable(Flag flag);
                bool            IsEnabled(Flag flag);
                void            Disable(Flag flag);

                void             VerboseDump();

    // Data
    public:

        ///----------------------------------------------------------------------------
        ///
        /// Declaration of each flag as a member variable of the corresponding type.
        /// These variables are made public because get/set are too ugly for simple
        /// flags. Besides there are flags like phase lists which are not simple and
        /// its better to have a uniform way to access them.
        ///
        ///     if(ConfigFlag->dump)     is much better than
        ///     if(ConfigFlag->GetDump())
        ///
        ///----------------------------------------------------------------------------

        #define FLAG(type, name, ...) \
            \
            type name;                      \

        #include "ConfigFlagsList.h"

                bool            flagPresent[FlagCount];

#if defined(ENABLE_DEBUG_CONFIG_OPTIONS) && CONFIG_PARSE_CONFIG_FILE
                // save the jscript.config for easier to get the raw input while analyzing dump file
                char16          rawInputFromConfigFile[512];
                int             rawInputFromConfigFileIndex;
#endif

    private:
        //
        // This variable is defined for the constructor
        //
                int             nDummy;

                // indicates whether a flag is a parent flag
                bool            flagIsParent[FlagMax + 1];

    // Implementation
    private:
                void *      GetProperty(Flag flag) const;

                void        SetAllParentFlagsAsDefaultValue();

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
                // special callback logic
                void        FlagSetCallback_ES6All(Js::Boolean value);
                void        FlagSetCallback_ES6Experimental(Js::Boolean value);
#endif

    public:
        void FinalizeConfiguration();
        void EnableExperimentalFlag();

        // Some config flags are expected to be constant during the lifetime of
        // a ScriptContext object. However, some other flags can change,
        // possibly multiple times.
        //
        // To keep the flags constant during the lifetime of a ScriptContext
        // object, we copy the ones that it needs into ThreadContext and have
        // it fetch them from ThreadContext instead of here. Given that a
        // ScriptContext object is bound to a ThreadContext object and never
        // gets reassigned, this keeps the flags constant while the
        // ScriptContext object is alive.
        //
        // Currently, among the flags used by ScriptContext, only the
        // experimental flags are altered after initialization. Therefore, only
        // access to these flags are serialized.
        //
        // Note that this lock is acquired automatically only when the
        // experimental flags are initialized via the EnableExperimentalFlag()
        // method. It should be manually acquired anywhere else where these
        // flags are accessed.
        CriticalSection csExperimentalFlags;

    private:
        void TransferAcronymFlagConfiguration();
        void TranslateFlagConfiguration();
    };


    class Profiler;

    class Configuration
    {
    public:
        ConfigFlagsTable           flags;
        static Configuration        Global;
        bool EnableJitInDebugMode();

        // Public in case the client wants to have
        // a separate config from the global one
        Configuration();
    };

//Create macros for a useful subset of the config options that either get the value from the configuration (if the option is enabled) or
//just use the hard coded default value (if not). All the ...IsEnabled(...) default to false.
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
#define CONFIG_ISENABLED(flag)      (Js::Configuration::Global.flags.IsEnabled(flag))
#define CUSTOM_CONFIG_ISENABLED(flags, flag)      (flags.IsEnabled(flag))
#define CONFIG_FLAG(flag)           (Js::Configuration::Global.flags.##flag)
#define CUSTOM_CONFIG_FLAG(flags, flag) (flags.##flag)
#define CONFIG_FLAG_RELEASE(flag)   CONFIG_FLAG(flag)
#define CONFIG_FLAG_CONTAINS(flag, func)  (Js::Configuration::Global.flags.##flag.Contains((func)->GetLocalFunctionId()))

#define PHASE_OFF_PROFILED_BYTE_CODE(phase, func) Js::Configuration::Global.flags.OffProfiledByteCode.IsEnabled((phase),(func)->GetSourceContextId(),(func)->GetLocalFunctionId())
#define PHASE_OFF_PROFILED_BYTE_CODE_ALL(phase) Js::Configuration::Global.flags.OffProfiledByteCode.IsEnabledForAll((phase))
#define PHASE_OFF_PROFILED_BYTE_CODE_OPTFUNC(phase, func) ((func) ? PHASE_OFF_PROFILED_BYTE_CODE(phase, func) : PHASE_OFF_PROFILED_BYTE_CODE_ALL(phase))

#define PHASE_OFF1(phase)           Js::Configuration::Global.flags.Off.IsEnabled((phase))
#define CUSTOM_PHASE_OFF1(flags, phase)           flags.Off.IsEnabled((phase))
#define PHASE_OFF_ALL(phase)        Js::Configuration::Global.flags.Off.IsEnabledForAll((phase))
#define PHASE_OFF(phase, func)      PHASE_OFF_RAW((phase), (func)->GetSourceContextId(), (func)->GetLocalFunctionId())
#define PHASE_OFF_OPTFUNC(phase, func) ((func) ? PHASE_OFF(phase, func) : PHASE_OFF_ALL(phase))
#define PHASE_OFF_RAW(phase, sourceId, functionId) \
                                    Js::Configuration::Global.flags.Off.IsEnabled((phase), (sourceId), (functionId))

#define PHASE_ON1(phase)            Js::Configuration::Global.flags.On.IsEnabled((phase))
#define CUSTOM_PHASE_ON1(flags, phase) flags.On.IsEnabled((phase))
#define PHASE_ON(phase, func)       PHASE_ON_RAW((phase), (func)->GetSourceContextId(), (func)->GetLocalFunctionId())
#define PHASE_ON_RAW(phase, sourceId, functionId) \
                                    Js::Configuration::Global.flags.On.IsEnabled((phase), (sourceId), (functionId))

#define PHASE_FORCE1(phase)         Js::Configuration::Global.flags.Force.IsEnabled((phase))
#define CUSTOM_PHASE_FORCE1(flags, phase) flags.Force.IsEnabled((phase))
#define PHASE_FORCE(phase, func)    PHASE_FORCE_RAW((phase), (func)->GetSourceContextId(), (func)->GetLocalFunctionId())
#define PHASE_FORCE_OPTFUNC(phase, func) ((func) ? PHASE_FORCE(phase, func) : PHASE_FORCE1(phase))
#define PHASE_FORCE_RAW(phase, sourceId, functionId) \
                                    Js::Configuration::Global.flags.Force.IsEnabled((phase), (sourceId), (functionId))

#define PHASE_STRESS1(phase)        Js::Configuration::Global.flags.Stress.IsEnabled((phase))
#define PHASE_STRESS(phase, func)   PHASE_STRESS_RAW((phase), (func)->GetSourceContextId(), (func)->GetLocalFunctionId())
#define PHASE_STRESS_RAW(phase, sourceId, functionId) \
                                    Js::Configuration::Global.flags.Stress.IsEnabled((phase), (sourceId), (functionId))

#define PHASE_TRACE1(phase)         Js::Configuration::Global.flags.Trace.IsEnabled((phase))
#define CUSTOM_PHASE_TRACE1(flags, phase) flags.Trace.IsEnabled((phase))
#define PHASE_TRACE(phase, func)    PHASE_TRACE_RAW((phase), (func)->GetSourceContextId(), (func)->GetLocalFunctionId())
#define PHASE_TRACE_RAW(phase, sourceId, functionId) \
                                    Js::Configuration::Global.flags.Trace.IsEnabled((phase), (sourceId), (functionId))

#if DBG
    // Enabling the ability to trace the StringConcat phase only in debug builds due to performance impact
    #define PHASE_TRACE_StringConcat PHASE_TRACE1(Js::StringConcatPhase)
#else
    #define PHASE_TRACE_StringConcat (false)
#endif

#define PHASE_VERBOSE_TRACE1(phase) \
    ((PHASE_TRACE1((phase))) && Js::Configuration::Global.flags.Verbose)

#define CUSTOM_PHASE_VERBOSE_TRACE1(flags, phase) \
    ((CUSTOM_PHASE_TRACE1((flags), (phase))) && flags.Verbose)

#define PHASE_VERBOSE_TRACE(phase, func) \
    ((PHASE_TRACE((phase), (func))) && Js::Configuration::Global.flags.Verbose)
#define PHASE_VERBOSE_TRACE_RAW(phase, sourceId, functionId) \
    ((PHASE_TRACE_RAW((phase), (sourceId), (functionId))) && Js::Configuration::Global.flags.Verbose)

#define PHASE_DUMP1(phase)          Js::Configuration::Global.flags.Dump.IsEnabled((phase))
#define PHASE_DUMP(phase, func)     Js::Configuration::Global.flags.Dump.IsEnabled((phase), (func)->GetSourceContextId(),(func)->GetLocalFunctionId())

#define PHASE_DEBUGBREAK_ON_PHASE_BEGIN(phase, func) Js::Configuration::Global.flags.DebugBreakOnPhaseBegin.IsEnabled((phase), (func)->GetSourceContextId(), (func)->GetLocalFunctionId())

#define PHASE_STATS1(phase)         Js::Configuration::Global.flags.Stats.IsEnabled((phase))
#define CUSTOM_PHASE_STATS1(flags, phase) flags.Stats.IsEnabled((phase))
#define PHASE_VERBOSE_STATS1(phase) \
    ((PHASE_STATS1(phase)) && Js::Configuration::Global.flags.Verbose)

#define PHASE_STATS_ALL(phase)      Js::Configuration::Global.flags.Stats.IsEnabledForAll((phase))
#define PHASE_STATS(phase, func)    PHASE_STATS_RAW((phase), (func)->GetSourceContextId(), (func)->GetLocalFunctionId())
#define PHASE_STATS_RAW(phase, sourceId, functionId) \
                                    Js::Configuration::Global.flags.Stats.IsEnabled((phase), (sourceId), (functionId))

#define PHASE_VERBOSE_STATS(phase, func) \
    ((PHASE_STATS(phase, func)) && Js::Configuration::Global.flags.Verbose)
#define PHASE_VERBOSE_STATS_RAW(phase, sourceId, functionId) \
    ((PHASE_STATS_RAW(phase, sourceId, functionId)) && Js::Configuration::Global.flags.Verbose)

#define PHASE_TESTTRACE1(phase) Js::Configuration::Global.flags.TestTrace.IsEnabled((phase))
#define PHASE_TESTTRACE(phase, func) PHASE_TESTTRACE_RAW((phase), (func)->GetSourceContextId(), (func)->GetLocalFunctionId())
#define PHASE_TESTTRACE_RAW(phase, sourceId, functionId) \
                                    Js::Configuration::Global.flags.TestTrace.IsEnabled((phase), (sourceId), (functionId))
#ifdef ENABLE_BASIC_TELEMETRY
#define PHASE_TESTTRACE1_TELEMETRY(phase) PHASE_TESTTRACE1((phase))
#else
#define PHASE_TESTTRACE1_TELEMETRY(phase) (false)
#endif

#define PHASE_PRINT_TRACE1(phase, ...) \
    if (PHASE_TRACE1(phase)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define CUSTOM_PHASE_PRINT_TRACE1(flags, phase, ...) \
    if (CUSTOM_PHASE_TRACE1(flags, phase)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define PHASE_PRINT_TRACE(phase, func, ...) \
    if (PHASE_TRACE(phase, func)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define PHASE_PRINT_TRACE_RAW(phase, sourceId, functionId, ...) \
    if (PHASE_TRACE_RAW(phase, sourceId, functionId)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define PHASE_PRINT_VERBOSE_TRACE1(phase, ...) \
    if (PHASE_VERBOSE_TRACE1(phase)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define CUSTOM_PHASE_PRINT_VERBOSE_TRACE1(flags, phase, ...) \
    if (CUSTOM_PHASE_VERBOSE_TRACE1(flags, phase)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define PHASE_PRINT_VERBOSE_TRACE(phase, func, ...) \
    if (PHASE_VERBOSE_TRACE(phase, func)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define PHASE_PRINT_VERBOSE_TRACE_RAW(phase, sourceId, functionId, ...) \
    if (PHASE_VERBOSE_TRACE_RAW(phase, sourceId, functionId)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define PHASE_VERBOSE_TESTTRACE1(phase) (PHASE_TESTTRACE1(phase) && Js::Configuration::Global.flags.Verbose)
#define PHASE_VERBOSE_TESTTRACE(phase, func) \
    (PHASE_TESTTRACE(phase, func) && Js::Configuration::Global.flags.Verbose)
#define PHASE_VERBOSE_TESTTRACE_RAW(phase, sourceId, functionId) \
    (PHASE_TESTTRACE_RAW(phase, sourceId, functionId) && Js::Configuration::Global.flags.Verbose)

#define PHASE_PRINT_TESTTRACE1(phase, ...) \
    if (PHASE_TESTTRACE1(phase)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define PHASE_PRINT_TESTTRACE(phase, func, ...) \
    if (PHASE_TESTTRACE(phase, func)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define PHASE_PRINT_TESTTRACE_RAW(phase, sourceId, functionId, ...) \
    if (PHASE_TESTTRACE_RAW(phase, sourceId, functionId)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define PHASE_PRINT_VERBOSE_TESTTRACE1(phase, ...) \
    if (PHASE_VERBOSE_TESTTRACE1(phase)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define PHASE_PRINT_VERBOSE_TESTTRACE(phase, func, ...) \
    if (PHASE_VERBOSE_TESTTRACE(phase, func)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#define PHASE_PRINT_VERBOSE_TESTTRACE_RAW(phase, sourceId, functionId, ...) \
    if (PHASE_VERBOSE_TESTTRACE_RAW(phase, sourceId, functionId)) \
    { \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }
#else
#define CONFIG_ISENABLED(flag)      (false)             //All flags.IsEnabled(foo) are false by default.
#define CUSTOM_CONFIG_ISENABLED(flags, flag)      (false)             //All flags.IsEnabled(foo) are false by default.
#define CONFIG_FLAG(flag)           (DEFAULT_CONFIG_##flag)
#define CUSTOM_CONFIG_FLAG(flags, flag)           (DEFAULT_CONFIG_##flag)
#define CONFIG_FLAG_RELEASE(flag)   (Js::Configuration::Global.flags.##flag)
#define CONFIG_FLAG_CONTAINS(flag, func)  (false)
#define PHASE_OFF_PROFILED_BYTE_CODE(phase, func) (false)
#define PHASE_OFF_PROFILED_BYTE_CODE_ALL(phase) (false)
#define PHASE_OFF_PROFILED_BYTE_CODE_OPTFUNC(phase, func) (false)

#define PHASE_OFF1(phase)           (false)             //All flags.Off.IsEnabled(foo) are false by default
#define CUSTOM_PHASE_OFF1(flags, phase)           (false)             //All flags.Off.IsEnabled(foo) are false by default
#define PHASE_OFF_ALL(phase)        (false)
#define PHASE_OFF(phase, func)      (false)
#define PHASE_OFF_RAW(phase, sourceId, functionId) (false)
#define PHASE_OFF_OPTFUNC(phase, func) (false)

#define PHASE_ON1(phase)            (false)
#define CUSTOM_PHASE_ON1(flags, phase) (false)
#define PHASE_ON(phase, func)       (false)
#define PHASE_ON_RAW(phase, sourceId, functionId) (false)

#define PHASE_FORCE1(phase)         (false)
#define CUSTOM_PHASE_FORCE1(flags, phase)         (false)
#define PHASE_FORCE(phase, func)    (false)
#define PHASE_FORCE_RAW(phase, sourceId, functionId) (false)
#define PHASE_FORCE_OPTFUNC(phase, func) (false)

#define PHASE_STRESS1(phase)         (false)
#define PHASE_STRESS(phase, func)    (false)
#define PHASE_STRESS_RAW(phase, sourceId, functionId) (false)

#define PHASE_TRACE1(phase)         (false)
#define CUSTOM_PHASE_TRACE1(phase)         (false)
#define PHASE_TRACE(phase, func)    (false)
#define PHASE_TRACE_RAW(phase, sourceId, functionId) (false)
#define PHASE_TRACE_StringConcat    (false)

#define PHASE_VERBOSE_TRACE1(phase) (false)
#define CUSTOM_PHASE_VERBOSE_TRACE1(flags, phase) (false)
#define PHASE_VERBOSE_TRACE(phase, func) (false)
#define PHASE_VERBOSE_TRACE_RAW(phase, sourceId, functionId) (false)

#define PHASE_TESTTRACE1(phase)     (false)
#define PHASE_TESTTRACE(phase, func) (false)
#define PHASE_TESTTRACE_RAW(phase, sourceId, functionId) (false)
#define PHASE_TESTTRACE1_TELEMETRY(phase) (false)

#define PHASE_PRINT_TRACE1(phase, ...)
#define CUSTOM_PHASE_PRINT_TRACE1(phase, ...)
#define PHASE_PRINT_TRACE(phase, func, ...)
#define PHASE_PRINT_TRACE_RAW(phase, sourceId, functionId, ...)

#define PHASE_PRINT_VERBOSE_TRACE1(phase, ...)
#define CUSTOM_PHASE_PRINT_VERBOSE_TRACE1(phase, ...)
#define PHASE_PRINT_VERBOSE_TRACE(phase, func, ...)
#define PHASE_PRINT_VERBOSE_TRACE_RAW(phase, sourceId, functionId, ...)

#define PHASE_VERBOSE_TESTTRACE1(phase) (false)
#define PHASE_VERBOSE_TESTTRACE(phase, func) (false)
#define PHASE_VERBOSE_TESTTRACE_RAW(phase, sourceId, functionId) (false)

#define PHASE_VERBOSE_TRACE1(phase) (false)
#define PHASE_VERBOSE_TRACE(phase, func) (false)
#define PHASE_VERBOSE_TRACE_RAW(phase, sourceId, functionId) (false)

#define PHASE_PRINT_TESTTRACE1(phase, ...)
#define PHASE_PRINT_TESTTRACE(phase, func, ...)
#define PHASE_PRINT_TESTTRACE_RAW(phase, sourceId, functionId, ...)

#define PHASE_PRINT_VERBOSE_TESTTRACE1(phase, ...)
#define PHASE_PRINT_VERBOSE_TESTTRACE(phase, func, ...)
#define PHASE_PRINT_VERBOSE_TESTTRACE_RAW(phase, sourceId, functionId, ...)

#define PHASE_DUMP(phase, func)     (false)             //All flags.Dump.IsEnabled(foo) are false by default

#define PHASE_STATS1(phase)         (false)
#define CUSTOM_PHASE_STATS1(flags, phase)         (false)
#define PHASE_VERBOSE_STATS1(phase) (false)
#define PHASE_STATS_ALL(phase)      (false)
#define PHASE_STATS(phase, func)    (false)
#define PHASE_STATS_RAW(phase, sourceId, functionId) (false)
#define PHASE_VERBOSE_STATS(phase, func) (false)
#define PHASE_VERBOSE_STATS_RAW(phase, sourceId, functionId) (false)
#endif

#ifdef ENABLE_REGEX_CONFIG_OPTIONS
#define REGEX_CONFIG_FLAG(flag) (Js::Configuration::Global.flags.##flag)
#else
#define REGEX_CONFIG_FLAG(flag) (DEFAULT_CONFIG_##flag)
#endif

#ifdef SUPPORT_INTRUSIVE_TESTTRACES
#define PHASE_PRINT_INTRUSIVE_TESTTRACE1(phase, ...) \
    PHASE_PRINT_TESTTRACE1(phase, __VA_ARGS__)
#else
#define PHASE_PRINT_INTRUSIVE_TESTTRACE1(phase, ...) (false)
#endif

#define PHASE_ENABLED1(phase)       (Js::PhaseIsEnabled::phase())
#define PHASE_ENABLED(phase, func)  (Js::PhaseIsEnabled::phase(func))
#define PHASE_ENABLED_RAW(phase, sourceId, functionId) \
                                    (Js::PhaseIsEnabled::phase(sourceId, functionId))
struct PhaseIsEnabled
{
#define PHASE_DEFAULT_ON(name) \
        static bool name##Phase() { return !PHASE_OFF1(Js::name##Phase); } \
        template<typename FUNC> static bool name##Phase(FUNC func) { return !PHASE_OFF(Js::name##Phase, func); } \
        static bool name##Phase(uint sourceId, Js::LocalFunctionId functionId) { return !PHASE_OFF_RAW(Js::name##Phase, sourceId, functionId); }
#define PHASE_DEFAULT_OFF(name) \
        static bool name##Phase() { return PHASE_ON1(Js::name##Phase); } \
        template<typename FUNC> static bool name##Phase(FUNC func) { return PHASE_ON(Js::name##Phase, func); } \
        static bool name##Phase(uint sourceId, Js::LocalFunctionId functionId) { return PHASE_ON_RAW(Js::name##Phase, sourceId, functionId); }
#include "ConfigFlagsList.h"
    };

///----------------------------------------------------------------------------
///----------------------------------------------------------------------------
///
/// class RangeBase
///
///----------------------------------------------------------------------------
///----------------------------------------------------------------------------

template <typename TRangeUnitData>
void
RangeBase<TRangeUnitData>::Add(TRangeUnitData i, RangeBase<TRangeUnitData>* oppositeRange)
{
    Add(i, i, oppositeRange);
}

template <typename TRangeUnitData>
void
RangeBase<TRangeUnitData>::Add(TRangeUnitData i, TRangeUnitData j, RangeBase<TRangeUnitData>* oppositeRange)
{
    RangeUnit<TRangeUnitData> a(i, j);
    range.Prepend(a);
    if (oppositeRange)
    {
        if (oppositeRange->range.Empty())
        {
            oppositeRange->range.Prepend(GetFullRange<TRangeUnitData>());
        }
        // Do an intersection
        auto it = oppositeRange->range.GetEditingIterator();
        while (it.Next())
        {
            Unit& unit = it.Data();
            bool c1 = RangeUnitContains(unit, i);
            bool c2 = RangeUnitContains(unit, j);
            bool c3 = RangeUnitContains(a, unit.i);
            bool c4 = RangeUnitContains(a, unit.j);
            enum IntersectionCase
            {
                NoIntersection = 0,
                AddedFullyContained = 3, // [ u [a] ]
                IntersectionAdded_Unit = 6, // [ a [ ] u ]
                CommonLowBound_Unit = 7, // [[ a ] u ]
                IntersectionUnit_Added = 9, // [ u [ ] a ]
                CommonTopBound_Unit = 11, // [ u [ a ]]
                UnitFullyContained = 12, // [ a [u] ]
                CommonLowBound_Added = 13, // [[ u ] a ]
                CommonTopBound_Added = 14, // [ a [ u ]]
                FullIntersection = 15 // [[a, u]]
            };
            IntersectionCase intersectionCase = (IntersectionCase)((int)c1 | (c2 << 1) | (c3 << 2) | (c4 << 3));
            switch (intersectionCase)
            {
            case NoIntersection:
                // Nothing to do
                break;
            case AddedFullyContained:
            {
                // Need to break the current in 2
                Unit lowUnit = unit;
                Unit topUnit = unit;
                lowUnit.j = GetPrevious(a.i);
                topUnit.i = GetNext(a.j);
                it.InsertBefore(lowUnit);
                it.InsertBefore(topUnit);
                it.RemoveCurrent();
                break;
            }
            case IntersectionAdded_Unit:
            case CommonLowBound_Unit:
                // Move the unit lower bound after the added upper bound
                unit.i = GetNext(a.j);
                break;
            case IntersectionUnit_Added:
            case CommonTopBound_Unit:
                // Move the unit upper bound before the added lower bound
                unit.j = GetPrevious(a.i);
                break;
            case CommonTopBound_Added:
            case CommonLowBound_Added:
            case UnitFullyContained:
            case FullIntersection:
                // Remove the unit
                it.RemoveCurrent();
                break;
            default:
                Assert(UNREACHED);
                break;
            }
        }
    }
}

template <typename TRangeUnitData>
bool
RangeBase<TRangeUnitData>::ContainsAll()
{
    return range.Empty();
}

template <typename TRangeUnitData>
void
Js::RangeBase<TRangeUnitData>::Clear()
{
    range.Clear();
}

///----------------------------------------------------------------------------
///
/// RangeBase::InRange
///
/// Searches for each element in the list of UnitRanges. If the given integer
/// is between the 2 values, then return true; If no element is present in range
/// then, then we return true
///
///----------------------------------------------------------------------------

template <typename TRangeUnitData>
bool RangeBase<TRangeUnitData>::InRange(TRangeUnitData n)
{
    if (range.Empty())
    {
        return true;
    }
    else
    {
        return range.MapUntil([n](RangeUnit<TRangeUnitData> const& unit)
        {
            return RangeUnitContains(unit, n);
        });
    }
}
}  // namespace Js

