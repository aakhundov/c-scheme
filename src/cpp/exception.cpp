#include "exception.hpp"

#include <cstdarg>
#include <cstdio>

format_exception::format_exception(const char* format, ...) {
    static char buffer[65536];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    _message = buffer;
}

format_exception::format_exception(const char* format, va_list args) {
    static char buffer[65536];

    vsnprintf(buffer, sizeof(buffer), format, args);

    _message = buffer;
}
