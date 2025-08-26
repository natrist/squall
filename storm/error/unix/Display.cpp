#include "storm/error/Error.hpp"

#include <cstdio>
#include <cstdarg>
#include <cstdlib>

int32_t SErrDisplayError(uint32_t errorcode, const char* filename, int32_t linenumber, const char* description, int32_t recoverable, uint32_t exitcode, uint32_t numFramesToSkip) {
    // TODO

    printf("\n=========================================================\n");

    if (linenumber == -5) {
        printf("Exception Raised!\n\n");

        printf(" App:         %s\n", "GenericBlizzardApp");

        if (errorcode != 0x85100000) {
            printf(" Error Code:  0x%08X\n", errorcode);
        }

        // TODO output time

        printf(" Error:       %s\n\n", description);
    } else {
        printf("Assertion Failed!\n\n");

        printf(" App:         %s\n", "GenericBlizzardApp");
        printf(" File:        %s\n", filename);
        printf(" Line:        %d\n", linenumber);

        if (errorcode != 0x85100000) {
            printf(" Error Code:  0x%08X\n", errorcode);
        }

        // TODO output time

        printf(" Assertion:   %s\n", description);
    }

    if (recoverable) {
        return 1;
    } else {
        exit(exitcode);
    }
}
