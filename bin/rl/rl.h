//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// rl.h
// Header file for rl


#undef UNICODE
#undef _UNICODE
#include <windows.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "xmlreader.h"
#include "rlfeint.h"

// Note that some of these look pretty bad, and are; this is a test host, so
// we're not as concerned here.
#pragma warning(disable:4127) // expression is constant, e.g., while(TRUE)
#pragma warning(disable:6001) // using uninitialized memory
#pragma warning(disable:6011) // dereferencing null pointer, potentially
#pragma warning(disable:6031) // ignoring return value from some system calls
#pragma warning(disable:6054) // string may not be zero-terminated
#pragma warning(disable:6271) // Extra parameter not used by format string
#pragma warning(disable:6262) // Function using too much stack for analyzer to look at it
#pragma warning(disable:6335) // leaking process information handle
#pragma warning(disable:6386) // Potential buffer overrun
#pragma warning(disable:6387) // Potential misadherance to specification of library functions
#pragma warning(disable:26439) // implicit noexcept
#pragma warning(disable:26451) // Arithmetic on smaller type before widening conversion
#pragma warning(disable:26495) // uninitialized member
#pragma warning(disable:28193) // ignoring value that must be examined

#define LOCAL static
typedef __int32 int32;
typedef unsigned __int32 uint32;

#define BUFFER_SIZE 1024
#define MAXQUEUE    10000

extern void assert(const char *file, int line);
#define ASSERT(ex) ((ex) ? (void)0 : assert(__FILE__, __LINE__))
#define ASSERTNR ASSERT
#define UNREACHED FALSE

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))
#define REMAININGARRAYLEN(a, p) (ASSERT(a <= p && p <= a + ARRAYLEN(a)), ((sizeof(a) - (p - a)) / sizeof((a)[0])))

// Target machines

typedef struct tagTARGETINFO {
    const char * name;
    BOOL fRL_MACHINEonly;
    BOOL fAutoCrossCompilation;
    BOOL fUseNoGPF;
    const char * TARGET_VM;
    const char * LINKFLAGS;
    const char * NotTags;
} TARGETINFO;

extern TARGETINFO TargetInfo[];

// Any changes made here should have corresponding name added
// to TargetInfo in rl.cpp. NOTE: The TARGET_MACHINES enum is in exactly
// the same order as the TargetInfo array is initialized

typedef enum {
    TM_UNKNOWN,
    TM_X86,
    TM_PPCWCE,
    TM_WVM,
    TM_WVMCEE,
    TM_WVMX86,
    TM_MIPS,
    TM_ARM,
    TM_THUMB,
    TM_ARM64,
    TM_SH3,
    TM_SH4,
    TM_SH5C,
    TM_SH5M,
    TM_IA64,
    TM_AMD64,
    TM_AMD64SYS,
    TM_WVM64,
    TM_AM33,
    TM_M32R,
    TM_MSIL,
    TM_ALL
} TARGET_MACHINES;
extern TARGET_MACHINES TargetMachine;
extern TARGET_MACHINES RLMachine;

// Any changes made here should have corresponding name added
// to TargetOSNames in rl.cpp. NOTE: The TARGET_OS enum is in exactly
// the same order as the TargetOSNames array is initialized
typedef enum {
    TO_UNKNOWN,
    TO_WIN7,
    TO_WIN8,
    TO_WINBLUE,
    TO_WIN10,
    TO_WP8,
    TO_ALL
} TARGET_OS;
extern TARGET_OS TargetOS;
#define DEFAULT_OS TO_WIN7

#define CFG_ERROR_EX(file,line,fmt,args) fprintf(stderr, "Error: %s(%d) - " ##fmt "\n", file, line, args)
#define CFG_WARNING_EX(file,line,fmt,args) do { if (!FQuiet) printf("Warning: %s(%d) - " ##fmt "\n", file, line, (args)); } while (0)

// Parsing delimiters

#define OPT_DELIM " \t"
#define XML_DELIM ","
#define POGO_TEST_PREFIX "Pogo:"

// Set default target machine based on the host machine
// NOTE: this should be done dynamically, using getenv("PROCESSOR_ARCHITECTURE")

#if defined(_M_IX86)
#define DEFAULT_TM TM_X86
#elif defined(_M_PPC)
#define DEFAULT_TM TM_PPCWCE
#elif defined(_M_MIPS)
#define DEFAULT_TM TM_MIPS
#elif defined(_M_ARM)
#define DEFAULT_TM TM_ARM
#elif defined(_M_ARM64)
#define DEFAULT_TM TM_ARM64
#elif defined(_M_THUMB)
#define DEFAULT_TM TM_THUMB
#elif defined(_M_SH)
#define DEFAULT_TM TM_SH
#elif defined(_M_IA64)
#define DEFAULT_TM TM_IA64
#else
#define DEFAULT_TM TM_X86
#endif


#define RL_PRE_ENV_VAR "RL"
#define RL_POST_ENV_VAR "_RL_"


enum RLMODE
{
    RM_ASM, RM_EXE, RM_DIR
};
extern RLMODE Mode;
#define DEFAULT_RLMODE RM_EXE

#define DEFAULT_ASM_DCFG "rlexedirs.xml"
#define DEFAULT_ASM_CFG  "rlexe.xml"
#define DEFAULT_ASM_CMD  "rl"

#define DEFAULT_EXE_DCFG "rlexedirs.xml"
#define DEFAULT_EXE_CFG  "rlexe.xml"
#define DEFAULT_EXE_CMD  "rl"

#define DEFAULT_TESTLST_DCFG "test.lst"
#define DEFAULT_ENVLST_CFG "env.lst"

#define DEFAULT_REGR_CL     "cl"
#define DEFAULT_REGR_DIFF   "diff"

#define DEFAULT_LOG_FILE         "rl.log"
#define DEFAULT_FULL_LOG_FILE    "rl.full.log"
#define DEFAULT_RESULTS_LOG_FILE "rl.results.log"

#define DEFAULT_CROSS_TARGET_VM "issuerun -id:%d"

#define DEFAULT_LINKER "link"

#define DEFAULT_EXEC_TESTS_FLAGS ";"

#define DEFAULT_TEST_TIMEOUT 60000
#define DEFAULT_TEST_TIMEOUT_RETRIES 0

#define DIR_LOCKFILE  "regrlock.txt"


// Statistics index.
#ifndef _RL_STATS_DEFINED
#define _RL_STATS_DEFINED
typedef enum RL_STATS {
    RLS_TOTAL = 0, // overall stats
    RLS_EXE, // should == RLS_TOTAL at this time
    RLS_BASELINES, // baseline stats (== total if FBaseline && !FDiff)
    RLS_DIFFS, // diff stats (== total if FDiff && !FBaseline)
    RLS_COUNT
};
#endif

enum ExternalTestKind
{
    TK_MAKEFILE,
    TK_CMDSCRIPT,
    TK_JSCRIPT,
    TK_HTML,
    TK_COMMAND,
};

enum TestInfoKind
{
   TIK_FILES = 0,
   TIK_BASELINE,
   TIK_COMPILE_FLAGS,
   TIK_LINK_FLAGS,
   TIK_TAGS,
   TIK_RL_DIRECTIVES,
   TIK_ENV,
   TIK_COMMAND,
   TIK_TIMEOUT,
   TIK_TIMEOUT_RETRIES,
   TIK_SOURCE_PATH,
   TIK_EOL_NORMALIZATION,
   TIK_CUSTOM_CONFIG_FILE,
   _TIK_COUNT
};

extern const char * const TestInfoKindName[];

struct TestInfo
{
   BOOL hasData[_TIK_COUNT];
   const char * data[_TIK_COUNT];
};

struct Tags
{
   Tags * next;
   const char * str;
   BOOL fInclude;
};

struct ConditionNodeList
{
   ConditionNodeList * next;
   Xml::Node * node;
};

enum FILE_STATUS
{
    FS_NONE, FS_EXCLUDED, FS_INCLUDED
};

enum PROCESS_CONFIG_STATUS
{
    PCS_OK, PCS_ERROR, PCS_FILE_NOT_FOUND
};

enum FILE_CONFIG_STATUS
{
    FCS_USER_SPECIFIED, FCS_US_FLAGS, FCS_READ
};

template<typename T>
struct ListNode
{
    ListNode *next;
    T string;
};

typedef ListNode<char*> StringList;
typedef ListNode<const char*> ConstStringList;

struct TestVariant
{
   TestVariant() : next(NULL), optFlags(NULL) {}

   TestVariant * next;

   // Exe optimization flags.

   const char * optFlags;

   // Test-specific info.

   TestInfo testInfo;
};

struct Test
{
   Test * next;

   // For tests

   StringList * files;

   // For directories

   const char * name;
   char * fullPath;
   int num;

   // Common.

   ConditionNodeList * conditionNodeList;
   TestInfo defaultTestInfo;
   TestVariant * variants;
};

struct TestList
{
    Test * first;
    Test * last;
};

// Time options. Can do multiple types of timings, so do it bitwise
#define TIME_NOTHING   0x0
#define TIME_DIR       0x1
#define TIME_TEST      0x2
#define TIME_VARIATION 0x4
#define TIME_ALL       0x7  // bitmask of all timing options

typedef int TIME_OPTION;     // could be enum, but C++ doesn't like |= on enum

extern TIME_OPTION Timing;

/////////////////////////////////////////////////////////////////////////
//
// Class declarations. Non-inline member functions are defined in rlmp.cpp.
//
/////////////////////////////////////////////////////////////////////////

class CProtectedLong;
class CHandle;
class CDirectory;
class CDirectoryQueue;
class CThreadInfo;
class COutputBuffer;

/////////////////////////////////////////////////////////////////////////

// CProtectedLong: a single value protected by a critical section. This is
// used for things like test counts which are accessed by both worker threads
// and the display thread.

class CProtectedLong {
    CRITICAL_SECTION    _cs;
    int32                _value;

public:

    CProtectedLong() : _value(0)
    {
        InitializeCriticalSection(&_cs);
    }

    ~CProtectedLong()
    {
        DeleteCriticalSection(&_cs);
    }

    int32 operator++(int)
    {
        EnterCriticalSection(&_cs);
        int32 tmp = _value++;
        LeaveCriticalSection(&_cs);
        return tmp;
    }

    int32 operator--(int)
    {
        EnterCriticalSection(&_cs);
        int32 tmp = _value--;
        LeaveCriticalSection(&_cs);
        return tmp;
    }

    int32 operator+=(int32 incr)
    {
        EnterCriticalSection(&_cs);
        int32 tmp = (_value += incr);
        LeaveCriticalSection(&_cs);
        return tmp;
    }

    int32 operator=(int32 val)
    {
        EnterCriticalSection(&_cs);
        _value = val;
        LeaveCriticalSection(&_cs);
        return val;
    }

    bool operator==(CProtectedLong& rhs)
    {
        return _value == rhs._value;
    }

    operator int()  { return _value; }
};

/////////////////////////////////////////////////////////////////////////

// CHandle: a convenience class for storing HANDLE objects. When destructed
// or set to another value, the current handle is closed. Helps prevent
// handle leaks.

class CHandle {
    HANDLE _h;
public:
    CHandle() : _h(INVALID_HANDLE_VALUE) {}
    ~CHandle()
    {
        if (_h != INVALID_HANDLE_VALUE && _h != NULL)
            CloseHandle(_h);
    }

    HANDLE operator=(HANDLE h)
    {
        if (_h != INVALID_HANDLE_VALUE && _h != NULL)
            CloseHandle(_h);
        _h = h;
        return _h;
    }

    BOOL operator==(HANDLE h) { return h == _h ? TRUE : FALSE; }
    BOOL operator!=(HANDLE h) { return h != _h ? TRUE : FALSE; }
    operator HANDLE() { return _h; };
};

/////////////////////////////////////////////////////////////////////////

// WorkObject: a wrapper class for elements used by WorkQueue. Requires
// children classes to implement Dump if debug and keeps track of a _next
// for use by WorkQueue.

template <typename TWorkObject>
class WorkObject {
public:
    TWorkObject* _next;

public:
#ifndef NODEBUG
    virtual void Dump() = 0;
#endif
};

/////////////////////////////////////////////////////////////////////////

// WorkQueue: a generic class for keeping track of WorkObjects which need
// to be queued and worked on.

template <typename TWorkObject>
class WorkQueue {
protected:
    int _length;
    TWorkObject* _head;
    TWorkObject* _tail;
    CRITICAL_SECTION _cs;       // ensure proper synchronized access
    CHandle _hWorkAvailSem;     // signalled whenever there's work on the list
    CHandle _hMaxWorkQueueSem;  // used to control length of work queue

public:
    WorkQueue()
        : _head(NULL), _tail(NULL), _length(0)
    {
        SECURITY_ATTRIBUTES sa;

        InitializeCriticalSection(&_cs);

        /*
         * Create the semaphores for the work lists
         */
        memset(&sa, 0, sizeof(sa));
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;
        _hWorkAvailSem = CreateSemaphoreW(&sa, 0, 100000, NULL);
        if (_hWorkAvailSem == NULL)
            Fatal("Unable to create semaphore (err %u)!\n", GetLastError());

        _hMaxWorkQueueSem = CreateSemaphoreW(&sa, MAXQUEUE, MAXQUEUE, NULL);
        if (_hMaxWorkQueueSem == NULL)
            Fatal("Unable to create queue length semaphore (err %u)!\n", GetLastError());
    }

    ~WorkQueue()
    {
        // The CHandle objects clean up after themselves.

        DeleteCriticalSection(&_cs);
    }

    virtual TWorkObject* Pop() = 0;

    TWorkObject* Append(TWorkObject* pNew)
    {
        if (WaitForSingleObject(_hMaxWorkQueueSem, INFINITE) != WAIT_OBJECT_0) {
            Fatal("Semaphore wait failed, can't add to work list");
        }

        EnterCriticalSection(&_cs);
        if (_tail == NULL) {
            _head = _tail = pNew;
        } else {
            _tail->_next = pNew;
            _tail = pNew;
        }
        pNew->_next = NULL;
        ++_length;
        LeaveCriticalSection(&_cs);

        ReleaseSemaphore(_hWorkAvailSem, 1, NULL);

        return pNew;
    }

    int Length()
    {
        int tmp;

        EnterCriticalSection(&_cs);
        tmp = _length;
        LeaveCriticalSection(&_cs);

        return tmp;
    }

    DWORD WaitForWork(DWORD dwMilliseconds)
    {
        return WaitForSingleObject(_hWorkAvailSem, dwMilliseconds);
    }

    // The queue work available semaphore has a count for every
    // directory. When the queue is exhausted, each thread waits on
    // this semaphore again. So at the end, everyone will still be waiting!
    // So, add the number of threads to the count, so each thread
    // notices, one by one, that everything's done.
    void AdjustWaitForThreadCount()
    {
        ReleaseSemaphore(_hWorkAvailSem, (int)NumberOfThreads, NULL);
    }

    TWorkObject* GetNextItem()
    {
        DWORD dwWait = 0;
        TWorkObject* toReturn = NULL;

        while (TRUE)
        {
            dwWait = this->WaitForWork(1000);

            if (dwWait == WAIT_OBJECT_0) {
                // pick an item off the head of the work list
                toReturn = this->Pop();
                break;
            } else if (dwWait == WAIT_TIMEOUT) {
                if (bThreadStop)
                    break;
            } else {
                LogError("Thread %d: Semaphore wait failed\n", ThreadId);
                break;
            }
        }

        if (dwWait != WAIT_OBJECT_0)
            return NULL;

        if (bThreadStop)
            return NULL;

        return toReturn;
    }

    TWorkObject* GetNextItem_NoBlock(DWORD waitTime)
    {
        DWORD dwWait = 0;
        TWorkObject* toReturn = NULL;

        dwWait = this->WaitForWork(waitTime);

        if(dwWait == WAIT_OBJECT_0) {
            // pick an item off the head of the work list
            toReturn = this->Pop();
        }

        return toReturn;
    }

#ifndef NODEBUG
    void Dump()
    {
        TWorkObject* tmp;

        EnterCriticalSection(&_cs);
        printf("WorkQueue, length %d\n", _length);
        for (tmp = _head; tmp != NULL; tmp = tmp->_next) {
            tmp->Dump();
        }
        LeaveCriticalSection(&_cs);
    }
#endif
};

/////////////////////////////////////////////////////////////////////////

// CDirectory: represents a single directory and the tests that need to
// be run within it.

class CDirectory : public WorkObject<CDirectory> {
    friend class CDirectoryQueue;

    TestList _testList;     // list of files in directory
    Test * _pDir;           // directory info
    CHandle _dirLock;       // the directory lock file
    bool _isDiffDirectory;  // directory is for doing diffs, not masters

    bool _isDirStarted;     // indicates if the directory has begun executing
    time_t start_dir;       // time when dir began executing
    time_t elapsed_dir;     // time taken for dir to execute.
                            // initialized to 0 but set to a value upon directory finish.

    CRITICAL_SECTION _cs;   // ensure proper synchronized access

    // Try to lock the directory. If we successfully got the lock without
    // waiting, return true.
    // Note that since we use FILE_FLAG_DELETE_ON_CLOSE, the lock file should
    // go away if the process dies because of ctrl-c.
    bool TryLock();

    // Lock the directory; wait on the lock until it's available
    void WaitLock();

    // Output begin/finish summaries if all tests in the directory are executed.
    void WriteDirectoryBeginSummary();
    void WriteDirectoryFinishSummary();

public:
    // Multiple threads can be working in a single directory,
    // we'll need to make these CProtectedLong.
    CProtectedLong NumVariations, NumVariationsRun, NumFailures, NumDiffs;
    RL_STATS stat;          // which mode stat to update

    CDirectory(Test * pDir, TestList testList)
        : _pDir(pDir), _testList(testList), _isDiffDirectory(false), _isDirStarted(false), elapsed_dir(0)
    {
        InitializeCriticalSection(&_cs);
    }

    ~CDirectory();

    TestList * GetTestList() { return &_testList; }

    const char* GetDirectoryName() { return _pDir->name; }
    char* GetDirectoryPath() { return _pDir->fullPath; }
    int GetDirectoryNumber() { return _pDir->num; }

    BOOL HasTestInfoData(TestInfoKind testInfoKind) { return _pDir->defaultTestInfo.hasData[testInfoKind]; }
    const char* GetTestInfoData(TestInfoKind testInfoKind) { return _pDir->defaultTestInfo.data[testInfoKind]; }
    const char* GetFullPathFromSourceOrDirectory() { return HasTestInfoData(TIK_SOURCE_PATH) ? GetTestInfoData(TIK_SOURCE_PATH) : GetDirectoryPath(); }

    bool IsBaseline() { return !_isDiffDirectory; }
    void SetDiffFlag() { _isDiffDirectory = true; }

    void InitStats(int run);
    int32 IncRun(int inc = 1);
    int32 IncFailures(int inc = 1);
    int32 IncDiffs();

    // Trigger update of directory state.
    void UpdateState();
    void TryBeginDirectory();
    void TryEndDirectory();

    int Count(); // return count of tests in the directory

#ifndef NODEBUG
    void Dump();
#endif
};

/////////////////////////////////////////////////////////////////////////

// CDirectoryQueue: a queue of directories that need to have work done.

class CDirectoryQueue : public WorkQueue<CDirectory> {
public:
    ~CDirectoryQueue();

    // Return a directory to use. Normally, just peel the first one off the
    // work queue. However, if that directory is locked by another copy of
    // rl.exe, then skip it and try the next directory, until there are no
    // directories left. If that happens, just wait on the first locked
    // directory.
    CDirectory* Pop();
};

/////////////////////////////////////////////////////////////////////////

// CDirectoryAndTestCase: Keep track of directory and test case in a class for
// use in CDirectoryAndTestCaseQueue.
// Reuse CDirectory because it has all the nice profiling data already.

class CDirectoryAndTestCase : public WorkObject<CDirectoryAndTestCase> {
public:
    CDirectory* _pDir;
    Test* _pTest;

public:
    CDirectoryAndTestCase(CDirectory* pDir, Test* pTest)
        : _pDir(pDir), _pTest(pTest)
    {
        _next = NULL;
    }

    ~CDirectoryAndTestCase()
    {
        // This class is just a container for CDirectory and Test.
        // We don't want to clean up the objects here,
        // CDirectory will be cleaned up via ~CDirectoryQueue.
    }

#ifndef NODEBUG
    void Dump()
    {
        _pDir->Dump();
    }
#endif
};

/////////////////////////////////////////////////////////////////////////

// CDirectoryAndTestCaseQueue: a simple queue to hold CDirectoryAndTestCase objects.

class CDirectoryAndTestCaseQueue : public WorkQueue<CDirectoryAndTestCase> {
public:
    CDirectoryAndTestCase* Pop();
};

/////////////////////////////////////////////////////////////////////////

// CThreadInfo: a class with various bits of information about the current
// state of a thread. An array of these objects is created before the worker
// threads are started. This array is accessed like this: ThreadInfo[ThreadId],
// where ThreadId is a __declspec(thread) TLS variable. The status update
// thread uses the info here to construct the title bar.

class CThreadInfo
{
    CRITICAL_SECTION _cs;
    char _currentTest[BUFFER_SIZE];
    bool _isDone;

    struct TmpFileList
    {
        TmpFileList* _next;
        char* const _fullPath;

        TmpFileList(TmpFileList* next, char* fullPath)
            : _next(next), _fullPath(_strdup(fullPath))
        {
        }

        ~TmpFileList()
        {
            free(_fullPath);
        }

        TmpFileList(const TmpFileList&) = delete;
        void operator=(const TmpFileList&) = delete;
    };

    TmpFileList* _head;

public:

    CThreadInfo()
        : _head(NULL), _isDone(false)
    {
        InitializeCriticalSection(&_cs);
        _currentTest[0] = '\0';
    }

    ~CThreadInfo()
    {
        ClearTmpFileList();
        DeleteCriticalSection(&_cs);
    }

    bool IsDone() { return _isDone; }

    void Done();

    // Track current test, for use in creating the title bar

    void SetCurrentTest(const char* dir, const char* test, bool isBaseline);

    template <size_t bufSize>
    void GetCurrentTest(char (&currentTest)[bufSize])
    {
        EnterCriticalSection(&_cs);
        strcpy_s(currentTest, _currentTest);
        LeaveCriticalSection(&_cs);
    }

    // Track currently in-use temp files, so we can clean them up if we exit
    // by ctrl-c.

    void AddToTmpFileList(char* fullPath);
    void ClearTmpFileList();
    void DeleteTmpFileList();
};

/////////////////////////////////////////////////////////////////////////

// COutputBuffer: a buffered output class. Store up output so it can
// all be flushed at once.  Used to keep per-thread output from
// interleaving too much and becoming unreadable. This class is
// not synchronized. It is expected that there is one of these objects per
// output location (stdout, file, etc) per thread. Flushing the output *is*
// synchronized on a critical section that should be used around *all*
// worker thread stdio. (Actually, all normal output should go through this
// class.) A NULL file or filename is allowed, in which case everything
// happens as normal, but the Flush() operation doesn't display anything or
// send anything to disk.

class COutputBuffer
{
    char* _start;
    char* _end;
    bool _buffered; // true == buffer output; false == flush immediately (e.g., primary thread)
    bool _textGrabbed; // true == someone grabbed the text, used for asserting
    size_t _bufSize;
    enum { OUT_ILLEGAL=0, OUT_FILE, OUT_FILENAME } _type;
    union { // output type info
        FILE* _pfile;       // OUT_FILE
        char* _filename;    // OUT_FILENAME
    };

    // Set the pointers to indicate the buffer is empty. Don't reallocate
    // or free the buffer, though---it will get reused.
    void Reset();

    // Flush the output; synchronizes printf output using csStdio
    void Flush(FILE* pfile);

public:

    COutputBuffer(const char* logfile, bool buffered = true);

    COutputBuffer(FILE* pfile, bool buffered = true);

    ~COutputBuffer();

    const char *GetText() { ASSERTNR(_type == OUT_FILE); _textGrabbed = true; return _start; }

    // Add without doing varargs formatting (avoids local buffer size problems)
    void AddDirect(char* string);

    // Add text to the output buffer. Just like printf.
    void Add(const char* fmt, ...);

    // Flush the output to wherever it's going
    void Flush();
};

/////////////////////////////////////////////////////////////////////////

extern const char *DIFF_DIR;
extern char *REGRESS, *MASTER_DIR;
extern const char *REGR_CL, *REGR_DIFF;
extern char *REGR_ASM, *REGR_SHOWD;
extern const char *TARGET_VM;
extern char *EXTRA_CC_FLAGS, *EXEC_TESTS_FLAGS;
extern const char *LINKER, *LINKFLAGS;
extern const char *JCBinary;

extern BOOL FBaseline;
extern BOOL FRebase; // Whether creates .rebase file if testout mismatches baseline
extern BOOL FDiff;
extern BOOL FBaseDiff;
extern BOOL FVerbose;
extern BOOL FSummary;
extern BOOL FNoDelete;
extern BOOL FCopyOnFail;
extern BOOL FParallel;
extern BOOL FSyncEnumDirs;
extern BOOL FNogpfnt;
extern BOOL FTest;
extern BOOL FStopOnError;
extern BOOL FAppendTestNameToExtraCCFlags;
extern BOOL FNoProgramOutput;
extern BOOL FOnlyAssertOutput;

#define MAXOPTIONS 60
extern const char *OptFlags[MAXOPTIONS + 1], *PogoOptFlags[MAXOPTIONS + 1];

#ifndef NODEBUG
extern BOOL FDebug;
#endif

extern BOOL GStopDueToError;
extern BOOL bThreadStop;

extern BOOL FSyncImmediate;
extern BOOL FSyncVariation;
extern BOOL FSyncTest;
extern BOOL FSyncDir;
extern BOOL FNoThreadId;

extern char* BaseCompiler;
extern char* DiffCompiler;
extern CDirectoryQueue DiffDirectoryQueue;

extern unsigned NumberOfThreads;
extern CRITICAL_SECTION csCurrentDirectory; // used when changing current directory
extern CRITICAL_SECTION csStdio;            // for printf / fprintf synchronization
extern __declspec(thread) int ThreadId;
extern __declspec(thread) char *TargetVM;
extern __declspec(thread) COutputBuffer* ThreadOut; // stdout
extern __declspec(thread) COutputBuffer* ThreadLog; // log file
extern __declspec(thread) COutputBuffer* ThreadFull; // full log file

extern CThreadInfo* ThreadInfo;
extern CProtectedLong NumVariationsRun[RLS_COUNT];
extern CProtectedLong NumVariationsTotal[RLS_COUNT];
extern CProtectedLong NumFailuresTotal[RLS_COUNT];
extern CProtectedLong NumDiffsTotal[RLS_COUNT];

extern BOOL FRLFE;
extern char *RLFEOpts;

extern const char * const ModeNames[];

// rl.cpp

extern void __cdecl Fatal(const char *fmt, ...);
extern void __cdecl Warning(const char *fmt, ...);
extern void __cdecl Message(const char *fmt, ...);
extern void __cdecl WriteLog(const char *fmt, ...);
extern void __cdecl LogOut(const char *fmt, ...);
extern void __cdecl LogError(const char *fmt, ...);
extern void FlushOutput(void);
extern char *mytmpnam(const char* directory, const char *prefix, char *filename);
extern int DoCompare(char *file1, char *file2, BOOL normalizeLineEndings = false);
extern void UpdateTitleStatus();
extern int mystrcmp(const char *a, const char *b);
extern char * mystrtok(char *s, const char *delim, const char *term);
extern void FreeTestList(TestList * pTestList);
#ifndef NODEBUG
extern void DumpTestList(TestList * pTestList);
#endif
extern void DeleteMultipleFiles(CDirectory* pDir, const char *pattern);
extern char *GetFilenamePtr(char *path);
extern const char* GetFilenameExt(const char *path);
extern void DeleteFileMsg(char *filename);
extern BOOL DeleteFileIfFound(const char *filename);
extern void DeleteFileRetryMsg(char *filename);
extern StringList * ParseStringList(const char* p, const char* delim);
extern StringList * AppendStringList(StringList * stringList, StringList * appendList);
extern StringList * AppendStringListCopy(StringList * stringList, StringList * appendList);
extern void PrintTagsList(Tags* pTagsList);
extern void AddTagToTagsList(Tags** pTagsList, Tags** pTagsLast, const char* str, BOOL fInclude);

extern BOOL
SuppressNoGPF(
    Test * pTest
);

extern BOOL
HasInfo
(
   const char * szInfoList,
   const char * delim,
   const char * szInfo
);

extern BOOL
HasInfoList
(
   const char * szInfoList1,
   const char * delim1,
   const char * szInfoList2,
   const char * delim2,
   bool         allMustMatch
);

extern BOOL
GetTestInfoFromNode
(
   const char * fileName,
   Xml::Node * node,
   TestInfo *  testInfo
);

extern char * getenv_unsafe(const char *);
extern FILE * fopen_unsafe(const char *, const char *);
extern char* strerror_unsafe(int errnum);
// rlregr.cpp

extern void RegrInit(void);
extern BOOL RegrStartDir(char* path);
extern BOOL RegrEndDir(char* path);
extern int RegrFile(CDirectory* pDir, Test * pTest, TestVariant * pTestVariant);


// rlrun.cpp

extern void RunInit(void);
extern BOOL IsPogoTest(Test * pFilename);
extern BOOL RunStartDir(char *dir);
extern int ExecTest(CDirectory* pDir, Test * pTest, TestVariant * pTestVariant);


// rlmp.cpp

extern int ExecuteCommand(
    const char* path,
    const char* CommandLine,
    DWORD millisecTimeout = INFINITE,
    uint32 timeoutRetries = 0,
    void* envFlags = NULL);

extern int DoOneExternalTest(
    CDirectory* pDir,
    TestVariant* pTestVariant,
    char *optFlags,
    char *inCCFlags,
    char *inLinkFlags,
    char *testCmd,
    ExternalTestKind kind,
    BOOL fSyncVariationWhenFinished,
    BOOL fCleanBefore,
    BOOL fCleanAfter,
    BOOL fSuppressNoGPF,
    DWORD millisecTimeout = INFINITE,
    uint32 timeoutRetries = 0,
    void *envFlags = NULL
);

extern void
WriteSummary(
   const char *name,
   BOOL fBaseline,
   int tests,
   int diffs,
   int failures
);


// rlfeint.cpp

extern void RLFEInit(BYTE numThreads, int numDirs);
extern void RLFEAddRoot(RL_STATS stat, DWORD total);
extern void RLFEAddTest(RL_STATS stat, CDirectory *pDir);
extern void RLFEAddLog(CDirectory *pDir, RLFE_STATUS rlfeStatus, const char *testName, const char *subTestName, const char *logText);
extern void RLFETestStatus(CDirectory *pDir);
extern void RLFEThreadDir(CDirectory *pDir, BYTE num);
extern void RLFEThreadStatus(BYTE num, const char *text);
extern BOOL RLFEConnect(const char *prefix);
extern void RLFEDisconnect(BOOL fKilled);
