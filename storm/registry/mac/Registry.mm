#include "storm/Error.hpp"
#include "storm/String.hpp"
#include "storm/registry/Registry.hpp"
#include "storm/registry/mac/Static.hpp"
#include <cstring>

const char* NextComponent(const char* path, char* component, size_t size) {
    auto sep = SStrChrR(path, '\\');
    if (!sep) {
        return path + SStrCopy(component, path, size);
    } else {
        SStrNCopy(component, path, sep - path, size);
        return sep  + 1;
    }
}

bool GetDefaultsAndKeyPath(const char* key, const char* name, uint32_t flags, NSUserDefaults** defaults, char* path, size_t size) {
    STORM_ASSERT(key);
    STORM_ASSERT(*key);

    auto nextcomponent = NextComponent(key, path, size);

    id prefix = @"com.blizzard";
    if ((flags & STORM_REGISTRY_BATTLENET)) {
        prefix = @"net.battle";
    }
    NSString* domain;
    if (!size || *path) {
        domain = [NSString stringWithFormat: @"%@.%s", prefix, path];
    } else {
        domain = prefix;
    }

    auto sregstatic = SRegStatic::Get();
    id hive = sregstatic->hives[domain];
    *defaults = hive;

    if (hive == nullptr) {
        auto bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
        if ([bundleIdentifier isEqualToString: domain]) {
            hive = [NSUserDefaults standardUserDefaults];
        } else {
            hive = [[NSUserDefaults alloc] initWithSuiteName:domain];
        }
        *defaults = hive;
        sregstatic->hives[domain] = hive;
    }

    int32_t length;

    if (name && *name) {
        if (key && *key) {
            length = SStrPrintf(path, size, "%s/%s", nextcomponent, name);
        } else {
            length = SStrCopy(path, name, size);
        }
    } else {
        length = SStrCopy(path, key, size);
    }

    STORM_VALIDATE_BEGIN;
    STORM_VALIDATE(length < size);
    STORM_VALIDATE_END;

    for (auto sep = path; ; *sep = '/') {
        sep = SStrChr(sep, '\\');
        if (!sep) {
            break;
        }
    }

    return true;
}

id GetObject(const char* keyname, const char* valuename, uint32_t flags) {
    NSUserDefaults* defaults;
    char path[STORM_MAX_PATH];
    if (!GetDefaultsAndKeyPath(keyname, valuename, flags, &defaults, path, STORM_MAX_PATH)) {
        return nil;
    }

    id string = [NSString stringWithUTF8String: path];
    return [defaults objectForKey: string];
}

bool SetObject(const char* keyname, const char* valuename, uint32_t flags, NSObject* object) {
    NSUserDefaults* defaults;
    char path[STORM_MAX_PATH];
    if (!GetDefaultsAndKeyPath(keyname, valuename, flags, &defaults, path, STORM_MAX_PATH)) {
        return false;
    }
    id string = [NSString stringWithUTF8String:path];
    [defaults setObject:object forKey:string];
    return true;
}

int32_t SRegLoadString(const char* keyname, const char* valuename, uint32_t flags, char* buffer, uint32_t buffersize) {
    STORM_VALIDATE_BEGIN;
    STORM_VALIDATE(keyname);
    STORM_VALIDATE(*keyname);
    STORM_VALIDATE(valuename);
    STORM_VALIDATE(*valuename);
    STORM_VALIDATE(buffer);
    STORM_VALIDATE_END;

    @autoreleasepool {
        id string = GetObject(keyname, valuename, flags);
        if ([string isKindOfClass:[NSString class]]) {
            return [string getCString:buffer maxLength:buffersize encoding:NSUTF8StringEncoding];
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

    @autoreleasepool {
        id num = GetObject(keyname, valuename, flags);
        if ([num isKindOfClass:[NSNumber class]]) {
            *value = [num unsignedIntValue];
            return 1;
        }
    }

    return 0;
}

int32_t SRegSaveString(const char* keyname, const char* valuename, uint32_t flags, const char* string) {
    STORM_VALIDATE_BEGIN;
    STORM_VALIDATE(keyname);
    STORM_VALIDATE(*keyname);
    STORM_VALIDATE(valuename);
    STORM_VALIDATE(*valuename);
    STORM_VALIDATE(string);
    STORM_VALIDATE_END;

    @autoreleasepool {
        return SetObject(keyname, valuename, flags, [NSString stringWithUTF8String:string]);
    }
}

int32_t SRegSaveValue(const char* keyname, const char* valuename, uint32_t flags, uint32_t value) {
    STORM_VALIDATE_BEGIN;
    STORM_VALIDATE(keyname);
    STORM_VALIDATE(*keyname);
    STORM_VALIDATE(valuename);
    STORM_VALIDATE(*valuename);
    STORM_VALIDATE_END;

    @autoreleasepool {
        return SetObject(keyname, valuename, flags, [NSNumber numberWithUnsignedInt:value]);
    }
}

void SRegDestroy() {
}
