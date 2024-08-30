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

#endif
