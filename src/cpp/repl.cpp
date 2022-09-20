#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "parse.hpp"
#include "terminal.hpp"
#include "value.hpp"

using namespace std::placeholders;

void handle_repl_input(const std::string& input, std::string& history) {
    try {
        std::shared_ptr<value> result = parse_values_from(input);
        if (result->type() == value_t::pair) {
            // there are items (not nil)
            auto list = to<value_pair>(result);
            for (const auto& item : *list) {
                // print one item per line
                std::cout << item << '\n';
            }

            history = list->str();                              // clean history
            history = history.substr(1, history.length() - 2);  // drop outermost brackets
        }
    } catch (std::exception& e) {
        auto error = make_error(e.what());
        std::cout << *error << '\n';  // print the error
        history = input;              // history as is
    }
}

int main() {
    terminal t{{"q", "quit", "exit"}};

    t.add_handler({"clr", "clear"}, std::bind([]() { if (system("clear")) {} }));
    t.add_handler({".*"}, std::bind(handle_repl_input, _1, _3));

    std::cout << "cpp-scheme version 0.1.0\n"
              << "press Ctrl-C to interrupt\n"
              << "type in \"quit\" to quit\n\n";

    t.run();  // run until an exit input

    std::cout << "\nbye!\n";

    return 0;
}
