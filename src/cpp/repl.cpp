#include <algorithm>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "evaluator.hpp"
#include "machine.hpp"
#include "parsing.hpp"
#include "terminal.hpp"
#include "value.hpp"

using namespace std::placeholders;

using std::bind;
using std::cerr;
using std::cout;
using std::exception;
using std::ref;
using std::shared_ptr;
using std::sort;
using std::string;
using std::vector;
using std::filesystem::path;

namespace {

void print_env(evaluator& e) {
    auto env = e.global();

    vector<string> names;
    for (const auto& record : e.global()) {
        names.push_back(record.first);
    }
    std::sort(names.begin(), names.end());

    for (const auto& name : names) {
        cout << name << " = " << *env[name] << '\n';
    }
}

void set_trace(evaluator& e, const smatch& match) {
    string trace = match[1].str();

    if (trace == "off") {
        e.trace(machine_trace::off);
    } else if (trace == "code") {
        e.trace(machine_trace::code);
    } else {
        auto error = make_error("illegal trace: %s", trace.c_str());
        cerr << *error << '\n';
    }
}

void handle_repl_input(evaluator& e, const string& input, string& history) {
    try {
        // parse the input
        shared_ptr<value_pair> list = parse_values_from(input);
        const value_pair* expressions = to_ptr<value_pair>(list);

        // save the clean history line
        // (without outermost brackets)
        history = expressions->str();
        history = history.substr(1, history.length() - 2);

        while (expressions != nilptr) {
            try {
                // evaluate one exp at a time
                auto exp = expressions->car();
                auto result = e.evaluate(exp);
                cout << *result << '\n';
            } catch (exception& e) {
                auto error = make_error(e.what());
                if (auto se = dynamic_cast<scheme_error*>(&e)) {
                    // if scheme error, set the topic
                    error->topic(se->topic());
                }
                cerr << *error << '\n';
            }
            expressions = expressions->pcdr();
        }
    } catch (parsing_error& e) {
        auto error = make_error(e.what());
        error->topic(e.topic());
        cerr << *error << '\n';
        history = input;  // history as is
    }
}

}  // namespace

int main() {
    terminal t{{"q", "quit", "exit"}};
    evaluator e{path{"./lib/machines/evaluator.scm"}};

    t.add_handler({"env"}, bind(print_env, std::ref(e)));
    t.add_handler({"reset"}, bind([&e]() { e.reset(); }));
    t.add_handler({"clr", "clear"}, bind([]() { if (system("clear")) {} }));
    t.add_handler({"trace (\\w+)"}, bind(set_trace, std::ref(e), _2));
    t.add_handler({".*"}, bind(handle_repl_input, std::ref(e), _1, _3));

    cout << "cpp-scheme version 0.1.0\n"
         << "press Ctrl-C to interrupt\n"
         << "type in \"quit\" to quit\n\n";

    t.run();  // run until an exit input

    cout << "\nbye!\n";

    return 0;
}
