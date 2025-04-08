#include "storm/Unicode.hpp"
#include "storm/String.hpp"
#include "test/Test.hpp"

TEST_CASE("SUniSGetUTF8", "[unicode]") {
    SECTION("returns ascii-range utf-8 first character") {
        auto string = "foobar";
        int32_t chars = 0;
        auto code = SUniSGetUTF8(reinterpret_cast<const uint8_t*>(string), &chars);

        REQUIRE(code == 'f');
        REQUIRE(chars == 1);
    }

    SECTION("returns non-ascii-range utf-8 first character") {
        auto string = "\xF0\x9F\x99\x82"
                      "foobar";
        int32_t chars = 0;
        auto code = SUniSGetUTF8(reinterpret_cast<const uint8_t*>(string), &chars);

        REQUIRE(code == 0x1F642);
        REQUIRE(chars == 4);
    }

    SECTION("returns null first character") {
        auto string = "";
        int32_t chars = 0;
        auto code = SUniSGetUTF8(reinterpret_cast<const uint8_t*>(string), &chars);

        REQUIRE(code == -1u);
        REQUIRE(chars == 0);
    }
}

TEST_CASE("SUniSPutUTF8", "[unicode]") {
    SECTION("writes ascii-range utf-8 first character") {
        auto code = 'f';
        char buffer[100] = { 0 };
        SUniSPutUTF8(code, buffer);

        REQUIRE(SStrLen(buffer) == 1);
        REQUIRE(!SStrCmp(buffer, "f", SStrLen(buffer)));
    }

    SECTION("writes non-ascii-range utf-8 first character") {
        auto code = 0x1F642;
        char buffer[100] = { 0 };
        SUniSPutUTF8(code, buffer);

        REQUIRE(SStrLen(buffer) == 4);
        REQUIRE(!SStrCmp(buffer, "\xF0\x9F\x99\x82", SStrLen(buffer)));
    }

    SECTION("writes null first character") {
        auto code = '\0';
        char buffer[100] = { 0 };
        SUniSPutUTF8(code, buffer);

        REQUIRE(SStrLen(buffer) == 0);
    }
}

TEST_CASE("SUniConvertUTF8to16", "[unicode]") {
    SECTION("convert ASCII to UTF-16") {
        auto str = "Software";
        uint16_t widechars[] = { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999 };
        uint32_t srcchars;
        uint32_t dstchars;
        auto result = SUniConvertUTF8to16(widechars, 64, reinterpret_cast<const uint8_t*>(str), 128, &dstchars, &srcchars);
        REQUIRE(result == 0);
        REQUIRE(dstchars == 9);
        REQUIRE(srcchars == 8);
        REQUIRE(widechars[0] == 0x0053);
        REQUIRE(widechars[1] == 0x006f);
        REQUIRE(widechars[2] == 0x0066);
        REQUIRE(widechars[3] == 0x0074);
        REQUIRE(widechars[4] == 0x0077);
        REQUIRE(widechars[5] == 0x0061);
        REQUIRE(widechars[6] == 0x0072);
        REQUIRE(widechars[7] == 0x0065);
        REQUIRE(widechars[8] == 0x0000);
}

    SECTION("convert UTF-8 汉字 to UTF-16") {
        uint16_t widechars[] = { 0x1111, 0x2222, 0x3333 };
        uint8_t chars[] = { 0xE6, 0xB1, 0x89, 0xE5, 0xAD, 0x97, 0x00 };
        uint32_t srcchars;
        uint32_t dstchars;
        auto result = SUniConvertUTF8to16(widechars, 3, chars, 7, &dstchars, &srcchars);
        REQUIRE(result == 0);
        REQUIRE(dstchars == 3);
        REQUIRE(srcchars == 6);

        REQUIRE(widechars[0] == 0x6C49);
        REQUIRE(widechars[1] == 0x5B57);
        REQUIRE(widechars[2] == 0x0000);
    }
}

TEST_CASE("SUniConvertUTF8to16Len", "[unicode]") {
    SECTION("fail with the correct result") {
        uint8_t chars[] = { 0xE6, 0xB1, 0x89, 0xE5, 0xAD, 0x97, 0x00 };
        uint32_t srcchars;

        int32_t result;

        result = SUniConvertUTF8to16Len(chars, 2, &srcchars);
        REQUIRE(result == -3);
        REQUIRE(srcchars == 0);

        result = SUniConvertUTF8to16Len(chars, 3, &srcchars);
        REQUIRE(result == -1);
        REQUIRE(srcchars == 3);

        result = SUniConvertUTF8to16Len(chars, 4, &srcchars);
        REQUIRE(result == -3);
        REQUIRE(srcchars == 3);

        result = SUniConvertUTF8to16Len(chars, 5, &srcchars);
        REQUIRE(result == -3);
        REQUIRE(srcchars == 3);

        result = SUniConvertUTF8to16Len(chars, 6, &srcchars);
        REQUIRE(result == -1);
        REQUIRE(srcchars == 6);
    }

    SECTION("get length correctly") {
        uint8_t chars[] = { 0xE6, 0xB1, 0x89, 0xE5, 0xAD, 0x97, 0x00 };
        uint32_t srcchars;

        auto result = SUniConvertUTF8to16Len(chars, 7, &srcchars);
        REQUIRE(result == 3);
        REQUIRE(srcchars == 7);
    }
}
