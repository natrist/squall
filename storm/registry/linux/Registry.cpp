#include "storm/registry/Registry.hpp"
#include "bc/file/Close.hpp"
#include "bc/file/Defines.hpp"
#include "bc/file/GetFileInfo.hpp"
#include "bc/file/GetPos.hpp"
#include "bc/file/Types.hpp"
#include "bc/memory/Storm.hpp"
#include "storm/String.hpp"
#include "storm/Error.hpp"
#include "storm/List.hpp"
#include "bc/File.hpp"
#include "bc/Memory.hpp"
#include "storm/Thread.hpp"

#include <climits>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <algorithm>

struct RegistryEntry : public TSLinkedNode<RegistryEntry> {
    char* name = nullptr;
    char* value = nullptr;

    bool ParseLine(const char* line) {
        char buffer[STORM_MAX_PATH];
        auto b = buffer;
        auto end = buffer + sizeof(buffer);
        while (b < end && *line && *line != '=') {
            *b++ = *line++;
        }
        if (*line++ != '=') {
            return false;
        }
        if (b >= end) {
            return false;
        }
        *b = '\0';
        this->name = SStrDupA(buffer, __FILE__, __LINE__);

        b = buffer;
        while (b < end && *line) {
            auto c = *line++;
            if (c == '\\') {
                if (*line == 'n') {
                    *b++ = '\n';
                } else if (*line == '\\') {
                    *b++ = '\\';
                } else {
                    *b++ = *line;
                }
                line++;
            } else {
                *b++ = c;
            }
        }
        if (b >= end) {
            return false;
        }
        *b = '\0';

        this->value = SStrDupA(buffer, __FILE__, __LINE__);

        return true;
    }

    ~RegistryEntry() {
        if (this->name) {
            SMemFree(this->name, __FILE__, __LINE__, 0x0);
        }

        if (this->value) {
            SMemFree(this->value, __FILE__, __LINE__, 0x0);
        }
    }
};

static int32_t s_initregistry = 0;
static STORM_LIST(RegistryEntry) s_registry;
static SCritSect s_lockregistry;

Blizzard::File::StreamRecord* RegistryFileCreate(int32_t mode) {
    const char* basedir = getenv("XDG_CONFIG_HOME");
    if (!basedir) {
        basedir = getenv("HOME");
        if (!basedir) {
            basedir = ".";
        }
    }

    char name[PATH_MAX];
    *name = '\0';
    SStrPack(name, basedir, sizeof(name));
    SStrPack(name, "/.whoa", sizeof(name));

    if (!Blizzard::File::IsDirectory(name)) {
        if (!Blizzard::File::CreateDirectory(name, false)) {
            return nullptr;
        }
    }

    SStrPack(name, "/registry.txt", sizeof(name));

    Blizzard::File::StreamRecord* file;
    if (!Blizzard::File::Open(name, mode, file)) {
        return nullptr;
    }
    return file;
}

int32_t RegistryFileWriteEntry(Blizzard::File::StreamRecord* file, RegistryEntry* entry) {
    char valuebuffer[STORM_REGISTRY_MAX_VALUE];
    auto source = reinterpret_cast<const char*>(entry->value);
    auto dest = reinterpret_cast<char*>(valuebuffer);
    auto sourceend = source + SStrLen(entry->value);
    auto destend = dest + sizeof(valuebuffer);

    while (dest < destend && source < sourceend) {
        auto c = *source++;
        if (c == '\n' && (dest+1) < destend) {
            *dest++ = '\\';
            *dest++ = 'n';
        } else {
            *dest++ = c;
        }
    }
    valuebuffer[std::min(sizeof(valuebuffer)-1, static_cast<size_t>(dest-valuebuffer))] = '\0';

    char linebuffer[STORM_REGISTRY_MAX_PATH + STORM_REGISTRY_MAX_VALUE + 2];
    SStrPrintf(linebuffer, sizeof(linebuffer), "%s=%s\n", entry->name, valuebuffer);

    auto length = SStrLen(linebuffer);
    return Blizzard::File::Write(file, linebuffer, length);
}

int32_t RegistryFileReadLine(Blizzard::File::StreamRecord* file, char* buffer, int32_t buffersize) {
    int64_t start;
    if (!Blizzard::File::GetPos(file, start)) {
        return false;
    }

    auto size = static_cast<int64_t>(Blizzard::File::GetFileInfo(file)->size);
    if (start == size) {
        return 0;
    }

    auto count = static_cast<int32_t>(std::min(static_cast<int64_t>(buffersize), size-start));
    if (!Blizzard::File::Read(file, buffer, &count)) {
        return 0;
    }

    auto p = buffer;
    auto end = buffer + std::min(buffersize-1, count);
    while (p < end && *p && *p != '\n') {
        p++;
    }
    *p = '\0';


    auto nextline = start + static_cast<int64_t>(p-buffer) + 1;

    Blizzard::File::SetPos(file, nextline, BC_FILE_SEEK_START);

    return 1;
}

void RegistryInit() {
    s_initregistry = true;

    auto file = RegistryFileCreate(Blizzard::File::Mode::mustexist|Blizzard::File::Mode::read);
    if (!file) {
        return;
    }

    char buffer[STORM_REGISTRY_MAX_PATH + STORM_REGISTRY_MAX_VALUE + 2];
    while (RegistryFileReadLine(file, buffer, sizeof(buffer))) {
        if (*buffer) {
            auto entry = s_registry.NewNode(1, 0, 0);

            if (!entry->ParseLine(buffer)) {
                s_registry.DeleteNode(entry);
            }
        }
    }

    Blizzard::File::Close(file);

}

void RegistryFlush(bool remove) {
    using Mode = Blizzard::File::Mode;
    auto file = RegistryFileCreate(Mode::create|Mode::write|Mode::truncate);
    if (!file) {
        return;
    }

    auto entry = s_registry.Head();
    while (entry) {
        auto curentry = entry;
        if (!RegistryFileWriteEntry(file, curentry)) {
            break;
        }
        entry = entry->Next();
        if (remove) {
            s_registry.DeleteNode(curentry);
        }
    }

    Blizzard::File::Close(file);
}

void RegistryShutdown() {
    s_initregistry = false;
    RegistryFlush(true);
}

void BuildFullKeyName(const char* keyname, const char* valuename, char* buffer, uint32_t buffersize) {
    char key[STORM_MAX_PATH];
    auto k = key;
    auto keyend = (key + sizeof(key)) - 1;
    while (k < keyend && *keyname) {
        *k = *keyname == '\\' ? ':' : *keyname;
        k++;
        keyname++;
    }
    *k = '\0';
    SStrPrintf(buffer, buffersize, "%s:%s", key, valuename);
}

int32_t InternalLoadEntry(const char* keyname, const char* valuename, uint32_t flags, char* buffer, uint32_t bytes, uint32_t* bytesread) {
    s_lockregistry.Enter();
    if (!s_initregistry) {
        RegistryInit();
    }

    char fullkeyname[STORM_MAX_PATH];
    *fullkeyname = '\0';
    *bytesread = 0;

    BuildFullKeyName(keyname, valuename, fullkeyname, STORM_MAX_PATH);

    for (auto entry = s_registry.Head(); entry; entry = entry->Next()) {
        if (!SStrCmpI(fullkeyname, entry->name, STORM_MAX_PATH)) {
            auto valuelength = static_cast<uint32_t>(SStrLen(entry->value));
            *bytesread = std::min(valuelength, bytes);
            SStrCopy(buffer, entry->value, bytes);
            s_lockregistry.Leave();
            return 1;
        }
    }
    s_lockregistry.Leave();

    return 0;
}

int32_t InternalSaveEntry(const char* keyname, const char* valuename, uint32_t flags, const char* value) {
    s_lockregistry.Enter();

    if (!s_initregistry) {
        RegistryInit();
    }

    char fullkeyname[STORM_MAX_PATH];
    *fullkeyname = '\0';
    BuildFullKeyName(keyname, valuename, fullkeyname, STORM_MAX_PATH);

    auto entry = s_registry.Head();
    while (entry) {
        if (!SStrCmpI(fullkeyname, entry->name, STORM_MAX_PATH)) {
            auto curentry = entry;
            entry = entry->Next();
            s_registry.DeleteNode(curentry);
        } else {
            entry = entry->Next();
        }
    }

    entry = s_registry.NewNode(1, 0, 0);
    if (!entry) {
        return 0;

    }

    entry->name  = SStrDupA(fullkeyname, __FILE__, __LINE__);
    entry->value = SStrDupA(value, __FILE__, __LINE__);

    if (flags & STORM_REGISTRY_FLUSH_KEY) {
        RegistryFlush(false);
    }

    s_lockregistry.Leave();

    return 1;
}

int32_t SRegLoadString(const char* keyname, const char* valuename, uint32_t flags, char* buffer, uint32_t buffersize) {
    STORM_VALIDATE_BEGIN;
    STORM_VALIDATE(keyname);
    STORM_VALIDATE(*keyname);
    STORM_VALIDATE(valuename);
    STORM_VALIDATE(*valuename);
    STORM_VALIDATE(buffer);
    STORM_VALIDATE_END;

    uint32_t bytesread;
    if (!InternalLoadEntry(keyname, valuename, flags, buffer, buffersize, &bytesread)) {
        return 0;
    }

    buffer[std::min(buffersize - 1, bytesread)] = '\0';
    return 1;
}

int32_t SRegLoadValue(const char* keyname, const char* valuename, uint32_t flags, uint32_t* value) {
    STORM_VALIDATE_BEGIN;
    STORM_VALIDATE(keyname);
    STORM_VALIDATE(*keyname);
    STORM_VALIDATE(valuename);
    STORM_VALIDATE(*valuename);
    STORM_VALIDATE(value);
    STORM_VALIDATE_END;

    char valuestring[11];

    uint32_t datatype;
    uint32_t bytesread;
    if (!InternalLoadEntry(keyname, valuename, flags, valuestring, sizeof(valuestring), &bytesread)) {
        return 0;
    }

    *value = SStrToUnsigned(valuestring);
    return 1;
}

int32_t SRegSaveString(const char* keyname, const char* valuename, uint32_t flags, const char* string) {
    STORM_VALIDATE_BEGIN;
    STORM_VALIDATE(keyname);
    STORM_VALIDATE(*keyname);
    STORM_VALIDATE(valuename);
    STORM_VALIDATE(*valuename);
    STORM_VALIDATE(string);
    STORM_VALIDATE_END;

    if (!InternalSaveEntry(keyname, valuename, flags, string)) {
        return 0;
    }

    return 1;
}

int32_t SRegSaveValue(const char* keyname, const char* valuename, uint32_t flags, uint32_t value) {
    STORM_VALIDATE_BEGIN;
    STORM_VALIDATE(keyname);
    STORM_VALIDATE(*keyname);
    STORM_VALIDATE(valuename);
    STORM_VALIDATE(*valuename);
    STORM_VALIDATE_END;

    char valuestring[11];
    SStrPrintf(valuestring, sizeof(valuestring), "%u", value);

    if (!InternalSaveEntry(keyname, valuename, flags, valuestring)) {
        return 0;
    }

    return 1;
}

void SRegDestroy() {
    s_lockregistry.Enter();
    RegistryShutdown();
    s_lockregistry.Leave();
}
