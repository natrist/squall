#include "storm/Registry.hpp"
#include "storm/String.hpp"
#include "storm/Option.hpp"
#include "test/Test.hpp"
#include "vendor/catch-2.13.10/catch.hpp"

TEST_CASE("SRegLoadValue", "[registry]") {
    SECTION("fail to load an entry that does not exist") {
        uint32_t value = 0;
        auto result = SRegLoadValue("Testing", "TestValue0", 0, &value);
        REQUIRE(value == 0);
        REQUIRE(result == 0);
    }

    SECTION("load values from a file") {
        auto result = SRegSaveValue("Testing", "TestValue1", 0, 0xFFFFFFFF);
        REQUIRE(result == 1);
        SRegDestroy();

        uint32_t value;
        result = SRegLoadValue("Testing", "TestValue1", 0, &value);
        REQUIRE(result == 1);
        REQUIRE(value == 0xFFFFFFFF);
    }

    SECTION("load value from memory") {
        auto result = SRegSaveValue("Testing", "TestValue2", 0, 0xFFFFFFFF);
        REQUIRE(result == 1);

        uint32_t value;
        result = SRegLoadValue("Testing", "TestValue2", 0, &value);
        REQUIRE(result == 1);
        REQUIRE(value == 0xFFFFFFFF);
    }

    SECTION("load an external value") {
        uint32_t value;
        if (SRegLoadValue("Testing", "ExternalValue", 0, &value)) {
            REQUIRE(value == 1);
        }
    }
}

TEST_CASE("SRegSaveValue", "[registry]") {
    SECTION("save a value into the registry") {
        auto result = SRegSaveValue("Testing", "TestValue1", 0, 0xFFFFFFFF);
        REQUIRE(result == 1);
        SRegDestroy();
    }
}

TEST_CASE("SRegLoadString", "[registry]") {
    SECTION("fail to load a string that does not exist") {
        char string[10];
        auto result = SRegLoadString("Testing", "TestString0", 0, string, sizeof(string));
        REQUIRE(result == 0);
    }

    SECTION("save and load a string") {
        SRegSaveString("Testing", "TestString1", 0, "teststring");

        char string[256];
        auto result = SRegLoadString("Testing", "TestString1", 0, string, sizeof(string));
        REQUIRE(result == 1);
        REQUIRE(SStrCmp("teststring", string, 256) == 0);
    }

    SECTION("save and load a UTF-8 string") {
        const char* source = "汉字";
        int32_t utf8enabled = 1;
        StormSetOption(11, &utf8enabled, sizeof(int32_t));
        int32_t result;
        result = SRegSaveString("Testing", "TestString2", STORM_REGISTRY_FLUSH_KEY, source);
        REQUIRE(result == 1);

        char string[256] = {0};
        result = SRegLoadString("Testing", "TestString2", 0, string, sizeof(string));
        REQUIRE(result == 1);
        REQUIRE(SStrCmp(source, string, 256) == 0);
    }
}
