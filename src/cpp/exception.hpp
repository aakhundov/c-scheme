#ifndef EXCEPTION_HPP_
#define EXCEPTION_HPP_

#include <cstdarg>
#include <exception>
#include <string>

class str_exception : public std::exception {
   public:
    str_exception(const std::string& message) : _message(message) {}

    const char* what() const noexcept override {
        return _message.c_str();
    }

   protected:
    str_exception() {}

    std::string _message;
};

class format_exception : public str_exception {
   public:
    format_exception(const char* format, ...);
    format_exception(const char* format, va_list args);

   protected:
    format_exception() {}
};

#endif  // EXCEPTION_HPP_
