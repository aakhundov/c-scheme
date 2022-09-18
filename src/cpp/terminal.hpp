#ifndef TERMINAL_H_
#define TERMINAL_H_

#include <fstream>
#include <functional>
#include <initializer_list>
#include <regex>
#include <string>
#include <utility>
#include <vector>

class terminal {
   public:
    // parameters: input string, regex match results, history line (out parameter)
    using handler_type = std::function<void(const std::string&, const std::smatch&, std::string&)>;

    terminal(std::initializer_list<std::string> exit_patterns, std::string history_path = ".history")
        : _history_file{history_path, std::ios::app},
          _exit_regexes{_make_exit_regexes(exit_patterns)} {
        _load_history(history_path);
    }

    void add_handler(std::initializer_list<std::string> patterns, handler_type handler);
    void run();

   private:
    std::vector<std::regex> _make_exit_regexes(const std::initializer_list<std::string>& patterns);

    std::string _get_input();
    void _add_history(const std::string& line);
    void _load_history(const std::string& path);

    std::ofstream _history_file;
    std::vector<std::regex> _exit_regexes;
    std::vector<std::pair<std::regex, handler_type>> _handlers;
};

#endif  // TERMINAL_H_
