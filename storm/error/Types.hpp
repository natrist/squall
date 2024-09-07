#ifndef STORM_ERROR_TYPES_HPP
#define STORM_ERROR_TYPES_HPP

#include <cstdint>

struct APPFATINFO {
    const char* filename;
    int32_t linenumber;
    uintptr_t threadId;
};

#endif
