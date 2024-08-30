#ifndef STORM_ERROR_HPP
#define STORM_ERROR_HPP

#include <cstdint>

#include "storm/error/Macro.hpp"
#include "storm/error/Codes.hpp"

[[noreturn]] void SErrDisplayAppFatal(const char* format, ...);

int32_t SErrDisplayError(uint32_t errorcode, const char* filename, int32_t linenumber, const char* description, int32_t recoverable, uint32_t exitcode, uint32_t a7);

int32_t SErrDisplayErrorFmt(uint32_t errorcode, const char* filename, int32_t linenumber, int32_t recoverable, uint32_t exitcode, const char* format, ...);

void SErrPrepareAppFatal(const char* filename, int32_t linenumber);

void SErrSetLastError(uint32_t errorcode);

uint32_t SErrGetLastError();

#endif
