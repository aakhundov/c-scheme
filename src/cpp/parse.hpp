#ifndef PARSE_H_
#define PARSE_H_

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "value.hpp"

using std::istream;
using std::shared_ptr;
using std::string;
using std::filesystem::path;

// parsing functions

shared_ptr<value_pair> parse_values_from(istream& is);
shared_ptr<value_pair> parse_values_from(const string& s);
shared_ptr<value_pair> parse_values_from(const char* s);
shared_ptr<value_pair> parse_values_from(const path& p);

// exceptions

class parsing_error : public format_exception {
   public:
    using format_exception::format_exception;
};

#endif  // PARSE_H_
