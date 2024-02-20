#ifndef STORM_HANDLE_HPP
#define STORM_HANDLE_HPP

#if !defined(DECLARE_HANDLE)
#define DECLARE_HANDLE(name) \
    struct name##__;         \
    typedef struct name##__* name
#endif

#if !defined(DECLARE_STRICT_HANDLE)
#define DECLARE_STRICT_HANDLE(name) \
    typedef struct name##__ {       \
        int unused;                 \
    }* name
#endif

#if !defined(DECLARE_DERIVED_HANDLE)
#define DECLARE_DERIVED_HANDLE(name, base)      \
    typedef struct name##__ : public base##__ { \
        int unused;                             \
    }* name
#endif

#endif
