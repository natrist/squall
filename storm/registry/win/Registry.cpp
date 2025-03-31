
#include "storm/registry/Registry.hpp"
#include "storm/Error.hpp"
#include "storm/String.hpp"
#include "storm/Unicode.hpp"
#include "storm/option/Options.hpp"
#include <algorithm>
#include <cstdlib>
#include <windows.h>

int32_t ILoadValue(HKEY parentKey, const char* subKeyName, const char* valuename, uint32_t* datatype, uint8_t* buffer, uint32_t bytes, uint32_t* bytesread) {
    int32_t result;
    uint16_t wname[STORM_MAX_PATH];
    HKEY key;

    if (g_opt.sregunicode) {
        SUniConvertUTF8to16(wname, STORM_MAX_PATH, reinterpret_cast<const uint8_t*>(subKeyName), STORM_MAX_STR, nullptr, nullptr);
        result = RegOpenKeyExW(parentKey, reinterpret_cast<LPWSTR>(wname), 0, KEY_EXECUTE, &key);
        if (result == ERROR_SUCCESS) {
            *bytesread = bytes;
            SUniConvertUTF8to16(wname, STORM_MAX_PATH, reinterpret_cast<const uint8_t*>(valuename), STORM_MAX_STR, nullptr, nullptr);
            auto value = RegQueryValueExW(key, reinterpret_cast<LPWSTR>(wname), nullptr, reinterpret_cast<LPDWORD>(datatype), reinterpret_cast<LPBYTE>(buffer), reinterpret_cast<LPDWORD>(bytesread));
            RegCloseKey(key);
            return value;
        }
    } else {
        result = RegOpenKeyExA(parentKey, reinterpret_cast<LPCSTR>(subKeyName), 0, KEY_EXECUTE, &key);
        if (result == ERROR_SUCCESS) {
            *bytesread = bytes;
            auto value = RegQueryValueExA(key, reinterpret_cast<LPCSTR>(valuename), nullptr, reinterpret_cast<LPDWORD>(datatype), reinterpret_cast<LPBYTE>(buffer), reinterpret_cast<LPDWORD>(bytesread));
            RegCloseKey(key);
            return value;
        }
    }

    return result;
}

int32_t SRegGetBaseKey(uint8_t flags, char* buffer, uint32_t buffersize) {
    STORM_VALIDATE_BEGIN;
    STORM_VALIDATE(buffer);
    STORM_VALIDATE(buffersize);
    STORM_VALIDATE_END;

    if (flags & STORM_REGISTRY_BATTLENET) {
        SStrCopy(buffer, "Software\\Battle.net\\", STORM_MAX_PATH);
    } else {
        SStrCopy(buffer, "Software\\Blizzard Entertainment\\", STORM_MAX_PATH);
    }

    return 1;
}

void BuildFullKeyName(const char* keyname, uint8_t flags, char* buffer, uint32_t buffersize) {
    *buffer = '\0';
    if (!(flags & STORM_REGISTRY_NO_BASE_KEY)) {
        SRegGetBaseKey(flags, buffer, buffersize);
    }
    SStrPack(buffer, keyname, buffersize);
}

int32_t InternalLoadEntry(const char* keyname, const char* valuename, uint32_t flags, uint32_t* datatype, uint8_t* buffer, uint32_t bytes, uint32_t* bytesread) {
    char fullkeyname[STORM_MAX_PATH];
    *fullkeyname = '\0';
    *bytesread   = 0;
    *datatype    = 0;

    BuildFullKeyName(keyname, flags, fullkeyname, STORM_MAX_PATH);

    auto result = ILoadValue(flags & STORM_REGISTRY_CURRENT_USER_ONLY ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, fullkeyname, valuename, datatype, buffer, bytes, bytesread);
    if (result) {
        SetLastError(result);
        return 0;
    }

    return 1;
}

int32_t InternalSaveEntry(const char* keyname, const char* valuename, uint32_t flags, uint32_t datatype, const uint8_t* buffer, uint32_t bytes) {
    char fullkeyname[STORM_MAX_PATH];
    *fullkeyname = '\0';
    BuildFullKeyName(keyname, flags, fullkeyname, STORM_MAX_PATH);

    HKEY key;
    DWORD disposition;
    int32_t result;
    uint16_t wname[STORM_MAX_PATH] = {0};

    if (g_opt.sregunicode) {
        SUniConvertUTF8to16(wname, STORM_MAX_PATH, reinterpret_cast<const uint8_t*>(fullkeyname), STORM_MAX_STR, nullptr, nullptr);
        result = RegCreateKeyExW(flags & STORM_REGISTRY_CURRENT_USER_ONLY ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, reinterpret_cast<LPCWSTR>(wname), 0, nullptr, 0, KEY_WRITE, nullptr, &key, &disposition);
    } else {
        result = RegCreateKeyExA(flags & STORM_REGISTRY_CURRENT_USER_ONLY ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, reinterpret_cast<LPCSTR>(fullkeyname), 0, nullptr, 0, KEY_WRITE, nullptr, &key, &disposition);
    }

    if (result) {
        SetLastError(result);
        return 0;
    }

    if (g_opt.sregunicode) {
        SUniConvertUTF8to16(wname, STORM_MAX_PATH, reinterpret_cast<const uint8_t*>(valuename), STORM_MAX_STR, nullptr, nullptr);
        result = RegSetValueExW(key, reinterpret_cast<LPCWSTR>(wname), 0, datatype, reinterpret_cast<const BYTE*>(buffer), bytes);
    } else {
        result = RegSetValueExA(key, reinterpret_cast<LPCSTR>(valuename), 0, datatype, reinterpret_cast<const BYTE*>(buffer), bytes);
    }
    if (!result && (flags & STORM_REGISTRY_FLUSH_KEY) != 0) {
        result = RegFlushKey(key);
    }

    RegCloseKey(key);
    if (result) {
        SetLastError(result);
        return 0;
    }
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

    uint32_t datatype;
    uint16_t widecharstr[4096];
    uint32_t bytesread;

    if (g_opt.sregunicode) {
        if (!InternalLoadEntry(keyname, valuename, flags, &datatype, reinterpret_cast<uint8_t*>(&widecharstr), sizeof(widecharstr), &bytesread)) {
            return 0;
        }

        if (datatype == REG_SZ) {
            uint32_t dstchars;
            SUniConvertUTF16to8(reinterpret_cast<uint8_t*>(buffer), buffersize, widecharstr, bytesread / 2, &dstchars, nullptr);
            buffer[std::min(buffersize - 1, dstchars)] = '\0';
            return 1;
        } else if (datatype == REG_DWORD) {
            SStrPrintf(buffer, buffersize, "%u", (reinterpret_cast<uint32_t*>(widecharstr))[0]);
            return 1;
        }
    } else {
        if (!InternalLoadEntry(keyname, valuename, flags, &datatype, reinterpret_cast<uint8_t*>(buffer), buffersize, &bytesread)) {
            return 0;
        }

        if (datatype == REG_SZ) {
            buffer[std::min(buffersize - 1, bytesread)] = '\0';
            return 1;
        } else if (datatype == REG_DWORD) {
            SStrPrintf(buffer, buffersize, "%u", (reinterpret_cast<uint32_t*>(buffer))[0]);
            return 1;
        }
    }

    return 0;
}

int32_t SRegLoadValue(const char* keyname, const char* valuename, uint32_t flags, uint32_t* value) {
    STORM_VALIDATE_BEGIN;
    STORM_VALIDATE(keyname);
    STORM_VALIDATE(*keyname);
    STORM_VALIDATE(valuename);
    STORM_VALIDATE(*valuename);
    STORM_VALIDATE(value);
    STORM_VALIDATE_END;

    uint8_t buffer[256];
    uint32_t bytesread;
    uint32_t datatype;
    if (!InternalLoadEntry(keyname, valuename, flags, &datatype, buffer, sizeof(buffer), &bytesread)) {
        return 0;
    }

    if (datatype == REG_SZ) {
        *value = strtoul(reinterpret_cast<char*>(buffer), 0, 0);
    } else if (datatype == REG_DWORD) {
        *value = *reinterpret_cast<uint32_t*>(buffer);
    }

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

    if (g_opt.sregunicode) {
        uint16_t widecharstr[STORM_MAX_PATH];
        SUniConvertUTF8to16(widecharstr, STORM_MAX_PATH, reinterpret_cast<const uint8_t*>(string), STORM_MAX_STR, nullptr, nullptr);
        // ersatz wcslen()
        uint32_t length;
        auto p = widecharstr;
        while (*p) {
            ++p;
        }
        length = p - widecharstr;
        return InternalSaveEntry(keyname, valuename, flags, REG_SZ, reinterpret_cast<const uint8_t*>(widecharstr), (length + 1) * 2);
    } else {
        return InternalSaveEntry(keyname, valuename, flags, REG_SZ, reinterpret_cast<const uint8_t*>(string), SStrLen(string) + 1);
    }
}

int32_t SRegSaveValue(const char* keyname, const char* valuename, uint32_t flags, uint32_t value) {
    STORM_VALIDATE_BEGIN;
    STORM_VALIDATE(keyname);
    STORM_VALIDATE(*keyname);
    STORM_VALIDATE(valuename);
    STORM_VALIDATE(*valuename);
    STORM_VALIDATE_END;

    return InternalSaveEntry(keyname, valuename, flags, REG_DWORD, reinterpret_cast<const uint8_t*>(&value), 4);
}

void SRegDestroy() {
}
