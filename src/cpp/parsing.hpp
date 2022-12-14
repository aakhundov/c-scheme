#ifndef PARSE_H_
#define PARSE_H_

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "error.hpp"
#include "value.hpp"

using std::istream;
using std::shared_ptr;
using std::string;
using std::filesystem::path;

// exceptions

class parsing_error : public scheme_error {
   public:
    template <typename... Args>
    parsing_error(const char* format, Args&&... args)
        : scheme_error("parsing error", format, forward<Args>(args)...) {}
};

// parsing functions

shared_ptr<value_pair> parse_values_from(istream& is);
shared_ptr<value_pair> parse_values_from(const string& s);
shared_ptr<value_pair> parse_values_from(const char* s);
shared_ptr<value_pair> parse_values_from(const path& p);

#endif  // PARSE_H_
