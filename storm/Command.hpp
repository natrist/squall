#ifndef STORM_COMMAND_HPP
#define STORM_COMMAND_HPP

#include "storm/List.hpp"

#include <cstdint>

#define STORM_COMMAND_ERROR_BAD_ARGUMENT         0x85100065
#define STORM_COMMAND_ERROR_NOT_ENOUGH_ARGUMENTS 0x8510006D
#define STORM_COMMAND_ERROR_OPEN_FAILED          0x6E

#define STORM_COMMAND_WHITESPACE_CHARS " ,;\"\t\n\r\x1A"

#define STORM_COMMAND_EXTRA_CASE_SENSITIVE (1 << 8)

// Type flags
#define STORM_COMMAND_TYPE_BOOL   (0 << 16)
#define STORM_COMMAND_TYPE_NUMBER (1 << 16)
#define STORM_COMMAND_TYPE_STRING (2 << 16)
#define STORM_COMMAND_TYPE_MASK   (STORM_COMMAND_TYPE_BOOL | STORM_COMMAND_TYPE_NUMBER | STORM_COMMAND_TYPE_STRING)

// Bool value
#define STORM_COMMAND_BOOL_SET   0
#define STORM_COMMAND_BOOL_CLEAR 1
#define STORM_COMMAND_BOOL_MASK  (STORM_COMMAND_BOOL_CLEAR | STORM_COMMAND_BOOL_SET)

// Numeric value
#define STORM_COMMAND_NUM_UNSIGNED 0
#define STORM_COMMAND_NUM_SIGNED   1
#define STORM_COMMAND_NUM_MASK     (STORM_COMMAND_NUM_UNSIGNED | STORM_COMMAND_NUM_SIGNED)

// Argument
#define STORM_COMMAND_ARG_FLAGGED  (0 << 24)
#define STORM_COMMAND_ARG_OPTIONAL (1 << 24)
#define STORM_COMMAND_ARG_REQUIRED (2 << 24)
#define STORM_COMMAND_ARG_MASK     (STORM_COMMAND_ARG_FLAGGED | STORM_COMMAND_ARG_OPTIONAL | STORM_COMMAND_ARG_REQUIRED)

// Getters
#define STORM_COMMAND_GET_TYPE(u32) (u32 & STORM_COMMAND_TYPE_MASK)
#define STORM_COMMAND_GET_ARG(u32)  (u32 & STORM_COMMAND_ARG_MASK)
#define STORM_COMMAND_GET_BOOL(u32) (u32 & STORM_COMMAND_BOOL_MASK)
#define STORM_COMMAND_GET_NUM(u32)  (u32 & STORM_COMMAND_NUM_MASK)

class CMDERROR;
class CMDPARAMS;

// Callback types
typedef int32_t (*CMDEXTRACALLBACKFCN)(const char*);
typedef int32_t (*CMDERRORCALLBACKFCN)(CMDERROR*);
typedef int32_t (*CMDPARAMSCALLBACKFCN)(CMDPARAMS*, const char*);

// Details a command line argument
class ARGLIST {
    public:
        uint32_t             flags;
        uint32_t             id;
        const char*          name;
        CMDPARAMSCALLBACKFCN callback;
};

// Parameters passed to argument callback
class CMDPARAMS {
    public:
        uint32_t        flags;
        uint32_t        id;
        const char*     name;
        void*           variable;
        uint32_t        setvalue;
        uint32_t        setmask;
        union {
            int32_t     boolvalue;
            int32_t     signedvalue;
            uint32_t    unsignedvalue;
            const char* stringvalue;
        };
};

// Command definitions
class CMDDEF : public TSLinkedNode<CMDDEF> {
    public:
        uint32_t             flags;
        uint32_t             id;
        char                 name[16];
        int32_t              namelength;
        uint32_t             setvalue;
        uint32_t             setmask;
        void*                variableptr;
        uint32_t             variablebytes;
        CMDPARAMSCALLBACKFCN callback;
        int32_t              found;
        union {
            uint32_t         currvalue;
            char*            currvaluestr;
        };
};

class CMDERROR {
    public:
        uint32_t    errorcode;
        const char* itemstr;
        const char* errorstr;
};

class PROCESSING {
    public:
        CMDDEF* ptr;
        char    name[16];
        int32_t namelength;
};

int32_t  SCmdRegisterArgument(uint32_t flags, uint32_t id, const char* name, void* variableptr, uint32_t variablebytes, uint32_t setvalue, uint32_t setmask, CMDPARAMSCALLBACKFCN callback);

int32_t  SCmdRegisterArgList(ARGLIST* listptr, uint32_t numargs);

int32_t  SCmdProcessCommandLine(CMDEXTRACALLBACKFCN extracallback, CMDERRORCALLBACKFCN errorcallback);

int32_t  SCmdProcess(const char* cmdline, int32_t skipprogname, CMDEXTRACALLBACKFCN extracallback, CMDERRORCALLBACKFCN errorcallback);

uint32_t SCmdGetNum(uint32_t id);

int32_t  SCmdGetBool(uint32_t id);

int32_t  SCmdGetString(uint32_t id, char* buffer, uint32_t bufferchars);

#endif
