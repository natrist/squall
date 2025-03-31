#include "storm/error/Error.hpp"
#include "storm/error/Types.hpp"
#include "storm/Thread.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

static uint32_t s_lasterror = ERROR_SUCCESS;
static APPFATINFO s_appFatInfo = {};

[[noreturn]] void SErrDisplayAppFatal(const char* format, ...) {
    // Format arguments
    constexpr size_t size = 1024;
    char buffer[size] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, size, format, args);
    va_end(args);

    SErrDisplayError(STORM_ERROR_APPLICATION_FATAL, s_appFatInfo.filename, s_appFatInfo.linenumber, buffer, 0, 1, 0);
}

int32_t SErrDisplayErrorFmt(uint32_t errorcode, const char* filename, int32_t linenumber, int32_t recoverable, uint32_t exitcode, const char* format, ...) {
    char buffer[2048];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    buffer[sizeof(buffer) - 1] = '\0';
    va_end(args);

    return SErrDisplayError(errorcode, filename, linenumber, buffer, recoverable, exitcode, 1);
}

void SErrPrepareAppFatal(const char* filename, int32_t linenumber) {
    s_appFatInfo.filename = filename;
    s_appFatInfo.linenumber = linenumber;
    s_appFatInfo.threadId = SGetCurrentThreadId();
}

void SErrSetLastError(uint32_t errorcode) {
    s_lasterror = errorcode;
#if defined(WHOA_SYSTEM_WIN)
    SetLastError(errorcode);
#endif
}

uint32_t SErrGetLastError() {
    return s_lasterror;
}
