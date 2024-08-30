#ifndef STORM_ERROR_MACRO_HPP
#define STORM_ERROR_MACRO_HPP

#define STORM_NO_ERROR 0x85100000

#define STORM_ERROR(id) (STORM_NO_ERROR | (id & 0xFFFF))

// assertions
#if defined(NDEBUG)
#define STORM_ASSERT(x)                          \
    (void)0
#else
#define STORM_ASSERT(x)                          \
    if (!(x)) {                                  \
        SErrPrepareAppFatal(__FILE__, __LINE__); \
        SErrDisplayAppFatal(#x);                 \
    }                                            \
    (void)0
#endif

#define STORM_VALIDATE(x, y, ...)                \
    if (!(x)) {                                  \
        SErrSetLastError(y);                     \
        return __VA_ARGS__;                      \
    }                                            \
    (void)0

#define STORM_VALIDATE_STRING(x, y, ...) STORM_VALIDATE(x && *x, y, __VA_ARGS__)

#define STORM_PANIC(...)                         \
    SErrPrepareAppFatal(__FILE__, __LINE__);     \
    SErrDisplayAppFatal(__VA_ARGS__);            \
    (void)0

#endif
