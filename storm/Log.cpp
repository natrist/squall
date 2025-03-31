#include "storm/Log.hpp"
#include "storm/Thread.hpp"
#include "storm/Error.hpp"
#include <bc/Memory.hpp>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>

#if defined(WHOA_SYSTEM_LINUX)
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/limits.h>
#endif

#if defined(WHOA_SYSTEM_MAC)
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <mach/mach_time.h>
#include <mach-o/dyld.h>
#include <sys/syslimits.h>
#endif

#define STORM_LOG_MAX_CHANNELS 4
#define STORM_LOG_MAX_BUFFER 0x10000
#define STORM_LOG_FLUSH_POINT 0xC000

#if defined(WHOA_SYSTEM_WIN)
typedef SYSTEMTIME SLOGTIME;
#else
typedef struct _SLOGTIME {
    uint16_t wYear;
    uint16_t wMonth;
    uint16_t wDayOfWeek;
    uint16_t wDay;
    uint16_t wHour;
    uint16_t wMinute;
    uint16_t wSecond;
    uint16_t wMilliseconds;
} SLOGTIME;
#endif

typedef struct _LOG {
    HSLOG log;
    _LOG* next;
    char filename[STORM_MAX_PATH];
    FILE* file;
    uint32_t flags;
    size_t bufferused;
    size_t pendpoint;
    int32_t indent;
    int32_t timeStamp;
    char buffer[STORM_LOG_MAX_BUFFER];
} LOG;

static SCritSect* s_critsect[STORM_LOG_MAX_CHANNELS] = { nullptr };
static SCritSect* s_defaultdir_critsect = nullptr;

static LOG* s_loghead[STORM_LOG_MAX_CHANNELS] = { nullptr };
static HSLOG s_sequence = 0;

static char s_defaultdir[STORM_MAX_PATH] = { '\0' };
static bool s_logsysteminit = false;

static LOG* LockLog(HSLOG log, HLOCKEDLOG* lockedhandle, bool createifnecessary) {
    if (!log) {
        *lockedhandle = (HLOCKEDLOG)-1;
        return nullptr;
    }

    size_t index = reinterpret_cast<size_t>(log) & 3;
    s_critsect[index]->Enter();
    *lockedhandle = (HLOCKEDLOG)index;

    LOG* result = s_loghead[index];
    LOG** p_next = &s_loghead[index];

    while (result && result->log != log) {
        p_next = &result->next;
        result = result->next;
    }

    if (result) {
        return result;
    }

    if (createifnecessary) {
        result = (LOG*)SMemAlloc(sizeof(LOG), __FILE__, __LINE__, 0);
    }

    if (!result) {
        s_critsect[index]->Leave();
        *lockedhandle = (HLOCKEDLOG)-1;
        return nullptr;
    }

    *p_next = result;
    result->log = log;
    result->next = nullptr;
    result->filename[0] = '\0';
    result->file = nullptr;
    result->bufferused = 0;
    result->pendpoint = 0;

    return result;
}

static void UnlockLog(HLOCKEDLOG lockedhandle) {
    s_critsect[reinterpret_cast<size_t>(lockedhandle)]->Leave();
}

static void UnlockDeleteLog(LOG* logptr, HLOCKEDLOG lockedhandle) {
    size_t index = reinterpret_cast<size_t>(lockedhandle);
    LOG* log = s_loghead[index];
    LOG** p_next = &s_loghead[index];

    while (log && log != logptr) {
        p_next = &log->next;
        log = log->next;
    }

    if (log) {
        *p_next = log->next;
        SMemFree(log);
    }

    s_critsect[index]->Leave();
}

static void FlushLog(LOG* logptr) {
    if (!logptr->bufferused) {
        return;
    }

    STORM_ASSERT(logptr->file);
    fwrite(logptr->buffer, 1, logptr->bufferused, logptr->file);
    fflush(logptr->file);
    logptr->bufferused = 0;
    logptr->pendpoint = 0;
}

static const char* PrependDefaultDir(char* newfilename, uint32_t newfilenamesize, const char* filename) {
    if (!filename || !filename[0] || filename[1] == ':' || SStrChr(filename, '\\') || SStrChr(filename, '/')) {
        return filename;
    }

    s_defaultdir_critsect->Enter();

    if (s_defaultdir[0]) {
        SStrCopy(newfilename, s_defaultdir, newfilenamesize);
        SStrPack(newfilename, filename, newfilenamesize);
    } else {
#if defined(WHOA_SYSTEM_WIN)
        GetModuleFileName(NULL, newfilename, newfilenamesize);
        char* slash = SStrChrR(newfilename, '\\');
        if (slash) {
            slash[0] = '\0';
        }

        SStrPack(newfilename, "\\", newfilenamesize);
        SStrPack(newfilename, filename, newfilenamesize);
#endif

#if defined(WHOA_SYSTEM_LINUX)
        char buffer[PATH_MAX];
        if (!realpath("/proc/self/exe", buffer)) {
            buffer[0] = '\0';
        }

        char* slash = SStrChrR(buffer, '/');
        if (slash) {
            slash[0] = '\0';
        }

        SStrCopy(newfilename, buffer, newfilenamesize);
        SStrPack(newfilename, "/", newfilenamesize);
        SStrPack(newfilename, filename, newfilenamesize);
#endif

#if defined(WHOA_SYSTEM_MAC)
        newfilename[0] = '\0';

        char path[PATH_MAX] = {0};
        char actualPath[PATH_MAX] = {0};
        uint32_t size = PATH_MAX;
        if (_NSGetExecutablePath(path, &size) == 0) {
            if (!realpath(path, actualPath)) {
                actualPath[0] = '\0';
            }
        }

        char* slash = SStrChrR(actualPath, '/');
        if (slash) {
            slash[0] = '\0';
        }

        SStrCopy(newfilename, actualPath, newfilenamesize);
        SStrPack(newfilename, "/", newfilenamesize);
        SStrPack(newfilename, filename, newfilenamesize);
#endif
    }

    s_defaultdir_critsect->Leave();
    return newfilename;
}

static size_t PathGetRootChars(const char* path) {
    if (path[0] == '/') {
        if (!SStrCmpI(path, "/Volumes/", 9)) {
            const char* offset = SStrChr(path + 9, '/');
            if (offset) {
                return offset - path + 1;
            }
        }
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
    char* slash = SStrChrR(path, '/');
    char* backslash = SStrChrR(path, '\\');
    if (slash <= backslash) {
        slash = backslash;
    }

    if (!slash) {
        return;
    }

    char* relative = &path[PathGetRootChars(path)];
    if (slash >= relative) {
        slash[1] = 0;
    } else {
        relative[0] = 0;
    }
}

static void PathConvertSlashes(char* path) {
    while (*path) {
#if defined(WHOA_SYSTEM_WIN)
        if (*path == '/') {
            *path = '\\';
        }
#else
        if (*path == '\\') {
            *path = '/';
        }
#endif
        path++;
    }
}

static bool CreateFileDirectory(const char* path) {
    STORM_ASSERT(path);

    char buffer[STORM_MAX_PATH];
    SStrCopy(buffer, path, STORM_MAX_PATH);
    PathStripFilename(buffer);

    char* relative = &buffer[PathGetRootChars(buffer)];
    PathConvertSlashes(relative);

    for (size_t i = 0; relative[i]; ++i) {
        if (relative[i] != '\\' && relative[i] != '/') {
            continue;
        }

        char slash = relative[i];
        relative[i] = '\0';
#if defined(WHOA_SYSTEM_WIN)
        CreateDirectory(buffer, NULL);
#else
        mkdir(buffer, 0755);
#endif
        relative[i] = slash;
    }

#if defined(WHOA_SYSTEM_WIN)
    return CreateDirectory(buffer, NULL);
#else
    return !mkdir(buffer, 0755);
#endif
}

static bool OpenLogFile(const char* filename, FILE** file, uint32_t flags) {
    *file = nullptr;

    if (filename && filename[0]) {
        char newfilename[STORM_MAX_PATH];
        PrependDefaultDir(newfilename, STORM_MAX_PATH, filename);
        CreateFileDirectory(newfilename);
        *file = fopen(newfilename, (flags & STORM_LOG_FLAG_APPEND) ? "ab" : "wb");
        return (*file != nullptr);
    }
    return false;
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

        SLOGTIME systime;

#if defined(WHOA_SYSTEM_WIN)
        GetLocalTime(&systime);
#else
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

    logptr->filename[0] = '\0';
    return false;
}

void SLogInitialize() {
    if (s_logsysteminit) {
        return;
    }

    for (size_t i = 0; i < STORM_LOG_MAX_CHANNELS; ++i) {
        s_critsect[i] = new SCritSect();
    }

    s_defaultdir_critsect = new SCritSect();

    s_logsysteminit = true;
}

int SLogIsInitialized() {
    return s_logsysteminit ? 1 : 0;
}

void SLogDestroy() {
    for (size_t i = 0; i < STORM_LOG_MAX_CHANNELS; ++i) {
        s_critsect[i]->Enter();
        LOG* log = s_loghead[i];
        while (log) {
            if (log->file) {
                FlushLog(log);
                fclose(log->file);
            }
            LOG* next = log->next;
            SMemFree(log);
            log = next;
        }
        s_loghead[i] = nullptr;
        s_critsect[i]->Leave();
        delete s_critsect[i];
    }

    delete s_defaultdir_critsect;
    s_logsysteminit = false;
}

int SLogCreate(const char* filename, uint32_t flags, HSLOG* log) {
    STORM_ASSERT(filename);
    STORM_ASSERT(*filename);
    STORM_ASSERT(log);

    FILE* file = nullptr;
    HLOCKEDLOG lockedhandle;

    *log = 0;

    if (flags & STORM_LOG_FLAG_NO_FILE) {
        filename = "";
        flags &= ~STORM_LOG_FLAG_OPEN_FILE;
    }

    if ((flags & STORM_LOG_FLAG_OPEN_FILE) == 0 || OpenLogFile(filename, &file, flags)) {
        s_sequence = reinterpret_cast<HSLOG>(reinterpret_cast<size_t>(s_sequence) + 1);
        *log = s_sequence;
        LOG* result = LockLog(s_sequence, &lockedhandle, true);
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
    LOG* logptr = LockLog(log, &lockedhandle, false);
    if (!logptr) {
        return;
    }

    if (logptr->file) {
        FlushLog(logptr);
        fclose(logptr->file);
    }

    UnlockDeleteLog(logptr, lockedhandle);
}

void SLogFlush(HSLOG log) {
    HLOCKEDLOG lockedhandle;
    LOG* logptr = LockLog(log, &lockedhandle, false);
    if (logptr) {
        if (logptr->file) {
            FlushLog(logptr);
        }
        UnlockLog(lockedhandle);
    }
}

void SLogFlushAll() {
    for (size_t i = 0; i < STORM_LOG_MAX_CHANNELS; ++i) {
        s_critsect[i]->Enter();
        for (LOG* log = s_loghead[i]; log; log = log->next) {
            if (log->file) {
                FlushLog(log);
            }
        }
        s_critsect[i]->Leave();
    }
}

void SLogGetDefaultDirectory(char* dirname, size_t dirnamesize) {
    s_defaultdir_critsect->Enter();
    SStrCopy(dirname, s_defaultdir, dirnamesize);
    s_defaultdir_critsect->Leave();
}

void SLogSetDefaultDirectory(const char* dirname) {
    s_defaultdir_critsect->Enter();
    size_t size = SStrCopy(s_defaultdir, dirname, STORM_MAX_PATH);
    PathConvertSlashes(s_defaultdir);
#if defined(WHOA_SYSTEM_WIN)
    char slash = '\\';
#else
    char slash = '/';
#endif
    if (size < STORM_MAX_PATH - 1 && s_defaultdir[size - 1] != slash) {
        s_defaultdir[size] = slash;
        s_defaultdir[size + 1] = 0;
    }
    s_defaultdir_critsect->Leave();
}

int32_t SLogSetAbsIndent(HSLOG log, int32_t indent) {
    HLOCKEDLOG lockedhandle;
    LOG* logptr = LockLog(log, &lockedhandle, false);
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
    LOG* logptr = LockLog(log, &lockedhandle, false);
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
    LOG* logptr = LockLog(log, &lockedhandle, false);
    if (logptr) {
        logptr->timeStamp = timeStamp;
        UnlockLog(lockedhandle);
    }
}

void SLogVWrite(HSLOG log, const char* format, va_list arglist) {
    HLOCKEDLOG lockedhandle;
    LOG* logptr = LockLog(log, &lockedhandle, false);
    if (!logptr) {
        return;
    }

    if (PrepareLog(logptr)) {
        OutputTime(logptr, true);
        OutputIndent(logptr);

        int count = vsnprintf(
            &logptr->buffer[logptr->bufferused],
            STORM_LOG_MAX_BUFFER - logptr->bufferused,
            format,
            arglist);
        if (count > 0) {
            logptr->bufferused += count;
        }

        OutputReturn(logptr);

#if defined(WHOA_SYSTEM_WIN)
        // if (g_opt.echotooutputdebugstring)
        OutputDebugString(&logptr->buffer[logptr->pendpoint]);
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
