#include "storm/Log.hpp"
#include "storm/Thread.hpp"
#include "storm/Error.hpp"
#include "storm/Memory.hpp"
#include <cstdio>
#include <cstdlib>
#if defined(WHOA_SYSTEM_LINUX)
#   include <sys/stat.h>
#   include <sys/types.h>
#endif


typedef struct _LOG {
    HSLOG log;
    _LOG* next;
    char filename[STORM_MAX_PATH];
    FILE* file;
    uint32_t flags;
    uint32_t bufferused;
    uint32_t pendpoint;
    int32_t indent;
    int32_t timeStamp;
    char buffer[0x10000];
} LOG;


static uint32_t lasttime;
static uint32_t timestrlen;
static char timestr[64];

static SCritSect* s_critsect[4];
static SCritSect* s_defaultdir_critsect;

static LOG* s_loghead[4];
static HSLOG s_sequence = 0;

static char s_defaultdir[STORM_MAX_PATH];
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

    if (result)
        return result;

    if (createifnecessary)
        result = (LOG*)SMemAlloc(sizeof(LOG), __FILE__, __LINE__, 0);

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
    if (!logptr->bufferused)
        return;

    STORM_ASSERT(logptr->file);
    fwrite(logptr->buffer, 1, logptr->bufferused, logptr->file);
    fflush(logptr->file);
    logptr->bufferused = 0;
    logptr->pendpoint = 0;
}

static const char* PrependDefaultDir(char* newfilename, uint32_t newfilenamesize, const char* filename)
{
    if (!filename || !filename[0] || filename[1] == ':' || SStrChr(filename, '\\') || SStrChr(filename, '/'))
        return filename;

    s_defaultdir_critsect->Enter();

    if (s_defaultdir[0]) {
        SStrCopy(newfilename, s_defaultdir, newfilenamesize);
        SStrPack(newfilename, filename, newfilenamesize);
    } else {
#if defined(WHOA_SYSTEM_WIN)
        GetModuleFileNameA(NULL, newfilename, newfilenamesize);
        char* slash = SStrChrR(newfilename, '\\');
        if (slash)
            slash[0] = '\0';

        SStrPack(newfilename, "\\", newfilenamesize);
        SStrPack(newfilename, filename, newfilenamesize);
#endif

#if defined(WHOA_SYSTEM_LINUX)
        char buffer[PATH_MAX];
        if (!realpath("/proc/self/exe", buffer))
            buffer[0] = '\0';

        char* slash = SStrChrR(buffer, '/');
        if (slash)
            slash[0] = '\0';

        SStrCopy(newfilename, buffer, newfilenamesize);
        SStrPack(newfilename, "/", newfilenamesize);
        SStrPack(newfilename, filename, newfilenamesize);
#endif

#if defined(WHOA_SYSTEM_MAC)
        // TODO: GetModuleFileName() for Mac implementation
#endif
    }

    s_defaultdir_critsect->Leave();
    return newfilename;
}

static size_t PathGetRootChars(const char* path) {
    if (path[0] == '/')
        return 1;

    size_t length = SStrLen(path);

    if (length < 2)
        return 0;

    if (path[1] == ':')
        return (path[2] == '\\') ? 3 : 2;

    if (path[0] != '\\' || path[1] != '\\')
        return 0;

    const char* v5 = path + 2;
    size_t v6 = 2;
    do {
        if (v5)
            v5 = SStrChr(v5, '\\') + 1;
        --v6;
    } while (v6);

    if (v5)
        return v5 - path;

    return length;
}

static void PathStripFilename(char* path) {
    char* v2; // edi
    char* v3; // eax
    char* v4; // eax

    v2 = SStrChrR(path, '\\');
    v3 = SStrChrR(path, '/');
    if (v2 <= v3)
        v2 = v3;

    if (v2) {
        v4 = &path[PathGetRootChars(path)];
        if (v2 >= v4)
            v2[1] = 0;
        else
            *v4 = 0;
    }
}

static void PathConvertSlashes(char* path) {
    while (*path) {
#if defined(WHOA_SYSTEM_WIN)
        if (*path == '/')
            *path = '\\';
#else
        if (*path == '\\')
            *path = '/';
#endif
        path++;
    }
}

static bool CreateFileDirectory(const char* path) {
    STORM_ASSERT(path);

    char buffer[STORM_MAX_PATH];
    SStrCopy(buffer, path, STORM_MAX_PATH);
    PathStripFilename(buffer);

    char* v1 = &buffer[PathGetRootChars(buffer)];
    PathConvertSlashes(v1);

    for (size_t i = 0; v1[i]; ++i) {
        if (v1[i] != '\\' && v1[i] != '/')
            continue;

        char slash = v1[i];
        v1[i] = '\0';
#if defined(WHOA_SYSTEM_WIN)
        CreateDirectoryA(buffer, NULL);
#else
        mkdir(buffer, 0755);
#endif
        v1[i] = slash;
    }

#if defined(WHOA_SYSTEM_WIN)
    return CreateDirectoryA(buffer, NULL);
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
        *file = fopen(newfilename, (flags & 2) ? "a+" : "w+");
        return (*file != nullptr);
    }
    return false;
}

void SLogInitialize() {
    if (s_logsysteminit)
        return;

    for (size_t i = 0; i < sizeof(s_loghead) / sizeof(s_loghead[0]); ++i)
        s_critsect[i] = new SCritSect();

    s_defaultdir_critsect = new SCritSect();

    s_logsysteminit = true;
}

bool SLogIsInitialized() {
    return s_logsysteminit;
}

void SLogDestroy() {
    // TODO: SLogFlushAll();
    // TODO: for (...) { SLogClose }

    for (size_t i = 0; i < sizeof(s_loghead) / sizeof(s_loghead[0]); ++i)
        delete s_critsect[i];

    delete s_defaultdir_critsect;

    s_logsysteminit = false;
}

uint32_t SLogCreate(const char* filename, uint32_t flags, HSLOG* log) {
    STORM_ASSERT(filename);
    STORM_ASSERT(*filename);
    STORM_ASSERT(log);

    FILE* file = nullptr;
    HLOCKEDLOG lockedhandle;

    *log = 0;

    if (flags & 2)
    {
        filename = "";
        flags &= ~1u;
    }

    if ((flags & 1) == 0 || OpenLogFile(filename, &file, flags)) {
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
    if (!s_logsysteminit)
        return;

    HLOCKEDLOG lockedhandle;
    LOG* logptr = LockLog(log, &lockedhandle, false);
    if (!logptr)
        return;

    if (logptr->file) {
        FlushLog(logptr);
        fclose(logptr->file);
    }

    UnlockDeleteLog(logptr, lockedhandle);
}
