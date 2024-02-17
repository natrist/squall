#ifndef STORM_LOG_HPP
#define STORM_LOG_HPP


#include "storm/Common.hpp"
#include "storm/String.hpp"


DECLARE_STRICT_HANDLE(HSLOG);
DECLARE_STRICT_HANDLE(HLOCKEDLOG);


void SLogInitialize();
int  SLogIsInitialized();
void SLogDestroy();
int  SLogCreate(const char* filename, uint32_t flags, HSLOG* log);
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
