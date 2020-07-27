//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifdef FLAG
FLAG(BSTR, dbgbaseline,                     "Baseline file to compare debugger output", NULL)
FLAG(bool, DebugLaunch,                     "Create the test debugger and execute test in the debug mode", false)
FLAG(BSTR, GenerateLibraryByteCodeHeader,   "Generate bytecode header file from library code", NULL)
FLAG(bool, GenerateParserStateCache,        "Parse source file to create parser state cache and write it to file or console", false)
FLAG(bool, UseParserStateCache,             "Create parser state cache while parsing and use it during script execution", false)
FLAG(int,  InspectMaxStringLength,          "Max string length to dump in locals inspection", 16)
FLAG(BSTR, Serialized,                      "If source is UTF8, deserializes from bytecode file", NULL)
FLAG(bool, OOPJIT,                          "Run JIT in a separate process", false)
FLAG(bool, EnsureCloseJITServer,            "JIT process will be force closed when ch is terminated", true)
FLAG(bool, IgnoreScriptErrorCode,           "Don't return error code on script error", false)
FLAG(bool, MuteHostErrorMsg,                "Mute host error output, e.g. module load failures", false)
FLAG(bool, TraceHostCallback,               "Output traces for host callbacks", false)
FLAG(bool, Test262,                         "load Test262 harness", false)
FLAG(bool, Module,                          "load the script as a module", false)
FLAG(bool, TrackRejectedPromises,           "Enable tracking of unhandled promise rejections", false)
FLAG(BSTR, CustomConfigFile,                "Custom config file to be used to pass in additional flags to Chakra", NULL)
FLAG(bool, ExecuteWithBgParse,              "Load script with bgparse (note: requires bgparse and parserstatecache be on as well)", false)
#undef FLAG
#endif
