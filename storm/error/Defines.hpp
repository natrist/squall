#ifndef STORM_ERROR_DEFINES_HPP
#define STORM_ERROR_DEFINES_HPP

// error codes

#if defined(WHOA_SYSTEM_WIN)

#include <winerror.h>

#else

#define ERROR_SUCCESS                   0x0000
#define ERROR_INVALID_FUNCTION          0x0001
#define ERROR_FILE_NOT_FOUND            0x0002
#define ERROR_PATH_NOT_FOUND            0x0003
#define ERROR_TOO_MANY_OPEN_FILES       0x0004
#define ERROR_ACCESS_DENIED             0x0005
#define ERROR_INVALID_HANDLE            0x0006
#define ERROR_ARENA_TRASHED             0x0007
#define ERROR_NOT_ENOUGH_MEMORY         0x0008
#define ERROR_INVALID_BLOCK             0x0009
#define ERROR_BAD_ENVIRONMENT           0x000A
#define ERROR_BAD_FORMAT                0x000B
#define ERROR_INVALID_ACCESS            0x000C
#define ERROR_INVALID_DATA              0x000D
#define ERROR_INVALID_DRIVE             0x000F
#define ERROR_CURRENT_DIRECTORY         0x0010
#define ERROR_NOT_SAME_DEVICE           0x0011
#define ERROR_NO_MORE_FILES             0x0012
#define ERROR_WRITE_PROTECT             0x0013
#define ERROR_BAD_UNIT                  0x0014
#define ERROR_NOT_READY                 0x0015
#define ERROR_BAD_COMMAND               0x0016
#define ERROR_CRC                       0x0017
#define ERROR_BAD_LENGTH                0x0018
#define ERROR_SEEK                      0x0019
#define ERROR_NOT_DOS_DISK              0x001A
#define ERROR_SECTOR_NOT_FOUND          0x001B
#define ERROR_OUT_OF_PAPER              0x001C
#define ERROR_WRITE_FAULT               0x001D
#define ERROR_READ_FAULT                0x001E
#define ERROR_GEN_FAILURE               0x001F
#define ERROR_SHARING_VIOLATION         0x0020
#define ERROR_LOCK_VIOLATION            0x0021
#define ERROR_WRONG_DISK                0x0022
#define ERROR_FCB_UNAVAILABLE           0x0023
#define ERROR_SHARING_BUFFER_EXCEEDED   0x0024
#define ERROR_NOT_SUPPORTED             0x0032
#define ERROR_FILE_EXISTS               0x0050
#define ERROR_DUP_FCB                   0x0051
#define ERROR_CANNOT_MAKE               0x0052
#define ERROR_FAIL_I24                  0x0053
#define ERROR_OUT_OF_STRUCTURES         0x0054
#define ERROR_ALREADY_ASSIGNED          0x0055
#define ERROR_INVALID_PASSWORD          0x0056
#define ERROR_INVALID_PARAMETER         0x0057
#define ERROR_NET_WRITE_FAULT           0x0058
#define ERROR_NO_PROC_SLOTS             0x0059
#define ERROR_NOT_FROZEN                0x005A
#define ERR_TSTOVFL                     0x005B
#define ERR_TSTDUP                      0x005C
#define ERROR_NO_ITEMS                  0x005D
#define ERROR_INTERRUPT                 0x005F
#define ERROR_TOO_MANY_SEMAPHORES       0x0064
#define ERROR_EXCL_SEM_ALREADY_OWNED    0x0065
#define ERROR_SEM_IS_SET                0x0066
#define ERROR_TOO_MANY_SEM_REQUESTS     0x0067
#define ERROR_INVALID_AT_INTERRUPT_TIME 0x0068
#define ERROR_SEM_OWNER_DIED            0x0069
#define ERROR_SEM_USER_LIMIT            0x006A
#define ERROR_DISK_CHANGE               0x006B
#define ERROR_DRIVE_LOCKED              0x006C
#define ERROR_BROKEN_PIPE               0x006D
#define ERROR_OPEN_FAILED               0x006E
#define ERROR_BUFFER_OVERFLOW           0x006F
#define ERROR_DISK_FULL                 0x0070
#define ERROR_NO_MORE_SEARCH_HANDLES    0x0071
#define ERROR_INVALID_TARGET_HANDLE     0x0072
#define ERROR_PROTECTION_VIOLATION      0x0073
#define ERROR_VIOKBD_REQUEST            0x0074
#define ERROR_INVALID_CATEGORY          0x0075
#define ERROR_INVALID_VERIFY_SWITCH     0x0076
#define ERROR_BAD_DRIVER_LEVEL          0x0077
#define ERROR_CALL_NOT_IMPLEMENTED      0x0078
#define ERROR_SEM_TIMEOUT               0x0079
#define ERROR_INSUFFICIENT_BUFFER       0x007A
#define ERROR_INVALID_NAME              0x007B
#define ERROR_INVALID_LEVEL             0x007C
#define ERROR_NO_VOLUME_LABEL           0x007D
#define ERROR_MOD_NOT_FOUND             0x007E
#define ERROR_PROC_NOT_FOUND            0x007F
#define ERROR_WAIT_NO_CHILDREN          0x0080
#define ERROR_CHILD_NOT_COMPLETE        0x0081
#define ERROR_DIRECT_ACCESS_HANDLE      0x0082
#define ERROR_NEGATIVE_SEEK             0x0083
#define ERROR_SEEK_ON_DEVICE            0x0084
#define ERROR_IS_JOIN_TARGET            0x0085
#define ERROR_IS_JOINED                 0x0086
#define ERROR_IS_SUBSTED                0x0087
#define ERROR_NOT_JOINED                0x0088
#define ERROR_NOT_SUBSTED               0x0089
#define ERROR_JOIN_TO_JOIN              0x008A
#define ERROR_SUBST_TO_SUBST            0x008B
#define ERROR_JOIN_TO_SUBST             0x008C
#define ERROR_SUBST_TO_JOIN             0x008D
#define ERROR_BUSY_DRIVE                0x008E
#define ERROR_SAME_DRIVE                0x008F
#define ERROR_DIR_NOT_ROOT              0x0090
#define ERROR_DIR_NOT_EMPTY             0x0091
#define ERROR_IS_SUBST_PATH             0x0092
#define ERROR_IS_JOIN_PATH              0x0093
#define ERROR_PATH_BUSY                 0x0094
#define ERROR_IS_SUBST_TARGET           0x0095
#define ERROR_SYSTEM_TRACE              0x0096
#define ERROR_INVALID_EVENT_COUNT       0x0097
#define ERROR_TOO_MANY_MUXWAITERS       0x0098
#define ERROR_INVALID_LIST_FORMAT       0x0099
#define ERROR_LABEL_TOO_LONG            0x009A
#define ERROR_TOO_MANY_TCBS             0x009B
#define ERROR_SIGNAL_REFUSED            0x009C
#define ERROR_DISCARDED                 0x009D
#define ERROR_NOT_LOCKED                0x009E
#define ERROR_BAD_THREADID_ADDR         0x009F
#define ERROR_BAD_ARGUMENTS             0x00A0
#define ERROR_BAD_PATHNAME              0x00A1
#define ERROR_SIGNAL_PENDING            0x00A2
#define ERROR_UNCERTAIN_MEDIA           0x00A3
#define ERROR_MAX_THRDS_REACHED         0x00A4
#define ERROR_MONITORS_NOT_SUPPORTED    0x00A5
#define ERROR_INVALID_SEGMENT_NUMBER    0x00B4
#define ERROR_INVALID_CALLGATE          0x00B5
#define ERROR_INVALID_ORDINAL           0x00B6
#define ERROR_ALREADY_EXISTS            0x00B7
#define ERROR_NO_CHILD_PROCESS          0x00B8
#define ERROR_CHILD_ALIVE_NOWAIT        0x00B9
#define ERROR_INVALID_FLAG_NUMBER       0x00BA
#define ERROR_SEM_NOT_FOUND             0x00BB
#define ERROR_INVALID_STARTING_CODESEG  0x00BC
#define ERROR_INVALID_STACKSEG          0x00BD
#define ERROR_INVALID_MODULETYPE        0x00BE
#define ERROR_INVALID_EXE_SIGNATURE     0x00BF
#define ERROR_EXE_MARKED_INVALID        0x00C0
#define ERROR_BAD_EXE_FORMAT            0x00C1
#define ERROR_ITERATED_DATA_EXCEEDS_64k 0x00C2
#define ERROR_INVALID_MINALLOCSIZE      0x00C3
#define ERROR_DYNLINK_FROM_INVALID_RING 0x00C4
#define ERROR_IOPL_NOT_ENABLED          0x00C5
#define ERROR_INVALID_SEGDPL            0x00C6
#define ERROR_AUTODATASEG_EXCEEDS_64k   0x00C7
#define ERROR_RING2SEG_MUST_BE_MOVABLE  0x00C8
#define ERROR_RELOC_CHAIN_XEEDS_SEGLIM  0x00C9
#define ERROR_INFLOOP_IN_RELOC_CHAIN    0x00CA
#define ERROR_ENVVAR_NOT_FOUND          0x00CB
#define ERROR_NOT_CURRENT_CTRY          0x00CC
#define ERROR_NO_SIGNAL_SENT            0x00CD
#define ERROR_FILENAME_EXCED_RANGE      0x00CE
#define ERROR_RING2_STACK_IN_USE        0x00CF
#define ERROR_META_EXPANSION_TOO_LONG   0x00D0
#define ERROR_INVALID_SIGNAL_NUMBER     0x00D1
#define ERROR_THREAD_1_INACTIVE         0x00D2
#define ERROR_INFO_NOT_AVAIL            0x00D3
#define ERROR_LOCKED                    0x00D4
#define ERROR_BAD_DYNALINK              0x00D5
#define ERROR_TOO_MANY_MODULES          0x00D6
#define ERROR_NESTING_NOT_ALLOWED       0x00D7
#define ERROR_INVALID_TOKEN             0x013B

#endif

#define STORM_NO_ERROR 0x85100000

#define STORM_ERROR(id) (STORM_NO_ERROR | (id & 0xFFFF))

#define STORM_ERROR_APPLICATION_FATAL STORM_ERROR(0x84)

#define STORM_COMMAND_ERROR_BAD_ARGUMENT         STORM_ERROR(0x65)
#define STORM_COMMAND_ERROR_NOT_ENOUGH_ARGUMENTS STORM_ERROR(0x6D)

// assertions

#if defined(WHOA_BUILD_ASSERTIONS)

#define STORM_ASSERT(x)                          \
    if (!(x)) {                                  \
        SErrPrepareAppFatal(__FILE__, __LINE__); \
        SErrDisplayAppFatal(#x);                 \
    }                                            \
    (void)0

#else

#define STORM_ASSERT(x) \
    (void)0

#endif

// validation

#if defined(NDEBUG)

// (release) check all validation cases and return if any is false without displaying an error

#define STORM_VALIDATE_BEGIN \
    { \
        bool intrn_valresult = true; \
        do {
#define STORM_VALIDATE(x) \
        if (!(x)) { \
            intrn_valresult &= !!(x); \
            break; \
        }
#define STORM_VALIDATE_END \
        } while(0); \
        if (!intrn_valresult) { \
            SErrSetLastError(ERROR_INVALID_PARAMETER); \
            return 0; \
        } \
    }
#define STORM_VALIDATE_END_VOID \
        } while(0); \
        if (!intrn_valresult) { \
            SErrSetLastError(ERROR_INVALID_PARAMETER); \
            return; \
        } \
    }
#else

// (debug) display any failed validation as you would a failed assertion

#define STORM_VALIDATE_BEGIN \
    do {
#define STORM_VALIDATE(x)                            \
        if (!(x)) {                                  \
            SErrPrepareAppFatal(__FILE__, __LINE__); \
            SErrDisplayAppFatal(#x);                 \
        }                                            \
        (void)0
#define STORM_VALIDATE_END \
    } while (0)
#define STORM_VALIDATE_END_VOID \
    } while (0)

#endif

#endif
