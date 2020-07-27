//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonCorePch.h"

#ifndef USING_PAL_STDLIB
#include <io.h>
#include <share.h>
#include <fcntl.h>
#include <strsafe.h>
#endif

#include "Memory/MemoryLogger.h"
#include "Memory/ForcedMemoryConstraints.h"
#include "Core/ICustomConfigFlags.h"
#include "Core/CmdParser.h"
#include "Core/ConfigParser.h"

ConfigParser ConfigParser::s_moduleConfigParser(Js::Configuration::Global.flags);

#ifdef ENABLE_TRACE
class ArenaHost
{
    AllocationPolicyManager m_allocationPolicyManager;
    PageAllocator m_pageAllocator;
    ArenaAllocator m_allocator;

public:
    ArenaHost(__in_z const char16* arenaName) :
        m_allocationPolicyManager(/* needConcurrencySupport = */ true),
        m_pageAllocator(&m_allocationPolicyManager, Js::Configuration::Global.flags),
        m_allocator(arenaName, &m_pageAllocator, Js::Throw::OutOfMemory)
    {
    }
    ArenaAllocator* GetAllocator() { return &m_allocator; }
};

static ArenaHost s_arenaHost1(_u("For Output::Trace (1)"));
static ArenaHost s_arenaHost2(_u("For Output::Trace (2)"));

ArenaAllocator* GetOutputAllocator1()
{
    return s_arenaHost1.GetAllocator();
}

ArenaAllocator* GetOutputAllocator2()
{
    return s_arenaHost2.GetAllocator();
}
#endif

void ConfigParser::ParseOnModuleLoad(CmdLineArgsParser& parser, HANDLE hmod)
{
    Assert(!s_moduleConfigParser.HasReadConfig());

    s_moduleConfigParser.ParseRegistry(parser);
    s_moduleConfigParser.ParseConfig(hmod, parser);
    s_moduleConfigParser.ProcessConfiguration(hmod);
    // 'parser' destructor post-processes some configuration
}

void ConfigParser::ParseRegistry(CmdLineArgsParser &parser)
{
#ifdef _WIN32
    HKEY hk;
    bool includeUserHive = true;

    if (NOERROR == RegOpenKeyExW(HKEY_LOCAL_MACHINE, JsUtil::ExternalApi::GetFeatureKeyName(), 0, KEY_READ, &hk))
    {
        DWORD dwValue;
        DWORD dwSize = sizeof(dwValue);

        ParseRegistryKey(hk, parser);

        // HKLM can prevent user config from being read.
        if (NOERROR == RegGetValueW(hk, nullptr, _u("AllowUserConfig"), RRF_RT_DWORD, nullptr, (LPBYTE)&dwValue, &dwSize) && dwValue == 0)
        {
            includeUserHive = false;
        }

        RegCloseKey(hk);
    }

    if (includeUserHive && NOERROR == RegOpenKeyExW(HKEY_CURRENT_USER, JsUtil::ExternalApi::GetFeatureKeyName(), 0, KEY_READ, &hk))
    {
        ParseRegistryKey(hk, parser);
        RegCloseKey(hk);
    }
#endif // _WIN32
}

void ConfigParser::ParseRegistryKey(HKEY hk, CmdLineArgsParser &parser)
{
#ifdef _WIN32
    DWORD dwSize;
    DWORD dwValue;

#if ENABLE_DEBUG_CONFIG_OPTIONS
    char16 regBuffer[MaxRegSize];
    dwSize = sizeof(regBuffer);
    if (NOERROR == RegGetValueW(hk, nullptr, _u("JScript9"), RRF_RT_REG_SZ, nullptr, (LPBYTE)regBuffer, &dwSize))
    {
        LPWSTR regValue = regBuffer, nextValue = nullptr;
        regValue = wcstok_s(regBuffer, _u(" "), &nextValue);
        while (regValue != nullptr)
        {
            int err = 0;
            if ((err = parser.Parse(regValue)) != 0)
            {
                break;
            }
            regValue = wcstok_s(nullptr, _u(" "), &nextValue);
        }
    }
#endif

    // MemSpect - This setting controls whether MemSpect instrumentation is enabled.
    // The value is treated as a bit field with the following bits:
    //   0x01 - Track Arena memory
    //   0x02 - Track Recycler memory
    //   0x04 - Track Page allocations
    dwValue = 0;
    dwSize = sizeof(dwValue);
    if (NOERROR == ::RegGetValueW(hk, nullptr, _u("MemSpect"), RRF_RT_DWORD, nullptr, (LPBYTE)&dwValue, &dwSize))
    {
        if (dwValue & 0x01)
        {
            ArenaMemoryTracking::Activate();
        }
        if (dwValue & 0x02)
        {
            RecyclerMemoryTracking::Activate();
        }
        if (dwValue & 0x04)
        {
            PageTracking::Activate();
        }
    }

    // JScriptJIT - This setting controls the JIT/interpretation of Jscript code.
    // The legal values are as follows:
    //      1- Force JIT code to be generated for everything.
    //      2- Force interpretation without profiling (turn off JIT)
    //      3- Default
    //      4- Interpreter, simple JIT, and full JIT run a predetermined number of times. Requires >= 3 calls to functions.
    //      5- Interpreter, simple JIT, and full JIT run a predetermined number of times. Requires >= 4 calls to functions.
    //      6- Force interpretation with profiling
    //
    // This reg key is present in released builds.  The QA team's tests use these switches to
    // get reliable JIT coverage in servicing runs done by IE/Windows.  Because this reg key is
    // released, the number of possible values is limited to reduce surface area.
    dwValue = 0;
    dwSize = sizeof(dwValue);
    if (NOERROR == RegGetValueW(hk, nullptr, _u("JScriptJIT"), RRF_RT_DWORD, nullptr, (LPBYTE)&dwValue, &dwSize))
    {
        Js::ConfigFlagsTable &configFlags = Js::Configuration::Global.flags;
        switch (dwValue)
        {
        case 1:
            configFlags.Enable(Js::ForceNativeFlag);
            configFlags.ForceNative = true;
            break;

        case 6:
            configFlags.Enable(Js::ForceDynamicProfileFlag);
            configFlags.ForceDynamicProfile = true;
            // fall through

        case 2:
            configFlags.Enable(Js::NoNativeFlag);
            configFlags.NoNative = true;
            break;

        case 3:
            break;

        case 4:
            configFlags.Enable(Js::AutoProfilingInterpreter0LimitFlag);
            configFlags.Enable(Js::ProfilingInterpreter0LimitFlag);
            configFlags.Enable(Js::AutoProfilingInterpreter1LimitFlag);
            configFlags.Enable(Js::SimpleJitLimitFlag);
            configFlags.Enable(Js::ProfilingInterpreter1LimitFlag);
            configFlags.Enable(Js::EnforceExecutionModeLimitsFlag);

            configFlags.AutoProfilingInterpreter0Limit = 0;
            configFlags.AutoProfilingInterpreter1Limit = 0;
            if (
#if ENABLE_DEBUG_CONFIG_OPTIONS
                configFlags.NewSimpleJit
#else
                DEFAULT_CONFIG_NewSimpleJit
#endif
                )
            {
                configFlags.ProfilingInterpreter0Limit = 0;
                configFlags.SimpleJitLimit = 0;
                configFlags.ProfilingInterpreter1Limit = 2;
            }
            else
            {
                configFlags.ProfilingInterpreter0Limit = 1;
                configFlags.SimpleJitLimit = 1;
                configFlags.ProfilingInterpreter1Limit = 0;
            }
            configFlags.EnforceExecutionModeLimits = true;
            break;

        case 5:
            configFlags.Enable(Js::AutoProfilingInterpreter0LimitFlag);
            configFlags.Enable(Js::ProfilingInterpreter0LimitFlag);
            configFlags.Enable(Js::AutoProfilingInterpreter1LimitFlag);
            configFlags.Enable(Js::SimpleJitLimitFlag);
            configFlags.Enable(Js::ProfilingInterpreter1LimitFlag);
            configFlags.Enable(Js::EnforceExecutionModeLimitsFlag);

            configFlags.AutoProfilingInterpreter0Limit = 0;
            configFlags.ProfilingInterpreter0Limit = 0;
            configFlags.AutoProfilingInterpreter1Limit = 1;
            if (
#if ENABLE_DEBUG_CONFIG_OPTIONS
                configFlags.NewSimpleJit
#else
                DEFAULT_CONFIG_NewSimpleJit
#endif
                )
            {
                configFlags.SimpleJitLimit = 0;
                configFlags.ProfilingInterpreter1Limit = 2;
            }
            else
            {
                configFlags.SimpleJitLimit = 2;
                configFlags.ProfilingInterpreter1Limit = 0;
            }
            configFlags.EnforceExecutionModeLimits = true;
            break;
        }
    }

    // EnumerationCompat
    // This setting allows disabling a couple of changes to enumeration:
    //     - A change that causes deleted property indexes to be reused for new properties, thereby changing the order in which
    //       properties are enumerated
    //     - A change that creates a true snapshot of the type just before enumeration, and enumerating only those properties. A
    //       property that was deleted before enumeration and is added back during enumeration will not be enumerated.
    // Values:
    //     0 - Default
    //     1 - Compatibility mode for enumeration order (disable changes described above)
    // This FCK does not apply to WWAs. WWAs should use the RC compat mode to disable these changes.
    dwValue = 0;
    dwSize = sizeof(dwValue);
    if (NOERROR == RegGetValueW(hk, nullptr, _u("EnumerationCompat"), RRF_RT_DWORD, nullptr, (LPBYTE)&dwValue, &dwSize))
    {
        if (dwValue == 1)
        {
            Js::Configuration::Global.flags.EnumerationCompat = true;
        }
    }

#ifdef ENABLE_PROJECTION
    // FailFastIfDisconnectedDelegate
    // This setting allows enabling fail fast if the delegate invoked is disconnected
    //     0 - Default return the error RPC_E_DISCONNECTED if disconnected delegate is invoked
    //     1 - Fail fast if disconnected delegate
    dwValue = 0;
    dwSize = sizeof(dwValue);
    if (NOERROR == RegGetValueW(hk, nullptr, _u("FailFastIfDisconnectedDelegate"), RRF_RT_DWORD, nullptr, (LPBYTE)&dwValue, &dwSize))
    {
        if (dwValue == 1)
        {
            Js::Configuration::Global.flags.FailFastIfDisconnectedDelegate = true;
        }
    }
#endif

    // ES6 feature control
    // This setting allows enabling\disabling es6 features
    //     0 - Enable ES6 flag - Also default behavior
    //     1 - Disable ES6 flag
    dwValue = 0;
    dwSize = sizeof(dwValue);
    if (NOERROR == RegGetValueW(hk, nullptr, _u("DisableES6"), RRF_RT_DWORD, nullptr, (LPBYTE)&dwValue, &dwSize))
    {
        Js::ConfigFlagsTable &configFlags = Js::Configuration::Global.flags;
        if (dwValue == 1)
        {
            configFlags.Enable(Js::ES6Flag);
            configFlags.SetAsBoolean(Js::ES6Flag, false);
        }
    }

    // WebAssembly experimental feature control
    //     1 - Enable WebAssembly Experimental features
    dwValue = 0;
    dwSize = sizeof(dwValue);
    if (NOERROR == RegGetValueW(hk, nullptr, _u("EnableWasmExperimental"), RRF_RT_DWORD, nullptr, (LPBYTE)&dwValue, &dwSize))
    {
        if (dwValue == 1)
        {
            Js::ConfigFlagsTable &configFlags = Js::Configuration::Global.flags;
            configFlags.Enable(Js::WasmExperimentalFlag);
            configFlags.SetAsBoolean(Js::WasmExperimentalFlag, true);
        }
    }

    // BgParse feature control
    //     0 - Disable BgParse
    //     1 - Enable BgParse
    dwValue = 0;
    dwSize = sizeof(dwValue);
    if (NOERROR == RegGetValueW(hk, nullptr, _u("EnableBgParse"), RRF_RT_DWORD, nullptr, (LPBYTE)&dwValue, &dwSize))
    {
        Js::ConfigFlagsTable &configFlags = Js::Configuration::Global.flags;
        configFlags.Enable(Js::BgParseFlag);
        if (dwValue == 0)
        {
            configFlags.SetAsBoolean(Js::BgParseFlag, false);
        }
        else if (dwValue == 1)
        {
            configFlags.SetAsBoolean(Js::BgParseFlag, true);
        }

#if ENABLE_DEBUG_CONFIG_OPTIONS
        Output::Print(_u("BgParse controlled by registry: %u\n"), dwValue);
#endif
    }

    // Spectre mitigation feature control
    // This setting allows enabling\disabling spectre mitigations
    //     0 - Disable Spectre mitigations
    //     1 - Enable Spectre mitigations - Also default behavior
    dwValue = 0;
    dwSize = sizeof(dwValue);
    if (NOERROR == RegGetValueW(hk, nullptr, _u("MitigateSpectre"), RRF_RT_DWORD, nullptr, (LPBYTE)&dwValue, &dwSize))
    {
        Js::ConfigFlagsTable &configFlags = Js::Configuration::Global.flags;
        configFlags.Enable(Js::MitigateSpectreFlag);
        if (dwValue == 0)
        {
            configFlags.SetAsBoolean(Js::MitigateSpectreFlag, false);
        }
        else if (dwValue == 1)
        {
            configFlags.SetAsBoolean(Js::MitigateSpectreFlag, true);
        }
    }

#ifdef ENABLE_BASIC_TELEMETRY
    SetConfigStringFromRegistry(hk, _u("Telemetry"), _u("Discriminator1"), Js::Configuration::Global.flags.TelemetryDiscriminator1);
    SetConfigStringFromRegistry(hk, _u("Telemetry"), _u("Discriminator2"), Js::Configuration::Global.flags.TelemetryDiscriminator2);
    SetConfigStringFromRegistry(hk, _u("Telemetry"), _u("RunType"), Js::Configuration::Global.flags.TelemetryRunType);
#endif

#endif // _WIN32
}

#ifdef _WIN32

void ConfigParser::SetConfigStringFromRegistry(_In_ HKEY hk, _In_z_ const char16* subKeyName, _In_z_ const char16* valName, _Inout_ Js::String& str)
{
    const char16* regValue = nullptr;
    DWORD len = 0;
    ReadRegistryString(hk, subKeyName, valName, &regValue, &len);
    if (regValue != nullptr)
    {
        str = regValue;
        // Js::String  makes a copy of buffer so delete here
        NoCheckHeapDeleteArray(len, regValue);
    }
}

/**
 * Read a string from the registry.  Will return nullptr if string registry entry 
 * doesn't exist, or if we can't allocate memory.  
 * Will allocate a char16* buffer on the heap. Caller is responsible for freeing.
 */
void ConfigParser::ReadRegistryString(_In_ HKEY hk, _In_z_ const char16* subKeyName, _In_z_ const char16* valName, _Outptr_result_maybenull_z_ const char16** sz, _Out_ DWORD* length)
{
    DWORD bufLength = 0;
    *length = 0;
    *sz = nullptr;

    // first read to get size of string
    DWORD result = RegGetValueW(hk, subKeyName, valName, RRF_RT_REG_SZ, nullptr, nullptr, &bufLength);
    if (NOERROR == result)
    {
        if (bufLength > 0)
        {
            byte* buf = NoCheckHeapNewArrayZ(byte, bufLength);
            if (buf != nullptr)
            {
                result = RegGetValueW(hk, subKeyName, valName, RRF_RT_REG_SZ, nullptr, buf, &bufLength);
                if (NOERROR == result)
                {
                    // if successful, bufLength won't include null terminator so add 1
                    *length = (bufLength / sizeof(char16)) + 1;
                    *sz = reinterpret_cast<char16*>(buf);
                }
                else
                {
                    NoCheckHeapDeleteArray(bufLength, buf);
                }
            }
        }
    }
}
#endif // _WIN32

void ConfigParser::ParseConfig(HANDLE hmod, CmdLineArgsParser &parser, const char16* strCustomConfigFile)
{
#if defined(ENABLE_DEBUG_CONFIG_OPTIONS) && CONFIG_PARSE_CONFIG_FILE
    Assert(!_hasReadConfig || strCustomConfigFile != nullptr);
    _hasReadConfig = true;

    const char16* configFileName = strCustomConfigFile;
    const char16* configFileExt = _u(""); /* in the custom config case,
                                             ext is expected to be passed
                                             in as part of the filename */

    if (configFileName == nullptr)
    {
        configFileName = _configFileName;
        configFileExt = _u(".config");
    }

    int err = 0;
    char16 modulename[_MAX_PATH];
    char16 filename[_MAX_PATH];

    GetModuleFileName((HMODULE)hmod, modulename, _MAX_PATH);
    char16 drive[_MAX_DRIVE];
    char16 dir[_MAX_DIR];

    _wsplitpath_s(modulename, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
    _wmakepath_s(filename, drive, dir, configFileName, configFileExt);

    FILE* configFile;
#ifdef _WIN32
    if (_wfopen_s(&configFile, filename, _u("r, ccs=UNICODE")) != 0 || configFile == nullptr)
    {
        WCHAR configFileFullName[MAX_PATH];

        StringCchPrintf(configFileFullName, MAX_PATH, _u("%s%s"), configFileName, configFileExt);

        // try the one in the current working directory (Desktop)
        if (_wfullpath(filename, configFileFullName, _MAX_PATH) == nullptr)
        {
            return;
        }

        if (_wfopen_s(&configFile, filename, _u("r, ccs=UNICODE")) != 0 || configFile == nullptr)
        {
            return;
        }
    }
#else
    // Two-pathed for a couple reasons
    // 1. PAL doesn't like the ccs option passed in.
    // 2. _wfullpath is not implemented in the PAL.
    // Instead, on xplat, we'll check the HOME directory to see if there is
    // a config file there that we can use
    if (_wfopen_s(&configFile, filename, _u("r")) != 0 || configFile == nullptr)
    {
        WCHAR homeDir[MAX_PATH];

        if (GetEnvironmentVariable(_u("HOME"), homeDir, MAX_PATH) == 0)
        {
            return;
        }
        
        WCHAR configFileFullName[MAX_PATH];

        StringCchPrintf(configFileFullName, MAX_PATH, _u("%s/%s%s"), homeDir, configFileName, configFileExt);
        if (_wfopen_s(&configFile, configFileFullName, _u("r")) != 0 || configFile == nullptr)
        {
            return;
        }
    }
#endif

    char16 configBuffer[MaxTokenSize];
    int index = 0;

#ifdef _WIN32
#define ReadChar(file) fgetwc(file)
#define UnreadChar(c, file) ungetwc(c, file)
#define CharType wint_t
#define EndChar WEOF
#else
#define ReadChar(file) fgetc(file)
#define UnreadChar(c, file) ungetc(c, file)
#define CharType int
#define EndChar EOF
#endif

    // We don't expect the token to overflow- if it does
    // the simplest thing to do would be to ignore the
    // read tokens
    // We could use _fwscanf_s here but the function
    // isn't implemented in the PAL and we'd have to deal with
    // wchar => char16 impedance mismatch.
    while (index < MaxTokenSize)
    {
        CharType curChar = ReadChar(configFile);

        if (this->_flags.rawInputFromConfigFileIndex < sizeof(this->_flags.rawInputFromConfigFile) / sizeof(this->_flags.rawInputFromConfigFile[0]))
        {
            this->_flags.rawInputFromConfigFile[this->_flags.rawInputFromConfigFileIndex++] = curChar;
        }

        if (curChar == EndChar || isspace(curChar) || curChar == 0)
        {
            configBuffer[index] = 0;

            // Parse only if there's something in configBuffer
            if (index > 0 && (err = parser.Parse(configBuffer)) != 0)
            {
                break;
            }

            while(curChar != EndChar && (isspace(curChar) || curChar == 0))
            {
                curChar = ReadChar(configFile);
            }

            if (curChar == EndChar)
            {
                break;
            }
            else
            {
                UnreadChar(curChar, configFile);
            }

            index = 0;
        }
        else
        {
            // The expectation is that non-ANSI characters
            // are not used in the config- otherwise it will
            // be interpreted incorrectly here
            configBuffer[index++] = (char16) curChar;
        }
    }

#undef ReadChar
#undef UnreadChar
#undef CharType
#undef EndChar

    fclose(configFile);

    if (err !=0)
    {
        return;
    }
#endif
}

void ConfigParser::ProcessConfiguration(HANDLE hmod)
{
#if defined(ENABLE_DEBUG_CONFIG_OPTIONS)
    bool hasOutput = false;
    char16 modulename[_MAX_PATH];

    GetModuleFileName((HMODULE)hmod, modulename, _MAX_PATH);

    // Win32 specific console creation code
    // xplat-todo: Consider having this mechanism available on other
    // platforms
    // Not a pressing need since ChakraCore runs only in consoles by
    // default so we don't need to allocate a second console for this
#if CONFIG_CONSOLE_AVAILABLE
    if (Js::Configuration::Global.flags.Console)
    {
        int fd;
        FILE *fp;

        // fail usually means there is an existing console. We don't really care.
        if (AllocConsole())
        {
            fd = _open_osfhandle((intptr_t)GetStdHandle(STD_OUTPUT_HANDLE), O_TEXT);
            fp = _wfdopen(fd, _u("w"));

            if (fp != nullptr)
            {
                *stdout = *fp;                
                setvbuf(stdout, nullptr, _IONBF, 0);

                fd = _open_osfhandle((intptr_t)GetStdHandle(STD_ERROR_HANDLE), O_TEXT);
                fp = _wfdopen(fd, _u("w"));

                if (fp != nullptr)
                {
                    *stderr = *fp;
                    setvbuf(stderr, nullptr, _IONBF, 0);

                    char16 buffer[_MAX_PATH + 70];

                    if (ConfigParserAPI::FillConsoleTitle(buffer, _MAX_PATH + 20, modulename))
                    {
                        SetConsoleTitle(buffer);
                    }

                    hasOutput = true;
                }
            }
        }
    }
#endif

    if (Js::Configuration::Global.flags.IsEnabled(Js::OutputFileFlag)
        && Js::Configuration::Global.flags.OutputFile != nullptr)
    {
        SetOutputFile(Js::Configuration::Global.flags.OutputFile, Js::Configuration::Global.flags.OutputFileOpenMode);
        hasOutput = true;
    }

    if (Js::Configuration::Global.flags.DebugWindow)
    {
        Output::UseDebuggerWindow();
        hasOutput = true;
    }

#ifdef ENABLE_TRACE
    if (CONFIG_FLAG(InMemoryTrace))
    {
        Output::SetInMemoryLogger(
            Js::MemoryLogger::Create(::GetOutputAllocator1(),
            CONFIG_FLAG(InMemoryTraceBufferSize) * 3));   // With stack each trace is 3 entries (header, msg, stack).
        hasOutput = true;
    }

#ifdef STACK_BACK_TRACE
    if (CONFIG_FLAG(TraceWithStack))
    {
        Output::SetStackTraceHelper(Js::StackTraceHelper::Create(::GetOutputAllocator2()));
    }
#endif // STACK_BACK_TRACE
#endif // ENABLE_TRACE

    if (hasOutput)
    {
        ConfigParserAPI::DisplayInitialOutput(modulename);

        Output::Print(_u("\n"));

        Js::Configuration::Global.flags.VerboseDump();
        Output::Flush();
    }

    if (Js::Configuration::Global.flags.ForceSerialized)
    {
        // Can't generate or execute byte code under forced serialize
        Js::Configuration::Global.flags.GenerateByteCodeBufferReturnsCantGenerate = true;
        Js::Configuration::Global.flags.ExecuteByteCodeBufferReturnsInvalidByteCode = true;
    }

    ForcedMemoryConstraint::Apply();
#endif

#ifdef MEMSPECT_TRACKING
    bool all = false;
    if (Js::Configuration::Global.flags.Memspect.IsEnabled(Js::AllPhase))
    {
        all = true;
    }
    if (all || Js::Configuration::Global.flags.Memspect.IsEnabled(Js::RecyclerPhase))
    {
        RecyclerMemoryTracking::Activate();
    }
    if (all || Js::Configuration::Global.flags.Memspect.IsEnabled(Js::PageAllocatorPhase))
    {
        PageTracking::Activate();
    }
    if (all || Js::Configuration::Global.flags.Memspect.IsEnabled(Js::ArenaPhase))
    {
        ArenaMemoryTracking::Activate();
    }
#endif
}

HRESULT ConfigParser::SetOutputFile(const WCHAR* outputFile, const WCHAR* openMode)
{
    // If present, replace the {PID} token with the process ID
    const WCHAR* pidStr = nullptr;
    WCHAR buffer[_MAX_PATH];
    if ((pidStr = wcsstr(outputFile, _u("{PID}"))) != nullptr)
    {
        size_t pidStartPosition = pidStr - outputFile;

        WCHAR* pDest = buffer;
        size_t bufferLen = _MAX_PATH;

        // Copy the filename before the {PID} token
        wcsncpy_s(pDest, bufferLen, outputFile, pidStartPosition);
        pDest += pidStartPosition;
        bufferLen = bufferLen - pidStartPosition;

        // Copy the PID
        _itow_s(GetCurrentProcessId(), pDest, /*bufferSize=*/_MAX_PATH - pidStartPosition, /*radix=*/10);
#pragma prefast(suppress: 26014, "ultow string length is smaller than 256")
        pDest += wcslen(pDest);
        bufferLen = bufferLen - wcslen(pDest);

        // Copy the rest of the string.
#pragma prefast(suppress: 26014, "Overwriting pDset's null terminator is intentional since the string being copied is null terminated")
        wcscpy_s(pDest, bufferLen, outputFile + pidStartPosition + /*length of {PID}*/ 5);

        outputFile = buffer;
    }

    char16 fileName[_MAX_PATH];
    char16 moduleName[_MAX_PATH];
    PlatformAgnostic::SystemInfo::GetBinaryLocation(moduleName, _MAX_PATH);
    _wsplitpath_s(moduleName, nullptr, 0, nullptr, 0, fileName, _MAX_PATH, nullptr, 0);
    if (_wcsicmp(fileName, _u("WWAHost")) == 0 ||
        _wcsicmp(fileName, _u("ByteCodeGenerator")) == 0 ||
        _wcsicmp(fileName, _u("spartan")) == 0 ||
        _wcsicmp(fileName, _u("spartan_edge")) == 0 ||
        _wcsnicmp(fileName, _u("MicrosoftEdge"), wcslen(_u("MicrosoftEdge"))) == 0)
    {

        // we need to output to %temp% directory in wwa. we don't have permission otherwise.
        if (GetEnvironmentVariable(_u("temp"), fileName, _MAX_PATH) != 0)
        {
            wcscat_s(fileName, _MAX_PATH, _u("\\"));
            const char16 * fileNameOnly = wcsrchr(outputFile, _u('\\'));
            // if outputFile is full path we just need filename, discard the path
            wcscat_s(fileName, _MAX_PATH, fileNameOnly == nullptr ? outputFile : fileNameOnly);
        }
        else
        {
            AssertMsg(FALSE, "Get temp environment failed");
        }
        outputFile = fileName;
    }

    FILE *fp;
    if ((fp = _wfsopen(outputFile, openMode, _SH_DENYWR)) != nullptr)
    {
        Output::SetOutputFile(fp);
        return S_OK;
    }

    AssertMsg(false, "Could not open file for logging output.");
    return E_FAIL;
}
