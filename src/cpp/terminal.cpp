#include "terminal.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>

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

void terminal::add_handler(std::initializer_list<std::string> patterns, handler_type handler) {
    for (const std::string& pattern : patterns) {
        // turn each pattern into a regex and add with the handler
        _handlers.push_back({std::regex(pattern), handler});
    }
}

void terminal::run() {
    while (true) {
        std::string input = _get_input();

        if (input.empty()) {
            continue;  // no input
        }

        for (const auto& exit_regex : _exit_regexes) {
            if (std::regex_match(input, exit_regex)) {
                return;  // exit input
            }
        }

        bool handled = false;
        for (const auto& pair : _handlers) {
            std::smatch matches;
            if (std::regex_match(input, matches, pair.first)) {
                // there is a match
                std::string history_line;
                pair.second(input, matches, history_line);  // handle

                if (!history_line.empty()) {
                    _add_history(history_line);
                }

                handled = true;
                break;
            }
        }

        if (!handled) {
            // there was no match
            std::cout << "unhandled input: '" << input << "'\n";
        }
    }
}

std::string terminal::_get_input() {
    std::string result;

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

void terminal::_add_history(const std::string& line) {
    HIST_ENTRY* curr = history_get(history_length);
    if (curr == NULL || line != curr->line) {
        // the line != last line in the history
        add_history(line.c_str());  // add to history

        if (_history_file.is_open()) {
            // add to the history file and flush
            _history_file << line << std::endl;
        }
    }
}

void terminal::_load_history(const std::string& path) {
    if (_history_file.is_open()) {
        // the history file exists
        std::ifstream file{path};  // open for reading
        std::string line;
        while (std::getline(file, line)) {
            // add each file line to history
            add_history(line.c_str());
        }
    }
}

std::vector<std::regex> terminal::_make_exit_regexes(const std::initializer_list<std::string>& patterns) {
    std::vector<std::regex> result;

    for (const std::string& pattern : patterns) {
        // turn each pattern into a regex and add
        result.push_back(std::regex(pattern));
    }

    return result;
}
