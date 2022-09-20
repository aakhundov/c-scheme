#ifndef PARSE_H_
#define PARSE_H_

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "value.hpp"

// parsing functions

std::shared_ptr<value> parse_values(std::istream& is);
std::shared_ptr<value> parse_values(const std::string& s);
std::shared_ptr<value> parse_values(const char* s);
std::shared_ptr<value> parse_values(const std::filesystem::path& p);

// exceptions

class parsing_error : public format_exception {
   public:
    using format_exception::format_exception;
};

#endif  // PARSE_H_
