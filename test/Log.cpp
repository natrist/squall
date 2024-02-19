#include "storm/Log.hpp"
#include "test/Test.hpp"


TEST_CASE("SLogInitialize", "[log]") {
    SECTION("constructs correctly") {
        SLogInitialize();
        REQUIRE(SLogIsInitialized() == 1);
        SLogDestroy();
    }

    SECTION("destructs correctly") {
        SLogInitialize();
        SLogDestroy();
        REQUIRE(SLogIsInitialized() == 0);
    }
}

TEST_CASE("SLogCreate", "[log]") {
    SECTION("creates new log handle") {
        SLogInitialize();

        HSLOG log;
        REQUIRE(SLogCreate("test.log", SLOG_FLAG_DEFAULT, &log) == 1);
        REQUIRE(log != 0);

        SLogDestroy();
    }

    SECTION("creates new log file") {
        SLogInitialize();

        HSLOG log;
        REQUIRE(SLogCreate("test.log", SLOG_FLAG_OPEN_FILE, &log) == 1);
        REQUIRE(log != 0);

        SLogDestroy();
    }
}

TEST_CASE("SLogWrite", "[log]") {
    SLogInitialize();

    HSLOG log;
    REQUIRE(SLogCreate("test.log", SLOG_FLAG_DEFAULT, &log) == 1);
    REQUIRE(log != 0);

    SLogWrite(log, "SLogWrite Test");

    SLogDestroy();
}
