#ifndef STORM_COMMON_HPP
#define STORM_COMMON_HPP


#define DECLARE_STRICT_HANDLE(name) \
    typedef struct name##__ {       \
        int unused;                 \
    }* name

#define DECLARE_DERIVED_HANDLE(name, base)      \
    typedef struct name##__ : public base##__ { \
        int unused;                             \
    }* name


#endif
