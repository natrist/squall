#ifndef STORM_REGISTRY_REGISTRY_HPP
#define STORM_REGISTRY_REGISTRY_HPP

#include <cstdint>

#define STORM_REGISTRY_MAX_PATH  260
#define STORM_REGISTRY_MAX_VALUE 16384

#define STORM_REGISTRY_BATTLENET         0x02
#define STORM_REGISTRY_CURRENT_USER_ONLY 0x04
#define STORM_REGISTRY_FLUSH_KEY         0x08
#define STORM_REGISTRY_NO_BASE_KEY       0x10

#define STORM_REGISTRY_TYPE_STRING 1
#define STORM_REGISTRY_TYPE_DWORD  4

int32_t SRegLoadString(const char* keyname, const char* valuename, uint32_t flags, char* buffer, uint32_t buffersize);

int32_t SRegLoadValue(const char* keyname, const char* valuename, uint32_t flags, uint32_t* value);

int32_t SRegSaveString(const char* keyname, const char* valuename, uint32_t flags, const char* string);

int32_t SRegSaveValue(const char* keyname, const char* valuename, uint32_t flags, uint32_t value);

void SRegDestroy();

#endif
