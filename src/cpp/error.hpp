#ifndef ERROR_HPP_
#define ERROR_HPP_

#include <cstdio>
#include <stdexcept>
#include <string>

using std::forward;
using std::runtime_error;
using std::string;

class scheme_error : public runtime_error {
   public:
    template <typename... Args>
    scheme_error(const char* format, Args&&... args)
        : runtime_error(_format(format, forward<Args>(args)...)) {}

   private:
    template <typename... Args>
    string _format(const char* format, Args&&... args) {
        static char buffer[65536];
        if constexpr (sizeof...(args) > 0) {
            snprintf(buffer, sizeof(buffer), format, forward<Args>(args)...);
        } else {
            snprintf(buffer, sizeof(buffer), "%s", format);
        }
        return string(buffer);
    }
};

#endif  // ERROR_HPP_
