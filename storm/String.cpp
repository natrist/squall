#include "storm/String.hpp"
#include "storm/Error.hpp"
#include "storm/Memory.hpp"
#include "storm/string/bjhash.hpp"
#include <cctype>
#include <cmath>
#include <cstring>
#include <strings.h>

uint8_t bytesFromUTF8[256] = {
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6
};

uint32_t offsetsFromUTF8[8] = {
    0,
    0,
    0x3080,
    0x0E2080,
    0x3C82080,
    0x0FA082080,
    0x82082080,
    0
};

int32_t s_initialized;

double s_realDigit[20][10];

void GetNextTextUpper(uint32_t* orig, const char** string, uint32_t* upper) {
    uint8_t byte = **string;
    int32_t v3 = bytesFromUTF8[byte];

    *orig = 0;
    *upper = 0;

    switch (v3) {
    case 6:
    case 5:
    case 4:
    case 3:
    case 2:
        *orig += **string;
        (*string)++;

        if (!**string) {
            return;
        }

        *orig <<= 6;

    case 1:
        *orig += **string;
        (*string)++;

    default:
        *orig -= offsetsFromUTF8[v3];

        if (*orig > 0xFFFF) {
            *orig = 0xFFFF;
        }

        uint32_t v4 = *orig;

        bool v5, v6;

        if (v3 == 1) {
            if (v4 < 0x61) {
                *upper = v4;
                return;
            }

            v5 = v4 < 0x7A;
            v6 = v4 == 122;

            if (!v5 && !v6) {
                *upper = v4;
                return;
            }

            v4 -= 32;
            *upper = v4;
            return;
        }

        if (v3 != 2) {
            *upper = v4;
            return;
        }

        if (v4 >= 0xE0 && v4 <= 0xFE) {
            v4 -= 32;
            *upper = v4;
            return;
        }

        if (v4 == 339) {
            *upper = 338;
            return;
        }

        if (v4 == 1105) {
            *upper = 1025;
            return;
        }

        if (v4 >= 0x430) {
            v5 = v4 < 0x44F;
            v6 = v4 == 1103;

            if (!v5 && !v6) {
                *upper = v4;
                return;
            }

            v4 -= 32;
            *upper = v4;
            return;
        }

        *upper = v4;
        return;
    }
}

void InitializeFloatDigits() {
    for (int32_t i = -1; i > -21; i--) {
        for (int32_t j = 0; j < 10; j++) {
            double v3 = 10.0;
            int32_t v4 = i < 0 ? -i : i;
            double v5 = 1.0;

            while (true) {
                if (v4 & 1) {
                    v5 *= v3;
                }

                v4 >>= 1;

                if (!v4) {
                    break;
                }

                v3 *= v3;
            }

            double v6 = i < 0 ? 1.0 / v5 : v5;

            double v7 = v6 * static_cast<double>(j);

            s_realDigit[-i - 1][j] = v7;
        }
    }
}

void SStrInitialize() {
    if (!s_initialized) {
        InitializeFloatDigits();
        s_initialized = 1;
    }
}

const char* SStrChr(const char* string, char search) {
    STORM_ASSERT(string);

    if (!*string) {
        return nullptr;
    }

    while (*string != search) {
        string++;

        if (!*string) {
            return nullptr;
        }
    }

    return string;
}

const char* SStrChrR(const char* string, char search) {
    STORM_ASSERT(string);

    const char* result;

    for (result = nullptr; *string; string++) {
        if (*string == search) {
            result = string;
        }
    }

    return result;
}

int32_t SStrCmp(const char* string1, const char* string2, size_t maxchars) {
    return strncmp(string1, string2, maxchars);
}

int32_t SStrCmpI(const char* string1, const char* string2, size_t maxchars) {
    return strncasecmp(string1, string2, maxchars);
}

size_t SStrCopy(char* dest, const char* source, size_t destsize) {
    STORM_ASSERT(dest);
    STORM_ASSERT(source);

    char* destbuf = dest;

    if (destsize == STORM_MAX_STR) {
        while (*source) {
            *destbuf = *source;

            ++destbuf;
            ++source;
        }
    } else {
        if (*source) {
            while (destbuf < &dest[destsize - 1]) {
                *destbuf = *source;

                ++destbuf;
                ++source;

                if (!*source) {
                    *destbuf = '\0';
                    return static_cast<size_t>(destbuf - dest);
                }
            }
        }
    }

    *destbuf = '\0';
    return static_cast<size_t>(destbuf - dest);
}

char* SStrDupA(const char* string, const char* filename, uint32_t linenumber) {
    size_t len = SStrLen(string) + 1;
    char* dup = static_cast<char*>(SMemAlloc(len, filename, linenumber, 0x0));
    memcpy(dup, string, len);

    return dup;
}

uint32_t SStrHashHT(const char* string) {
    char normalized[0x400];
    char* buf = normalized;

    uint32_t length = 0;

    if (*string) {
        uint32_t value, orig, upper;

        while (*string && length <= 0x3FB) {
            // Convert each character to uppercase
            GetNextTextUpper(&orig, &string, &upper);

            // Replace forward slash with back slash
            value = upper == '/' ? '\\' : upper;

            while (value) {
                *buf = value;
                value >>= 8;
                length++;
                buf++;
            }
        }
    }

    // Ensure string is terminated
    *buf = 0;

    return bjhash((uint8_t*)&normalized, length, 0);
}

size_t SStrLen(const char* string) {
    STORM_ASSERT(string);

    auto stringEnd = string;
    while (*stringEnd) {
        stringEnd++;
    }

    return stringEnd - string;
}

void SStrLower(char* string) {
    while (*string) {
        *string = static_cast<char>(tolower(*string));
        string++;
    }
}

uint32_t SStrPack(char* dest, const char* source, uint32_t destsize) {
    STORM_ASSERT(dest);
    STORM_ASSERT(source);

    if (!destsize) {
        return 0;
    }

    char* i;
    const char* v5;
    char v6;
    const char* j;

    for (i = dest; *i; i++)
        ;

    if (destsize == STORM_MAX_STR) {
        v6 = *source;

        for (j = source; *j; i++) {
            j++;
            *i = v6;
            v6 = *j;
        }
    } else {
        v5 = source;

        if (*source) {
            while (i < &dest[destsize - 1]) {
                *i++ = *v5++;

                if (!*v5) {
                    *i = '\0';
                    return i - dest;
                }
            }
        }
    }

    *i = '\0';
    return i - dest;
}

const char* SStrStr(const char* string, const char* search) {
    STORM_ASSERT(string);
    STORM_ASSERT(search);

    if (!*string) {
        return nullptr;
    }

    auto searchEnd = search;
    while (*searchEnd) {
        searchEnd++;
    }
    size_t searchLength = searchEnd - search;

    auto substring = string;

    while (SStrCmp(substring, search, searchLength)) {
        substring++;

        if (!*substring) {
            return nullptr;
        }
    }

    return substring;
}

float SStrToFloat(const char* string) {
    STORM_ASSERT(string);

    SStrInitialize();

    double result;
    bool negative = false;

    if (*string == '-') {
        negative = true;
        string++;
    }

    double v16 = 10.0;
    double v4 = 0.0;
    uint32_t v5 = *string - '0';
    const char* v6 = string;

    if (v5 >= 10) {
        v5 = 0;
        result = static_cast<double>(v5);
    } else {
        string++;

        uint32_t v8 = *string - '0';

        if (v8 >= 10) {
            result = static_cast<double>(v5);
        } else {
            do {
                v5 = v8 + 10 * v5;
                string++;

                if (v5 >= 0x19999999) {
                    v4 = v4 * pow(10.0, string - v6) + static_cast<double>(v5);
                    v5 = 0;
                    v6 = string;
                }

                v8 = *string - '0';
            } while (v8 < 10);

            if (v4 == 0.0) {
                result = static_cast<double>(v5);
            } else {
                result = pow(10.0, string - v6) * v4 + static_cast<double>(v5);
            }
        }
    }

    if (*string == '.') {
        string++;

        uint32_t v23 = *string - '0';
        int32_t v24 = 0;

        if (v23 < 10) {
            int32_t v25 = 0;
            int32_t v26 = -1;
            double v31;

            do {
                string++;

                if (v24 < 20) {
                    v31 = s_realDigit[0][v25 + v23];
                } else {
                    v31 = pow(v16, v26) * v23;
                }

                result += v31;

                v23 = *string - '0';
                v24++;
                v26--;
                v25 += 10;
            } while (v23 < 10);
        }
    }

    if (*string == 'e' || *string == 'E') {
        const char* v32 = string + 1;

        if (*v32 == '+') {
            v32++;
        }

        result *= pow(10.0, SStrToInt(v32));
    }

    if (negative) {
        result = -result;
    }

    return static_cast<float>(result);
}

int32_t SStrToInt(const char* string) {
    STORM_ASSERT(string);

    int32_t result = 0;
    bool negative = false;

    if (*string == '-') {
        negative = true;
        string++;
    }

    uint32_t digit;
    while ((digit = *string - '0') < 10) {
        result = digit + (10 * result);
        string++;
    }

    if (negative) {
        result = -result;
    }

    return result;
}
