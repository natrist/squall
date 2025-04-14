#ifndef STORM_LOG_HPP
#define STORM_LOG_HPP

#include <cstdarg>
#include <cstdint>

#include "storm/Handle.hpp"
#include "storm/String.hpp"

#define STORM_LOG_FLAG_DEFAULT   0 // Create or open log file with first SLogWrite() call
#define STORM_LOG_FLAG_OPEN_FILE 1 // Create or open log file with SLogCreate()
#define STORM_LOG_FLAG_NO_FILE   2 // Don't use log file (use OutputDebugString or console only)
#define STORM_LOG_FLAG_APPEND    4 // Don't truncate existing log file

DECLARE_STRICT_HANDLE(HSLOG);
DECLARE_STRICT_HANDLE(HLOCKEDLOG);

void SLogInitialize();
int32_t SLogIsInitialized();
void SLogDestroy();
int32_t SLogCreate(const char* filename, uint32_t flags, HSLOG* log);
void SLogClose(HSLOG log);
void SLogFlush(HSLOG log);
void SLogFlushAll();
void SLogGetDefaultDirectory(char* dirname, size_t dirnamesize);
void SLogSetDefaultDirectory(const char* dirname);
int32_t SLogSetAbsIndent(HSLOG log, int32_t indent);
int32_t SLogSetIndent(HSLOG log, int32_t deltaIndent);
void SLogVWrite(HSLOG log, const char* format, va_list arglist);
void SLogWrite(HSLOG log, const char* format, ...);

#endif
