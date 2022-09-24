#include "terminal.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>

using std::cout;
using std::endl;
using std::getline;
using std::ifstream;
using std::regex;
using std::regex_match;
using std::smatch;
using std::string;
using std::vector;

// editline OS-specific variants

#ifdef _WIN32

#include <cstdio>

static char* readline(const char* prompt) {
    static char buffer[65536];

    fputs(prompt, stdout);
    fgets(buffer, sizeof(buffer), stdin);

    char* result = malloc(strlen(buffer) + 1);
    strcpy(result, buffer);

    // replace the trailing '\n' by '\0'
    result[strlen(buffer) - 1] = '\0';

    return result;
}

static void add_history(const char* line) {
}

#elif __APPLE__

#include <editline/readline.h>

#elif __linux__

#include <editline/history.h>
#include <editline/readline.h>

#endif  // _WIN32

// terminal

void terminal::add_handler(const vector<string>& patterns, handler_type handler) {
    for (const string& pattern : patterns) {
        // turn each pattern into a regex and add with the handler
        _handlers.push_back({regex(pattern), handler});
    }
}

void terminal::run() {
    while (true) {
        string input = _get_input();

        if (input.empty()) {
            continue;  // no input
        }

        for (const auto& exit_rgx : _exit_regexes) {
            if (regex_match(input, exit_rgx)) {
                return;  // exit input
            }
        }

        bool handled = false;
        for (const auto& [rgx, handler] : _handlers) {
            smatch matches;
            if (regex_match(input, matches, rgx)) {
                // there is a match
                string history_line;
                handler(input, matches, history_line);  // handle

                if (!history_line.empty()) {
                    _add_history(history_line);
                }

                handled = true;
                break;
            }
        }

        if (!handled) {
            // there was no match
            cout << "unhandled input: '" << input << "'\n";
        }
    }
}

string terminal::_get_input() {
    string result;

    char* line = readline(">>> ");
    if (strlen(line) > 0) {
        // one-line input
        result += line;
        free(line);
    } else {
        // multi-line input
        free(line);
        int done = 0;
        while (!done) {
            // collect lines until empty
            line = readline("... ");
            if (strlen(line) > 0) {
                // non-empty line
                result += line;  // next line in a row
                result += ' ';   // separate lines with space
            } else {
                // empty line
                done = 1;
            }
            free(line);
        }
        result.pop_back();  // drop trailing space
    }

    return result;
}

void terminal::_add_history(const string& line) {
    HIST_ENTRY* curr = history_get(history_length);
    if (curr == NULL || line != curr->line) {
        // the line != last line in the history
        add_history(line.c_str());  // add to history

        if (_history_file.is_open()) {
            // add to the history file and flush
            _history_file << line << endl;
        }
    }
}

void terminal::_load_history(const string& path) {
    if (_history_file.is_open()) {
        // the history file exists
        ifstream file{path};  // open for reading
        string line;
        while (getline(file, line)) {
            // add each file line to history
            add_history(line.c_str());
        }
    }
}

vector<regex> terminal::_make_exit_regexes(const vector<string>& patterns) {
    vector<regex> result;

    for (const string& pattern : patterns) {
        // turn each pattern into a regex and add
        result.push_back(regex(pattern));
    }

    return result;
}
