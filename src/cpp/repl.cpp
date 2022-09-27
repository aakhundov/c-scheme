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

using std::bind;
using std::cout;
using std::exception;
using std::shared_ptr;
using std::string;

void handle_repl_input(const string& input, string& history) {
    try {
        shared_ptr<value_pair> list = parse_values_from(input);

        for (const auto& item : *list) {
            // print one item per line
            cout << item << '\n';
        }

        history = list->str();                              // clean history
        history = history.substr(1, history.length() - 2);  // drop outermost brackets
    } catch (exception& e) {
        auto error = make_error(e.what());
        if (auto se = dynamic_cast<scheme_error*>(&e)) {
            error->topic(se->topic());
        }
        cout << *error << '\n';  // print the error
        history = input;         // history as is
    }
}

int main() {
    terminal t{{"q", "quit", "exit"}};

    t.add_handler({"clr", "clear"}, bind([]() { if (system("clear")) {} }));
    t.add_handler({".*"}, bind(handle_repl_input, _1, _3));

    cout << "cpp-scheme version 0.1.0\n"
         << "press Ctrl-C to interrupt\n"
         << "type in \"quit\" to quit\n\n";

    t.run();  // run until an exit input

    cout << "\nbye!\n";

    return 0;
}
