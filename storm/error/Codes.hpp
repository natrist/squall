#ifndef STORM_ERROR_CODES_HPP
#define STORM_ERROR_CODES_HPP

#if defined(WHOA_SYSTEM_WIN)
#include "storm/error/win/Codes.hpp"
#endif

#if defined(WHOA_SYSTEM_MAC) || defined(WHOA_SYSTEM_LINUX)
#include "storm/error/unix/Codes.hpp"
#endif

#include "storm/error/Macro.hpp"

#define STORM_ERROR_APPLICATION_FATAL STORM_ERROR(0x84)

#define STORM_COMMAND_ERROR_BAD_ARGUMENT         STORM_ERROR(0x65)
#define STORM_COMMAND_ERROR_NOT_ENOUGH_ARGUMENTS STORM_ERROR(0x6D)
#define STORM_COMMAND_ERROR_OPEN_FAILED          0x6E

#endif
