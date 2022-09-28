#ifndef PRIMITIVES_HPP_
#define PRIMITIVES_HPP_

#include <memory>
#include <string>
#include <unordered_map>

#include "value.hpp"

using std::shared_ptr;
using std::string;
using std::unordered_map;

// types

using primitive_op = shared_ptr<value> (*)(const value_pair*);

// exceptions

class primitive_error : public scheme_error {
   public:
    template <typename... Args>
    primitive_error(const char* format, Args&&... args)
        : scheme_error("primitive error", format, forward<Args>(args)...) {}
};

// helper functions

bool is_primitive(const string& name);
const unordered_map<string, primitive_op>& get_primitives();

#endif  // PRIMITIVES_HPP_
