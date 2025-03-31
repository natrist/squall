#include "storm/Unicode.hpp"
#include "storm/String.hpp"

#include <limits>

static const uint32_t offsetsFromUTF8[] = {
    0x0,
    0x3080,
    0xE2080,
    0x3C82080,
    0xFA082080,
    0x82082080
};

static const uint32_t firstByteMark[] = {
    0x0,
    0x0,
    0xC0,
    0xE0,
    0xF0,
    0xF8,
    0xFC
};

static const uint8_t bytesFromUTF8[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05
};

uint32_t SUniSGetUTF8(const uint8_t* strptr, int32_t* chars) {
    if (chars) {
        *chars = 0;
    }

    if (!strptr || !*strptr) {
        return -1;
    }

    if (chars) {
        *chars = *chars + 1;
    }

    uint32_t value = *strptr;
    int32_t extra;

    if ((value & 0xFE) == 0xFC) {
        value &= 0x01;
        extra = 5;
    } else if ((value & 0xFC) == 0xF8) {
        value &= 0x03;
        extra = 4;
    } else if ((value & 0xF8) == 0xF0) {
        value &= 0x07;
        extra = 3;
    } else if ((value & 0xF0) == 0xE0) {
        value &= 0x0F;
        extra = 2;
    } else if ((value & 0xE0) == 0xC0) {
        value &= 0x1F;
        extra = 1;
    } else if ((value & 0x80) == 0x00) {
        return value;
    } else {
        return 0x80000000;
    }

    const uint8_t* next = strptr + 1;

    for (int32_t i = 0; i < extra; i++, next++) {
        if (!*next) {
            return -1;
        }

        if (chars) {
            *chars = *chars + 1;
        }

        if ((*next & 0xC0) != 0x80) {
            return 0x80000000;
        }

        value = (value << 6) | (*next & 0x3F);
    }

    return value;
}

void SUniSPutUTF8(uint32_t c, char* strptr) {
    if (!strptr) {
        return;
    }

    auto curstr = strptr;
    auto v3 = c;
    char v4, v5, v6, v7;

    if (c >= 0x80) {
        if (c >= 0x800) {
            if (c >= 0x10000) {
                if (c >= 0x200000) {
                    if (c >= 0x400000) {
                        if (c >= 0x80000000) {
                            *curstr = 0;
                            return;
                        }

                        *strptr = (c >> 30) | 0xFC;
                        curstr = strptr + 1;
                        // TODO this seems likely to be bitwise right shift 24, not 8
                        v7 = ((c >> 8) & 0x3F) | 0x80;
                    } else {
                        // TODO this seems likely to be bitwise right shift 24, not 8
                        v7 = (c >> 8) | 0xF8;
                    }

                    *curstr++ = v7;
                    v6 = ((c >> 18) & 0x3F) | 0x80;
                } else {
                    v6 = (c >> 18) | 0xF0;
                }

                *curstr++ = v6;
                v5 = ((c >> 12) & 0x3F) | 0x80;
            } else {
                v5 = (c >> 12) | 0xE0;
            }

            *curstr++ = v5;
            v4 = ((c >> 6) & 0x3F) | 0x80;
        } else {
            v4 = (c >> 6) | 0xC0;
        }

        *curstr++ = v4;
        v3 = (c & 0x3F) | 0x80;
    }

    *curstr++ = v3;
    *curstr = '\0';
}

int32_t SUniConvertUTF16to8(uint8_t* dst, uint32_t dstmaxchars, const uint16_t* src, uint32_t srcmaxchars, uint32_t* dstchars, uint32_t* srcchars) {
    auto srcend   = srcmaxchars == STORM_MAX_STR ? reinterpret_cast<const uint16_t*>(UINTPTR_MAX) : &src[srcmaxchars];
    auto dstend   = &dst[dstmaxchars];
    auto dststart = dst;
    auto srcstart = src;

    int32_t result;

    while (src < srcend) {
        uint32_t widechars = 1;
        auto grapheme      = static_cast<uint32_t>(src[0]);
        if (0xD7FF < grapheme && grapheme < 0xDC00) {
            if (src + 1 >= srcend) {
                goto fail;
            }
            auto char2 = static_cast<uint32_t>(src[1]);
            if (0xDBFF < char2 && char2 < 0xE000) {
                grapheme  = ((grapheme - 0xD7F7) * 1024) + char2;
                widechars = 2;
            }
        }

        uint32_t chars;

        if (grapheme < 0x80) {
            chars = 1;
            if (grapheme == 0) {
                if (dst < dstend) {
                    *dst++ = '\0';
                    result = 0;
                }
                goto done;
            }
        } else if (grapheme < 0x800) {
            chars = 2;
        } else if (grapheme < 0x10000) {
            chars = 3;
        } else if (grapheme < 0x200000) {
            chars = 4;
        } else if (grapheme < 0x4000000) {
            chars = 5;
        } else if (grapheme > 0x7FFFFFFF) {
            grapheme = 0xFFFD;
            chars    = 2;
        } else {
            chars = 6;
        }

        result = chars;

        dst += chars;

        if (dst > dstend) {
            goto done;
        }

        switch (chars) {
        case 6:
            *--dst = (grapheme & 0x3F) | 0x80;
            grapheme >>= 6;
        case 5:
            *--dst = (grapheme & 0x3F) | 0x80;
            grapheme >>= 6;
        case 4:
            *--dst = (grapheme & 0x3F) | 0x80;
            grapheme >>= 6;
        case 3:
            *--dst = (grapheme & 0x3F) | 0x80;
            grapheme >>= 6;
        case 2:
            *--dst = (grapheme & 0x3F) | 0x80;
            grapheme >>= 6;
        case 1:
            *--dst = grapheme | firstByteMark[chars];
        }

        src += widechars;
        dst += chars;
    }

fail:
    result = -1;

done:
    if (srcchars) {
        *srcchars = src - srcstart;
    }

    if (dstchars) {
        *dstchars = dst - dststart;
    }
    return result;
}

int32_t SUniConvertUTF8to16(uint16_t* dst, uint32_t dstmaxchars, const uint8_t* src, uint32_t srcmaxchars, uint32_t* dstchars, uint32_t* srcchars) {
    auto srcend   = srcmaxchars == STORM_MAX_STR ? reinterpret_cast<const uint8_t*>(UINTPTR_MAX) : src + srcmaxchars;
    auto dstend   = dst + dstmaxchars;
    auto dststart = dst;
    auto srcstart = src;

    int32_t result = 0;

    while (src < srcend) {
        auto bytes = bytesFromUTF8[*src];
        if ((src + bytes) >= srcend) {
            result = -1 - bytes;
            goto done;
        }

        uint32_t grapheme = 0;

        int32_t width = 0;

        switch (bytes) {
        case 5:
            grapheme = (grapheme + src[width++]) << 6;
        case 4:
            grapheme = (grapheme + src[width++]) << 6;
        case 3:
            grapheme = (grapheme + src[width++]) << 6;
        case 2:
            grapheme = (grapheme + src[width++]) << 6;
        case 1:
            grapheme = (grapheme + src[width++]) << 6;
        case 0:
            grapheme = (grapheme + src[width++]) - offsetsFromUTF8[bytes];
        }

        if (dst >= dstend) {
            result = 1;
            goto done;
        }

        if (grapheme < 0x10000) {
            *dst++ = static_cast<uint16_t>(grapheme);
            if (grapheme == 0x0) {
                goto done;
            }
        } else if (grapheme < 0x110000) {
            if (dst >= dstend) {
                result = 0;
                goto done;
            }
            auto v16 = grapheme - 0x10000;
            *dst++   = static_cast<uint16_t>(v16 >> 10)   - 0x2800;
            *dst++   = static_cast<uint16_t>(v16 & 0x3FF) - 0x2400;
        } else {
            *dst++ = 0xFFFD;
        }

        src += width;
    }

    result = -1;

done:
    if (srcchars) {
        *srcchars = src - srcstart;
    }

    if (dstchars) {
        *dstchars = dst - dststart;
    }
    return result;
}
