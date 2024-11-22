#include "storm/Command.hpp"
#include "test/Test.hpp"

TEST_CASE("SCmdRegisterArgList", "[command]") {
    SECTION("register an argument list normally") {
        ARGLIST argList[] = {
            { 0x0, 1, "one", nullptr },
            { 0x0, 2, "two", nullptr }
        };

        SCmdRegisterArgList(argList, sizeof(argList) / sizeof(ARGLIST));
    }
}
