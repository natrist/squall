#include "storm/Log.hpp"
#include "Log.hpp"
#include "storm/Thread.hpp"
#include "storm/Error.hpp"
#include <bc/Memory.hpp>
#include <bc/os/File.hpp>
#include <bc/os/Path.hpp>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <algorithm>

#if defined(WHOA_SYSTEM_MAC) || defined(WHOA_SYSTEM_LINUX)
#include <sys/time.h>
#endif

#if defined(WHOA_SYSTEM_MAC)
#include <mach/mach_time.h>
#endif

#define STORM_LOG_MAX_CHANNELS 4
#define STORM_LOG_MAX_BUFFER 0x10000
#define STORM_LOG_FLUSH_POINT 0xC000

struct SLOGTIME {
    uint16_t wYear;
    uint16_t wMonth;
    uint16_t wDayOfWeek;
    uint16_t wDay;
    uint16_t wHour;
    uint16_t wMinute;
    uint16_t wSecond;
    uint16_t wMilliseconds;
};

struct LOG {
    HSLOG    log;
    LOG*     next;
    char     filename[STORM_MAX_PATH];
    HOSFILE  file;
    uint32_t flags;
    size_t   bufferused;
    size_t   pendpoint;
    int32_t  indent;
    int32_t  timeStamp;
    char     buffer[STORM_LOG_MAX_BUFFER];
};

#if defined(WHOA_SYSTEM_WIN)

static CRITICAL_SECTION s_critsect[STORM_LOG_MAX_CHANNELS];
static CRITICAL_SECTION s_defaultdir_critsect;

#define INITLOCK(i) InitializeCriticalSection(&s_critsect[i])

#define LOCK(i) EnterCriticalSection(&s_critsect[i])

#define UNLOCK(i) LeaveCriticalSection(&s_critsect[i])

#define DESTROYDESTROY(i) DeleteCriticalSection(&s_critsect[i])

#define INITDEFAULTDIRLOCK InitializeCriticalSection(&s_defaultdir_critsect)

#define LOCKDEFAULTDIR EnterCriticalSection(&s_defaultdir_critsect)

#define UNLOCKDEFAULTDIR LeaveCriticalSection(&s_defaultdir_critsect)

#define DESTROYDEFAULTDIRLOCK DeleteCriticalSection(&s_default_dir_critsect)

#endif

#if defined(WHOA_SYSTEM_MAC) || defined(WHOA_SYSTEM_LINUX)

static pthread_mutex_t s_mutex[STORM_LOG_MAX_CHANNELS];
static pthread_mutex_t s_defaultdir_mutex;

#define INITLOCK(i) pthread_mutex_init(&s_mutex[i], nullptr);

#define LOCK(i) pthread_mutex_lock(&s_mutex[i]);

#define UNLOCK(i) pthread_mutex_unlock(&s_mutex[i]);

#define DESTROYLOCK(i) pthread_mutex_destroy(&s_mutex[i]);

#define INITDEFAULTDIRLOCK pthread_mutex_init(&s_defaultdir_mutex, nullptr);

#define LOCKDEFAULTDIR pthread_mutex_lock(&s_defaultdir_mutex);

#define UNLOCKDEFAULTDIR pthread_mutex_unlock(&s_defaultdir_mutex);

#define DESTROYDEFAULTDIRLOCK pthread_mutex_destroy(&s_defaultdir_mutex);

#endif

static LOG* s_loghead[STORM_LOG_MAX_CHANNELS];
static HSLOG s_sequence;

static char s_defaultdir[STORM_MAX_PATH];
static bool s_logsysteminit;

static LOG* LockLog(HSLOG log, HLOCKEDLOG* lockedhandle, bool createifnecessary) {
    if (!log) {
        *lockedhandle = reinterpret_cast<HLOCKEDLOG>(intptr_t(-1));
        return nullptr;
    }

    auto index = reinterpret_cast<uintptr_t>(log) & (STORM_LOG_MAX_CHANNELS-1);

    LOCK(index);

    *lockedhandle = reinterpret_cast<HLOCKEDLOG>(index);

    auto currptr = s_loghead[index];
    auto nextptr = &s_loghead[index];

    while (currptr) {
        if (currptr->log == log) {
            return currptr;
        }

        nextptr = &currptr->next;
        currptr = currptr->next;
    }

    if (createifnecessary) {
        currptr = static_cast<LOG*>(ALLOC(sizeof(LOG)));
        if (!currptr) {
            UNLOCK(index);
            *lockedhandle = reinterpret_cast<HLOCKEDLOG>(intptr_t(-1));
            return nullptr;
        }
    }

    *nextptr = currptr;

    currptr->log = log;
    currptr->next = nullptr;
    *currptr->filename = '\0';
    currptr->file = nullptr;
    currptr->bufferused = 0;
    currptr->pendpoint = 0;

    return currptr;
}

static void UnlockLog(HLOCKEDLOG lockedhandle) {
    auto index = reinterpret_cast<uintptr_t>(lockedhandle);
    UNLOCK(index);
}

static void UnlockDeleteLog(LOG* logptr, HLOCKEDLOG lockedhandle) {
    auto index = reinterpret_cast<uintptr_t>(lockedhandle);
    auto log = s_loghead[index];
    auto p_next = &s_loghead[index];

    while (log && log != logptr) {
        p_next = &log->next;
        log = log->next;
    }

    if (log) {
        *p_next = log->next;
        FREE(log);
    }

    UNLOCK(index);
}

static void FlushLog(LOG* logptr) {
    if (!logptr->bufferused) {
        return;
    }

    STORM_ASSERT(logptr->file);
    uint32_t bytes;
    OsWriteFile(logptr->file, logptr->buffer, logptr->bufferused, &bytes);
    logptr->bufferused = 0;
    logptr->pendpoint = 0;
}

static const char* PrependDefaultDir(char* newfilename, uint32_t newfilenamesize, const char* filename) {
    if (!filename || !*filename || filename[1] == ':' || SStrChr(filename, '\\') || SStrChr(filename, '/')) {
        return filename;
    }

    LOCKDEFAULTDIR;

    if (*s_defaultdir) {
        SStrCopy(newfilename, s_defaultdir, newfilenamesize);
        SStrPack(newfilename, filename, newfilenamesize);
    } else {
        OsGetExePath(newfilename, newfilenamesize);
#if defined(WHOA_SYSTEM_WIN)
        SStrPack(newfilename, "\\", newfilenamesize);
#else
        SStrPack(newfilename, "/", newfilenamesize);
#endif
        SStrPack(newfilename, filename, newfilenamesize);
    }

    UNLOCKDEFAULTDIR;

    return newfilename;
}

static size_t PathGetRootChars(const char* path) {
    if (path[0] == '/') {
#if defined(WHOA_SYSTEM_MAC)
        if (!SStrCmpI(path, "/Volumes/", 9)) {
            const char* offset = SStrChr(path + 9, '/');
            if (offset) {
                return offset - path + 1;
            }
        }
#endif
        return 1;
    }

    if (SStrLen(path) < 2) {
        return 0;
    }

    if (path[1] == ':') {
        return (path[2] == '\\') ? 3 : 2;
    }

    if (path[0] != '\\' || path[1] != '\\') {
        return 0;
    }

    const char* slash1 = SStrChr(path + 2, '\\');
    if (!slash1) {
        return 0;
    }

    const char* slash2 = SStrChr(slash1 + 1, '\\');
    if (!slash2) {
        return 0;
    }

    return slash2 - path + 1;
}

static void PathStripFilename(char* path) {
    auto slash = std::max(SStrChrR(path, '/'), SStrChrR(path, '\\'));

    if (slash) {
        auto relative = path + PathGetRootChars(path);
        if (slash < relative) {
            *relative = '\0';
        } else {
            slash[1] = '\0';
        }
    }
}

static bool CreateFileDirectory(const char* path) {
    STORM_ASSERT(path);

    char buffer[STORM_MAX_PATH];
    SStrCopy(buffer, path, STORM_MAX_PATH);
    PathStripFilename(buffer);

    return OsCreateDirectory(buffer, 1) == 1;
}

static bool OpenLogFile(const char* filename, HOSFILE* file, uint32_t flags) {
    if (!filename || !*filename) {
        *file = nullptr;
        return false;
    }

    char newfilename[STORM_MAX_PATH];
    auto fileName = PrependDefaultDir(newfilename, STORM_MAX_PATH, filename);
    CreateFileDirectory(newfilename);
    *file = OsCreateFile(
        fileName,
        OS_GENERIC_WRITE,
        OS_FILE_SHARE_READ|OS_FILE_SHARE_WRITE,
        (flags & STORM_LOG_FLAG_APPEND) ? OS_OPEN_ALWAYS : OS_CREATE_ALWAYS,
        OS_FILE_ATTRIBUTE_NORMAL,
        OS_FILE_TYPE_DEFAULT);
    return *file != HOSFILE_INVALID;
}

static void OutputIndent(LOG* logptr) {
    int32_t indent = logptr->indent;
    if (indent > 0) {
        if (indent >= 128) {
            indent = 128;
        }
        memset(&logptr->buffer[logptr->bufferused], ' ', indent);
        logptr->buffer[indent + logptr->bufferused] = 0;
        logptr->bufferused += indent;
    }
}

static void OutputReturn(LOG* logptr) {
#if defined(WHOA_SYSTEM_WIN)
    logptr->buffer[logptr->bufferused++] = '\r';
#endif
    logptr->buffer[logptr->bufferused++] = '\n';
    logptr->buffer[logptr->bufferused] = '\0';
}

static void OutputTime(LOG* logptr, bool show) {
    static uint64_t lasttime = 0;
    static size_t timestrlen = 0;
    static char timestr[64] = { '\0' };

    if (logptr->timeStamp == 0) {
        return;
    }

    uint64_t ticks = 0;

#if defined(WHOA_SYSTEM_WIN)
    ticks = GetTickCount64();
#endif

#if defined(WHOA_SYSTEM_LINUX)
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC_COARSE, &ts) == 0) {
        ticks = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }
#endif

#if defined(WHOA_SYSTEM_MAC)
    ticks = mach_absolute_time();
#endif

    if (ticks != lasttime) {
        lasttime = ticks;


#if defined(WHOA_SYSTEM_WIN)
        SYSTEMTIME systime;
        GetLocalTime(&systime);
#else
        SLOGTIME systime;
        time_t t = time(nullptr);
        struct tm* ts = localtime(&t);
        systime.wYear = ts->tm_year + 1900;
        systime.wMonth = ts->tm_mon + 1;
        systime.wDayOfWeek = ts->tm_wday;
        systime.wDay = ts->tm_mday;
        systime.wHour = ts->tm_hour;
        systime.wMinute = ts->tm_min;
        systime.wSecond = ts->tm_sec;

        struct timeval tv = { 0 };
        gettimeofday(&tv, 0);
        systime.wMilliseconds = tv.tv_usec / 1000;
#endif

        timestrlen = SStrPrintf(timestr,
                                sizeof(timestr),
                                "%u/%u %02u:%02u:%02u.%03u  ",
                                systime.wMonth,
                                systime.wDay,
                                systime.wHour,
                                systime.wMinute,
                                systime.wSecond,
                                systime.wMilliseconds);
    }


    if (show) {
        memcpy(&logptr->buffer[logptr->bufferused], timestr, timestrlen);
    } else {
        memset(&logptr->buffer[logptr->bufferused], ' ', timestrlen);
    }
    logptr->bufferused += timestrlen;
    logptr->buffer[logptr->bufferused] = '\0';
}

static bool PrepareLog(LOG* logptr) {
    if (logptr->file) {
        return true;
    }

    if (OpenLogFile(logptr->filename, &logptr->file, logptr->flags)) {
        return true;
    }

    *logptr->filename = '\0';
    return false;
}

void SLogInitialize() {
    if (!s_logsysteminit) {
        for (uint32_t i = 0; i < STORM_LOG_MAX_CHANNELS; i++) {
            INITLOCK(i);
        }

        INITDEFAULTDIRLOCK;
        s_logsysteminit = true;
    }
}

int32_t SLogIsInitialized() {
    return s_logsysteminit ? 1 : 0;
}

void SLogDestroy() {
    for (uint32_t i = 0; i < STORM_LOG_MAX_CHANNELS; ++i) {
        LOCK(i);
        auto log = s_loghead[i];
        while (log) {
            if (log->file) {
                FlushLog(log);
                OsCloseFile(log->file);
            }
            auto next = log->next;
            FREE(log);
            log = next;
        }
        s_loghead[i] = nullptr;
        UNLOCK(i);
        DESTROYLOCK(i);
    }

    DESTROYDEFAULTDIRLOCK;
    s_logsysteminit = false;
}

int32_t SLogCreate(const char* filename, uint32_t flags, HSLOG* log) {
    STORM_ASSERT(filename);
    STORM_ASSERT(*filename);
    STORM_ASSERT(log);

    HOSFILE file = HOSFILE_INVALID;
    HLOCKEDLOG lockedhandle;

    *log = 0;

    if (flags & STORM_LOG_FLAG_NO_FILE) {
        filename = "";
        flags &= ~STORM_LOG_FLAG_OPEN_FILE;
    }

    if ((flags & STORM_LOG_FLAG_OPEN_FILE) == 0 || OpenLogFile(filename, &file, flags)) {
        s_sequence = reinterpret_cast<HSLOG>(reinterpret_cast<size_t>(s_sequence) + 1);
        *log = s_sequence;
        auto result = LockLog(s_sequence, &lockedhandle, true);
        if (result) {
            result->file = file;
            SStrCopy(result->filename, filename, STORM_MAX_PATH);
            result->flags = flags;
            result->timeStamp = 1;
            result->indent = 0;
            UnlockLog(lockedhandle);
            return 1;
        } else {
            *log = 0;
        }
    }
    return 0;
}

void SLogClose(HSLOG log) {
    if (!s_logsysteminit) {
        return;
    }

    HLOCKEDLOG lockedhandle;
    auto logptr = LockLog(log, &lockedhandle, false);
    if (!logptr) {
        return;
    }

    if (logptr->file) {
        FlushLog(logptr);
        OsCloseFile(logptr->file);
    }

    UnlockDeleteLog(logptr, lockedhandle);
}

void SLogFlush(HSLOG log) {
    HLOCKEDLOG lockedhandle;
    auto logptr = LockLog(log, &lockedhandle, false);
    if (logptr) {
        if (logptr->file) {
            FlushLog(logptr);
        }
        UnlockLog(lockedhandle);
    }
}

void SLogFlushAll() {
    for (uint32_t i = 0; i < STORM_LOG_MAX_CHANNELS; ++i) {
        LOCK(i);
        for (auto log = s_loghead[i]; log; log = log->next) {
            if (log->file) {
                FlushLog(log);
            }
        }
        UNLOCK(i);
    }
}

void SLogGetDefaultDirectory(char* dirname, size_t dirnamesize) {
    LOCKDEFAULTDIR;
    SStrCopy(dirname, s_defaultdir, dirnamesize);
    UNLOCKDEFAULTDIR;
}

void SLogSetDefaultDirectory(const char* dirname) {
    LOCKDEFAULTDIR;
    auto size = SStrCopy(s_defaultdir, dirname, STORM_MAX_PATH);
#if defined(WHOA_SYSTEM_WIN)
    char slash = '\\';
#else
    char slash = '/';
#endif
    if (size < STORM_MAX_PATH - 1 && s_defaultdir[size - 1] != '/' && s_defaultdir[size - 1] != '\\') {
        s_defaultdir[size] = slash;
        s_defaultdir[size + 1] = 0;
    }
    UNLOCKDEFAULTDIR;
}

int32_t SLogSetAbsIndent(HSLOG log, int32_t indent) {
    HLOCKEDLOG lockedhandle;
    auto logptr = LockLog(log, &lockedhandle, false);
    if (logptr) {
        int32_t result = logptr->indent;
        logptr->indent = indent;
        UnlockLog(lockedhandle);
        return result;
    }
    return 0;
}

int32_t SLogSetIndent(HSLOG log, int32_t deltaIndent) {
    HLOCKEDLOG lockedhandle;
    auto logptr = LockLog(log, &lockedhandle, false);
    if (logptr) {
        int32_t result = logptr->indent;
        logptr->indent = result + deltaIndent;
        UnlockLog(lockedhandle);
        return result;
    }
    return 0;
}

void SLogSetTimestamp(HSLOG log, int32_t timeStamp) {
    HLOCKEDLOG lockedhandle;
    auto logptr = LockLog(log, &lockedhandle, false);
    if (logptr) {
        logptr->timeStamp = timeStamp;
        UnlockLog(lockedhandle);
    }
}

void SLogVWrite(HSLOG log, const char* format, va_list arglist) {
    HLOCKEDLOG lockedhandle;
    auto logptr = LockLog(log, &lockedhandle, false);
    if (!logptr) {
        return;
    }

    if (PrepareLog(logptr)) {
        OutputTime(logptr, true);
        OutputIndent(logptr);

        auto count = vsnprintf(
            &logptr->buffer[logptr->bufferused],
            STORM_LOG_MAX_BUFFER - logptr->bufferused,
            format,
            arglist);
        if (count > 0) {
            logptr->bufferused += count;
        }

        OutputReturn(logptr);

#if defined(WHOA_SYSTEM_WIN)
        if (g_opt.echotooutputdebugstring) {
            OutputDebugString(&logptr->buffer[logptr->pendpoint]);
        }
#else
        fputs(&logptr->buffer[logptr->pendpoint], stderr);
#endif

        logptr->pendpoint = logptr->bufferused;
        if (logptr->bufferused >= STORM_LOG_FLUSH_POINT) {
            FlushLog(logptr);
        }
    }
    UnlockLog(lockedhandle);
}

void SLogWrite(HSLOG log, const char* format, ...) {
    va_list arglist;
    va_start(arglist, format);
    SLogVWrite(log, format, arglist);
}
