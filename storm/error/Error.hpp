#ifndef STORM_ERROR_ERROR_HPP
#define STORM_ERROR_ERROR_HPP

#include <cstdint>

struct APPFATINFO {
    const char *filename;
    int32_t linenumber;
    uintptr_t threadId;
};

extern uint32_t s_lasterror;
extern APPFATINFO s_appFatInfo;

#endif
