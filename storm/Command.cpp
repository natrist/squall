#include "storm/Command.hpp"
#include "storm/Error.hpp"
#include "storm/String.hpp"

#include "storm/Memory.hpp"

#include <bc/os/File.hpp>
#include <bc/os/CommandLine.hpp>

#include <cstring>
#include <algorithm>

STORM_LIST(CMDDEF) s_arglist;
STORM_LIST(CMDDEF) s_flaglist;

static int32_t s_addedoptional = 0;

const char* s_errorstr[] = {
    "Invalid argument: %s",
    "The syntax of the command is incorrect.",
    "Unable to open response file: %s"
};

static void GenerateError(CMDERRORCALLBACK errorcallback, uint32_t errorcode, const char* itemstring) {
    char errorstr[256] = {0};
    uint32_t strid = 0;

    switch (errorcode)  {
    case STORM_COMMAND_ERROR_BAD_ARGUMENT:
        strid = 0;
        break;
    case STORM_COMMAND_ERROR_NOT_ENOUGH_ARGUMENTS:
        strid = 1;
        break;
    case STORM_COMMAND_ERROR_OPEN_FAILED:
        strid = 2;
        break;
    default:
        return;
    }

    auto source = s_errorstr[strid];
    STORM_ASSERT(source);

    char buffer[256] = {0};
    SStrCopy(buffer, s_errorstr[strid], 256);

    if (strstr(buffer, "%s")) {
        SStrPrintf(buffer, 256, itemstring);
    } else {
        SStrCopy(errorstr, buffer, 256);

        if (errorstr[0]) {
            SStrPack(errorstr, "\n", 256);
        }
    }

    SErrSetLastError(errorcode);

    CMDERROR data;
    data.errorcode = errorcode;
    data.itemstr   = itemstring;
    data.errorstr  = errorstr;
    errorcallback(&data);
}

static CMDDEF* FindFlagDef(const char* string, CMDDEF* firstdef, int32_t minlength) {
    CMDDEF* bestptr = nullptr;
    int32_t match;
    auto strlength = SStrLen(string);
    auto bestlength = minlength - 1;

    for (auto def = firstdef; def; def = def->Next()) {
        if ((def->namelength > bestlength) && (def->namelength <= strlength)) {
            if (def->flags & STORM_COMMAND_EXTRA_CASE_SENSITIVE) {
                match = !SStrCmp(def->name, string, def->namelength);
            } else {
                match = !SStrCmpI(def->name, string, def->namelength);
            }

            if (match) {
                bestlength = def->namelength;
                bestptr = def;
            }
        }
    }

    return bestptr;
}


static void ConvertBool(CMDDEF* ptr, const char* string, int32_t* datachars) {
    int32_t set;

    if (*string == '-') {
        set = 0;
        *datachars = 1;
    } else if (*string == '+') {
        set = 1;
        *datachars = 1;
    } else if (STORM_COMMAND_GET_BOOL(ptr->flags) == STORM_COMMAND_BOOL_CLEAR) {
        set = 0;
    } else {
        set = 1;
    }

    auto notmask = ~ptr->setmask;

    ptr->currvalue &= notmask;
    if (set) {
        ptr->currvalue |= ptr->setvalue;
    }

    if (ptr->variableptr) {
        *(reinterpret_cast<uint32_t*>(ptr->variableptr)) &= notmask;

        if (set) {
            *(reinterpret_cast<uint32_t*>(ptr->variableptr)) &= ptr->setvalue;
        }
    }
}

static void ConvertNumber(CMDDEF* ptr, const char* string, int32_t* datachars) {
    char* endptr = nullptr;

    if (STORM_COMMAND_GET_NUM(ptr->flags) == STORM_COMMAND_NUM_SIGNED) {
        ptr->currvalue = static_cast<uint32_t>(strtol(string, &endptr, 0));
    } else {
        ptr->currvalue = static_cast<uint32_t>(strtoul(string, &endptr, 0));
    }

    if (endptr) {
        *datachars = endptr - string;
    } else {
        *datachars = SStrLen(string);
    }

    if (ptr->variableptr) {
        memcpy(ptr->variableptr, &ptr->currvaluestr, std::min(static_cast<uint32_t>(sizeof(ptr->currvalue)), ptr->variablebytes));
    }
}

static void ConvertString(CMDDEF* ptr, const char* string, int32_t* datachars) {
    *datachars = SStrLen(string);

    if (ptr->currvaluestr) {
        SMemFree(ptr->currvaluestr, __FILE__, __LINE__, 0);
    }

    auto size = SStrLen(string) + 1;
    auto ch = reinterpret_cast<char*>(SMemAlloc(size, __FILE__, __LINE__, 0));

    SStrCopy(ch, string, size);

    ptr->currvaluestr = ch;

    if (ptr->variableptr) {
        SStrCopy(reinterpret_cast<char*>(ptr->variableptr), string, ptr->variablebytes);
    }
}

static int32_t PerformConversion(CMDDEF* ptr, const char* string, int32_t* datachars) {
    CMDPARAMS params;

    *datachars = 0;

    switch (STORM_COMMAND_GET_TYPE(ptr->flags)) {
    case STORM_COMMAND_TYPE_BOOL:
        ConvertBool(ptr, string, datachars);
        break;
    case STORM_COMMAND_TYPE_NUMBER:
        ConvertNumber(ptr, string, datachars);
        break;
    case STORM_COMMAND_TYPE_STRING:
        ConvertString(ptr, string, datachars);
        break;
    default:
        return 0;
    }

    ptr->found = 1;

    if (ptr->callback) {
        params.flags = ptr->flags;
        params.id = ptr->id;
        params.name = ptr->name;
        params.variable = ptr->variableptr;
        params.setvalue = ptr->setvalue;
        params.setmask = ptr->setmask;
        params.unsignedvalue = ptr->currvalue;

        if (!ptr->callback(&params, string)) {
            return 0;
        }
    }

    for (int32_t flaglist = 0; flaglist < 2; flaglist++) {
        auto& list = flaglist ? s_flaglist : s_arglist;

        for (auto def = list.Head(); def; def = def->Next()) {
            if ((def->id == ptr->id) && (STORM_COMMAND_GET_TYPE(def->flags) == STORM_COMMAND_GET_TYPE(ptr->flags)) && (def != ptr)) {
                def->found = 1;

                if (STORM_COMMAND_GET_TYPE(def->flags) == STORM_COMMAND_TYPE_STRING) {
                    if (def->currvaluestr) {
                        SMemFree(def->currvaluestr, __FILE__, __LINE__, 0);
                    }

                    auto newlen = SStrLen(ptr->currvaluestr) + 1;
                    def->currvaluestr = reinterpret_cast<char*>(SMemAlloc(newlen, __FILE__, __LINE__, 0));
                    SStrCopy(def->currvaluestr, ptr->currvaluestr, newlen);
                } else {
                    def->currvalue = ptr->currvalue;
                }
            }
        }
    }

    return 1;
}

static int32_t ProcessCurrentFlag(const char* string, PROCESSING* processing, int32_t* datachars) {
    *datachars = 0;

    auto ptr = processing->ptr;
    processing->ptr = nullptr;

    int32_t currdatachars;

    while (ptr) {
        if (!PerformConversion(ptr, string, &currdatachars)) {
            return 0;
        }

        *datachars = std::max(*datachars, currdatachars);

        ptr = FindFlagDef(processing->name, ptr->Next(), processing->namelength);
    }

    return 1;
}

static int32_t ProcessFlags(const char* string, PROCESSING* processing, CMDERRORCALLBACK errorcallback) {
    char lastflag[256] = { 0 };

    while (string[0] != '\0') {
        int32_t datachars;
        CMDDEF* ptr = nullptr;
        auto strlength = SStrLen(string);
        auto lastflaglength = std::max(SStrLen(lastflag), size_t(1));

        while (lastflaglength--) {
            if (strlength + lastflaglength < 256) {
                SStrCopy(&lastflag[lastflaglength], string, 256);

                ptr = FindFlagDef(lastflag, s_flaglist.Head(), 0);

                if (ptr) {
                    lastflaglength = ptr->namelength;
                    lastflag[lastflaglength] = '\0';
                    break;
                }
            }
        }

        if (!ptr) {
            if (errorcallback) {
                GenerateError(errorcallback, STORM_COMMAND_ERROR_BAD_ARGUMENT, string);
            }

            return 0;
        }

        string += lastflaglength;

        processing->ptr = ptr;
        processing->namelength = lastflaglength;
        SStrCopy(processing->name, lastflag, 16);

        if (!string[0] && STORM_COMMAND_GET_TYPE(ptr->flags) != STORM_COMMAND_TYPE_BOOL) {
            return 1;
        }

        if (!ProcessCurrentFlag(string, processing, &datachars)) {
            return 0;
        }

        string += datachars;
    }

    return 1;
}

static int32_t ProcessFile(const char* filename, PROCESSING* processing, CMDDEF** nextarg, CMDEXTRACALLBACK extracallback, CMDERRORCALLBACK errorcallback);

static int32_t ProcessToken(const char* string, int32_t quoted, PROCESSING* processing, CMDDEF** nextarg, CMDEXTRACALLBACK extracallback, CMDERRORCALLBACK errorcallback) {
    if (string[0] == '@' && !quoted) {
        return ProcessFile(&string[1], processing, nextarg, extracallback, errorcallback);
    }

    int32_t datachars;

    // Changed because practically no one uses /forward /slash /prefixed /arguments
    // anymore, and passing any string argument that begins with / gets misinterpreted
    // Formerly:
    // if (SStrChr("-/", string[0]) != nullptr && !quoted) {
    if (SStrChr("-", string[0]) != nullptr && !quoted) {
        processing->ptr = nullptr;
        return ProcessFlags(string + 1, processing, errorcallback);
    }

    if (processing->ptr != nullptr) {
        return ProcessCurrentFlag(string, processing, &datachars);
    }

    if (*nextarg) {
        if (!PerformConversion(*nextarg, string, &datachars)) {
            return 0;
        }
        *nextarg = (*nextarg)->Next();
        return 1;
    } else {
        if (extracallback) {
            return extracallback(string);
        }

        if (errorcallback) {
            GenerateError(errorcallback, STORM_COMMAND_ERROR_OPEN_FAILED, string);
        }
    }

    return 0;
}

static int32_t ProcessString(const char** stringptr, PROCESSING* processing, CMDDEF** nextarg, CMDEXTRACALLBACK extracallback, CMDERRORCALLBACK errorcallback) {
    char buffer[256] = {0};
    int32_t quoted = 0;

    while (**stringptr != '\0') {
        auto nextptr = *stringptr;

        SStrTokenize(&nextptr, buffer, sizeof(buffer), STORM_COMMAND_WHITESPACE_CHARS, &quoted);

        if (!ProcessToken(buffer, quoted, processing, nextarg, extracallback, errorcallback)) {
            break;
        }

        *stringptr = nextptr;
    }

    return **stringptr == '\0';
}

static int32_t ProcessFile(const char* filename, PROCESSING* processing, CMDDEF** nextarg, CMDEXTRACALLBACK extracallback, CMDERRORCALLBACK errorcallback) {
    // TODO
    auto file = OsCreateFile(filename, OS_GENERIC_READ, OS_FILE_SHARE_READ, OS_OPEN_EXISTING, OS_FILE_FLAG_SEQUENTIAL_SCAN, 0);

    if (!file) {
        if (errorcallback) {
            GenerateError(errorcallback, STORM_COMMAND_ERROR_OPEN_FAILED, filename);
        }
        return false;
    }

    auto size = OsGetFileSize(file);

    auto buffer = reinterpret_cast<char*>(SMemAlloc(size + 1, __FILE__, __LINE__, 0));

    size_t bytesread = 0;
    OsReadFile(file, buffer, size, &bytesread);

    OsCloseFile(file);

    buffer[bytesread] = '\0';

    const char* curr = buffer;
    auto status = ProcessString(&curr, processing, nextarg, extracallback, errorcallback);

    SMemFree(buffer, __FILE__, __LINE__, 0);

    return status;
}

int32_t SCmdRegisterArgument(uint32_t flags, uint32_t id, const char* name, void* variableptr, uint32_t variablebytes, uint32_t setvalue, uint32_t setmask, CMDPARAMSCALLBACK callback) {
    if (name == nullptr) {
        name = "";
    }

    auto namelength = SStrLen(name);

    STORM_VALIDATE(namelength < 16, ERROR_INVALID_PARAMETER, 0);
    STORM_VALIDATE((!variablebytes) || variableptr, ERROR_INVALID_PARAMETER, 0);
    STORM_VALIDATE(((STORM_COMMAND_GET_ARG(flags) != STORM_COMMAND_ARG_REQUIRED) || !s_addedoptional), ERROR_INVALID_PARAMETER, 0);
    STORM_VALIDATE((STORM_COMMAND_GET_ARG(flags) != STORM_COMMAND_ARG_FLAGGED) || (namelength > 0), ERROR_INVALID_PARAMETER, 0);
    STORM_VALIDATE((STORM_COMMAND_GET_TYPE(flags) != STORM_COMMAND_TYPE_BOOL) || (!variableptr) || (variablebytes == sizeof(uint32_t)), ERROR_INVALID_PARAMETER, 0);

    // If argument is flagged, it goes in the flag list
    auto listptr = &s_arglist;
    if (STORM_COMMAND_GET_ARG(flags) == STORM_COMMAND_ARG_FLAGGED) {
        listptr = &s_flaglist;
    }
    auto cmd = listptr->NewNode(2, 0, 0);

    SStrCopy(cmd->name, name, sizeof(cmd->name));
    cmd->id = id;
    cmd->namelength = namelength;
    cmd->variableptr = variableptr;
    cmd->variablebytes = variablebytes;
    cmd->flags = flags;
    cmd->setvalue = setvalue;
    cmd->setmask = setmask;
    cmd->callback = callback;
    if ((STORM_COMMAND_GET_TYPE(flags) == STORM_COMMAND_TYPE_BOOL) && (STORM_COMMAND_GET_BOOL(flags) == STORM_COMMAND_BOOL_CLEAR)) {
        cmd->currvalue = setvalue;
    } else {
        cmd->currvalue = 0;
    }

    if (STORM_COMMAND_GET_ARG(flags) == STORM_COMMAND_ARG_OPTIONAL) {
        s_addedoptional = 1;
    }

    return 1;
}

int32_t SCmdRegisterArgList(ARGLIST* listptr, uint32_t numargs) {
    STORM_VALIDATE(listptr, ERROR_INVALID_PARAMETER, 0);

    for (int32_t i = 0; i < numargs; i++) {
        if (!SCmdRegisterArgument(listptr->flags, listptr->id, listptr->name, 0, 0, 1, 0xFFFFFFFF, listptr->callback)) {
            return 1;
        }

        listptr++;
    }

    return 1;
}

int32_t SCmdProcess(const char* cmdline, int32_t skipprogname, CMDEXTRACALLBACK extracallback, CMDERRORCALLBACK errorcallback) {
    STORM_VALIDATE(cmdline, ERROR_INVALID_PARAMETER, 0);

    if (skipprogname) {
        SStrTokenize(&cmdline, nullptr, 0, STORM_COMMAND_WHITESPACE_CHARS, nullptr);
    }

    PROCESSING processing;
    memset(&processing, 0, sizeof(processing));

    auto nextarg = s_arglist.Head();

    if (!ProcessString(&cmdline, &processing, &nextarg, extracallback, errorcallback)) {
        return 0;
    }

    int32_t allfilled = 1;

    while (nextarg && allfilled) {
        if (STORM_COMMAND_GET_ARG(nextarg->flags) == STORM_COMMAND_ARG_REQUIRED) {
            allfilled = 0;
        } else {
            nextarg = nextarg->Next();
        }
    }

    if (errorcallback && !allfilled) {
        GenerateError(errorcallback, STORM_COMMAND_ERROR_NOT_ENOUGH_ARGUMENTS, "");
    }

    return 1;
}

int32_t SCmdProcessCommandLine(CMDEXTRACALLBACK extracallback, CMDERRORCALLBACK errorcallback) {
    auto cmdline = OsGetCommandLine();

    return SCmdProcess(cmdline, 1, extracallback, errorcallback);
}

uint32_t SCmdGetNum(uint32_t id) {
    for (int32_t flaglist = 0; flaglist <= 1; flaglist++) {
        auto& list = flaglist ? s_flaglist : s_arglist;

        for (auto def = list.Head(); def; def = def->Next()) {
            if (def->id == id) {
                return def->currvalue;
            }
        }
    }

    return 0;
}

int32_t SCmdGetBool(uint32_t id) {
    return SCmdGetNum(id) != 0;
}

int32_t SCmdGetString(uint32_t id, char* buffer, uint32_t bufferchars) {
    if (buffer) {
        *buffer = '\0';
    }

    STORM_VALIDATE(buffer, ERROR_INVALID_PARAMETER, 0);
    STORM_VALIDATE(bufferchars, ERROR_INVALID_PARAMETER, 0);

    for (int32_t flaglist = 0; flaglist <= 1; flaglist++) {
        auto& list = flaglist ? s_flaglist : s_arglist;

        for (auto def = list.Head(); def; def = def->Next()) {
            if (def->id == id) {
                if (def->currvaluestr) {
                    SStrCopy(buffer, def->currvaluestr, bufferchars);
                }

                return 1;
            }
        }
    }

    return 0;
}
