#include "storm/error/Error.hpp"
#include "storm/Array.hpp"

#include <windows.h>

#include <cstdarg>
#include <cstdlib>
#include <cstdint>
#include <string>

class error_report {
    private:
        TSGrowableArray<uint8_t> m_text;
        uint32_t m_errorcode;
        const char* m_filename;
        int32_t m_linenumber;
        const char* m_description;
        int32_t m_recoverable;

        void printf(const char *format, ...) {
            constexpr size_t size = 1024;
            char buf[size] = {0};

            va_list args;
            va_start(args, format);
            auto n = vsnprintf(buf, size, format, args);
            va_end(args);

            // Remove trailing zero
            this->m_text.SetCount(this->m_text.Count()-1);
            // Add formatted bytes plus trailing zero
            this->m_text.Add(n+1, reinterpret_cast<uint8_t*>(buf));
        }
    public:
        error_report(uint32_t errorcode, const char* filename, int32_t linenumber, const char* description, int32_t recoverable) {
            this->m_errorcode = errorcode;
            this->m_filename = filename;
            this->m_linenumber = linenumber;
            this->m_description = description;
            this->m_recoverable = recoverable;
            this->m_text.SetCount(1);
            this->m_text[0] = '\0';
        }

        void Format() {
            this->printf("\n=========================================================\n");

            if (this->m_linenumber == -5) {
                this->printf("Exception Raised!\n\n");

                this->printf(" App:         %s\n", "GenericBlizzardApp");

                if (this->m_errorcode != 0x85100000) {
                    this->printf(" Error Code:  0x%08X\n", this->m_errorcode);
                }

                // TODO output time

                this->printf(" Error:       %s\n\n", this->m_description);
            } else {
                this->printf("Assertion Failed!\n\n");

                this->printf(" App:         %s\n", "GenericBlizzardApp");
                this->printf(" File:        %s\n", this->m_filename);
                this->printf(" Line:        %d\n", this->m_linenumber);

                if (this->m_errorcode != 0x85100000) {
                    this->printf(" Error Code:  0x%08X\n", this->m_errorcode);
                }

                // TODO output time
                this->printf(" Assertion:   %s\n", this->m_description);
            }
        }

        void Display() {
            // Output text to debugger
            OutputDebugString(reinterpret_cast<LPCSTR>(this->m_text.Ptr()));
            // Title
            const char* caption = this->m_recoverable ? "Error" : "Unrecoverable error";
            // Icon/type
            UINT icon = this->m_recoverable ? MB_ICONWARNING : MB_ICONERROR;
            MessageBox(nullptr, reinterpret_cast<LPCSTR>(this->m_text.Ptr()), caption, icon);
        }
};

int32_t SErrDisplayError(uint32_t errorcode, const char* filename, int32_t linenumber, const char* description, int32_t recoverable, uint32_t exitcode, uint32_t a7) {
    error_report report(errorcode, filename, linenumber, description, recoverable);

    // Format error message
    report.Format();
    // Show MessageBox (blocks until user response)
    report.Display();

    if (recoverable) {
        return 1;
    } else {
        exit(exitcode);
    }
}
