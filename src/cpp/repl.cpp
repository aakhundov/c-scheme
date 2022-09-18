#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "parse.hpp"
#include "terminal.hpp"
#include "value.hpp"

using namespace std::placeholders;

void handle_repl_input(const std::string& input, std::string& history) {
    std::shared_ptr<value> result = parse_values(input);

    if (result->type() == value_t::error) {
        std::cout << *result << '\n';  // print the error
        history = input;               // history as is
    } else if (result->type() == value_t::pair) {
        auto list = std::reinterpret_pointer_cast<value_pair>(result);
        for (const auto& item : *list) {
            // print one item per line
            std::cout << *item << '\n';
        }

        history = list->str();                              // clean history
        history = history.substr(1, history.length() - 2);  // omit outermost brackets
    }
}

int main() {
    terminal t{{"q", "quit", "exit"}};

    t.add_handler({"clr", "clear"}, std::bind([]() { system("clear"); }));
    t.add_handler({".*"}, std::bind(handle_repl_input, _1, _3));

    std::cout << "cpp-scheme version 0.1.0\n"
              << "press Ctrl-C to interrupt\n"
              << "type in \"quit\" to quit\n\n";

    t.run();  // run until an exit input

    std::cout << "\nbye!\n";

    return 0;
}
