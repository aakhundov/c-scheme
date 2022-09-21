#ifndef TERMINAL_H_
#define TERMINAL_H_

#include <fstream>
#include <functional>
#include <initializer_list>
#include <regex>
#include <string>
#include <utility>
#include <vector>

using std::function;
using std::initializer_list;
using std::ios;
using std::ofstream;
using std::pair;
using std::regex;
using std::smatch;
using std::string;
using std::vector;

class terminal {
   public:
    // parameters: input string, regex match results, history line (out parameter)
    using handler_type = function<void(const string&, const smatch&, string&)>;

    terminal(initializer_list<string> exit_patterns, string history_path = ".history")
        : _history_file{history_path, ios::app},
          _exit_regexes{_make_exit_regexes(exit_patterns)} {
        _load_history(history_path);
    }

    void add_handler(initializer_list<string> patterns, handler_type handler);
    void run();

   private:
    vector<regex> _make_exit_regexes(const initializer_list<string>& patterns);

    string _get_input();
    void _add_history(const string& line);
    void _load_history(const string& path);

    ofstream _history_file;
    vector<regex> _exit_regexes;
    vector<pair<regex, handler_type>> _handlers;
};

#endif  // TERMINAL_H_
