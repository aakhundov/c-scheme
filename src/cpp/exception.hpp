#ifndef EXCEPTION_HPP_
#define EXCEPTION_HPP_

#include <cstdio>
#include <exception>
#include <string>

using std::exception;
using std::forward;
using std::string;

class str_exception : public exception {
   public:
    str_exception(const string& message) : _message(message) {}

    const char* what() const noexcept override {
        return _message.c_str();
    }

   protected:
    str_exception() {}

    string _message;
};

class format_exception : public str_exception {
   public:
    template <typename... Args>
    format_exception(const char* format, Args&&... args) {
        static char buffer[65536];

        if constexpr (sizeof...(args) > 0) {
            snprintf(buffer, sizeof(buffer), format, forward<Args>(args)...);
        } else {
            snprintf(buffer, sizeof(buffer), "%s", format);
        }

        _message = buffer;
    }
};

#endif  // EXCEPTION_HPP_
