#include "primitives.hpp"

#include <memory>
#include <string>
#include <unordered_map>

using std::shared_ptr;
using std::string;
using std::unordered_map;

namespace {

// helpers

// shared_ptr<value_error> assert_num_args(const value_pair* args, size_t num_args) {
//     if (args->length() != num_args) {
//         return make_error(
//             "expects %zu arg%s, but got %zu",
//             num_args, (num_args == 1 ? "" : "s"), args->length());
//     } else {
//         return nullptr;
//     }
// }

shared_ptr<value_error> assert_min_args(const value_pair* args, size_t min_args) {
    if (args->length() < min_args) {
        return make_error(
            "expects at least %zu arg%s, but got %zu",
            min_args, (min_args == 1 ? "" : "s"), args->length());
    } else {
        return nullptr;
    }
}

// shared_ptr<value_error> assert_max_args(const value_pair* args, size_t max_args) {
//     if (args->length() > max_args) {
//         return make_error(
//             "expects at most %zu arg%s, but got %zu",
//             max_args, (max_args == 1 ? "" : "s"), args->length());
//     } else {
//         return nullptr;
//     }
// }

// shared_ptr<value_error> assert_arg_type(const value_pair* args, size_t ordinal, value_t type) {
//     size_t running = ordinal;
//     for (const auto& arg : *args) {
//         if (running == 0) {
//             if (arg.type() != type) {
//                 return make_error(
//                     "arg #%zu must be %s, but is %s %s",
//                     ordinal, get_type_name(type), get_type_name(arg.type()), arg.str().c_str());
//             } else {
//                 return nullptr;
//             }
//         }
//         running--;
//     }
//     return make_error(
//         "arg #%zu of type %s is missing",
//         ordinal, get_type_name(type));
// }

shared_ptr<value_error> assert_all_args_type(const value_pair* args, size_t offset, value_t type) {
    size_t ordinal = 0;
    for (const auto& arg : *args) {
        if (ordinal >= offset && arg.type() != type) {
            return make_error(
                "arg #%zu must be %s, but is %s %s",
                ordinal, get_type_name(type), get_type_name(arg.type()), arg.str().c_str());
        }
        ordinal++;
    }
    return nullptr;
}

// shared_ptr<value> get_optional_arg(const value_pair* args, size_t ordinal, const shared_ptr<value>& default_) {
//     while (args != nilptr) {
//         if (ordinal == 0) {
//             return args->car();
//         }
//         args = args->pcdr();
//         ordinal--;
//     }
//     return default_;
// }

// primitives

shared_ptr<value> add(const value_pair* args) {
    if (auto error = assert_min_args(args, 1)) {
        return error;
    } else if (auto error = assert_all_args_type(args, 0, value_t::number)) {
        return error;
    }

    double result = *to_ptr<value_number>(args->car());
    for (const auto& arg : *args->pcdr()) {
        result += reinterpret_cast<const value_number&>(arg);
    }

    return make_number(result);
}

shared_ptr<value> subtract(const value_pair* args) {
    if (auto error = assert_min_args(args, 1)) {
        return error;
    } else if (auto error = assert_all_args_type(args, 0, value_t::number)) {
        return error;
    }

    if (args->cdr() == nil) {
        // single argument: negate
        double value = *to_ptr<value_number>(args->car());
        return make_number(-value);
    }

    double result = *to_ptr<value_number>(args->car());
    for (const auto& arg : *args->pcdr()) {
        result -= reinterpret_cast<const value_number&>(arg);
    }

    return make_number(result);
}

shared_ptr<value> multiply(const value_pair* args) {
    if (auto error = assert_min_args(args, 1)) {
        return error;
    } else if (auto error = assert_all_args_type(args, 0, value_t::number)) {
        return error;
    }

    double result = *to_ptr<value_number>(args->car());
    for (const auto& arg : *args->pcdr()) {
        result *= reinterpret_cast<const value_number&>(arg);
    }

    return make_number(result);
}

shared_ptr<value> divide(const value_pair* args) {
    if (auto error = assert_min_args(args, 1)) {
        return error;
    } else if (auto error = assert_all_args_type(args, 0, value_t::number)) {
        return error;
    }

    double result = *to_ptr<value_number>(args->car());
    for (const auto& arg : *args->pcdr()) {
        double value = reinterpret_cast<const value_number&>(arg);
        if (value == 0.0) {
            return make_error("division by zero");
        }
        result /= value;
    }

    return make_number(result);
}

const unordered_map<string, primitive_op> primitives{
    {"+", add},
    {"-", subtract},
    {"*", multiply},
    {"/", divide},
};

}  // namespace

// hepler functions

bool is_primitive(const string& name) {
    return primitives.count(name) > 0;
}

const unordered_map<string, primitive_op>& get_primitives() {
    return primitives;
}
