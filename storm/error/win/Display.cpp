#include "storm/Error.hpp"
#include "storm/error/Error.cpp"

#include <windows.h>

#include <cstdarg>
#include <cstdlib>
#include <string>

std::string errorf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    constexpr size_t size = 1024;

    char buf[size] = {0};
    auto n = vsnprintf(buf, size, format, args);

    va_end(args);

    if (n < 0) {
        return "";
    } else if (n >= size) {
        std::string result(buf, n);
        return result;
    } else {
        return std::string(buf);
    }
}

int32_t SErrDisplayError(uint32_t errorcode, const char* filename, int32_t linenumber, const char* description, int32_t recoverable, uint32_t exitcode, uint32_t a7) {
    std::string s = "";
    s.reserve(1024);

    s += errorf("\n=========================================================\n");

    if (linenumber == -5) {
        s += errorf("Exception Raised!\n\n");

        s += errorf(" App:         %s\n", "GenericBlizzardApp");

        if (errorcode != 0x85100000) {
            s += errorf(" Error Code:  0x%08X\n", errorcode);
        }

        // TODO output time

        s += errorf(" Error:       %s\n\n", description);
    } else {
        s += errorf("Assertion Failed!\n\n");

        s += errorf(" App:         %s\n", "GenericBlizzardApp");
        s += errorf(" File:        %s\n", filename);
        s += errorf(" Line:        %d\n", linenumber);

        if (errorcode != 0x85100000) {
            s += errorf(" Error Code:  0x%08X\n", errorcode);
        }

        // TODO output time

        s += errorf(" Assertion:   %s\n", description);
    }

    // Print error in debugger
    OutputDebugString(s.c_str());

    // Title
    const char* caption = recoverable ? "Error" : "Unrecoverable error";

    // Icon/type
    UINT icon = recoverable ? MB_ICONWARNING : MB_ICONERROR;

    MessageBox(nullptr, s.c_str(), caption, icon);

    if (recoverable) {
        return 1;
    } else {
        exit(exitcode);
    }
}
