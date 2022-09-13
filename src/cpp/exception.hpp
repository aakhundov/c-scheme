#ifndef EXCEPTION_HPP_
#define EXCEPTION_HPP_

#include <exception>
#include <string>

class str_exception : public std::exception {
   public:
    str_exception(const std::string& message) : _message(message) {}

    const char* what() const noexcept override {
        return _message.c_str();
    }

   private:
    std::string _message;
};

#endif  // EXCEPTION_HPP_
