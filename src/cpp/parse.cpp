#include "parse.hpp"

#include <cctype>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "value.hpp"

using std::ifstream;
using std::invalid_argument;
using std::istream;
using std::istringstream;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::filesystem::exists;
using std::filesystem::is_regular_file;
using std::filesystem::path;

namespace {

// constants

const char list_open_char{'('};
const char list_close_char{')'};
const char comment_char{';'};
const char quote_char{'\''};
const char string_delim_char{'"'};

const string quote_symbol{"quote"};
const string dot_symbol{"."};

const unordered_set<char> non_alnum_symbol_chars{
    '_', '!', '?', '.', '#', '+', '-', '*', '/',
    '%', '^', '=', '<', '>', '&', '|', '\\'};

const unordered_map<string, shared_ptr<value>> special_symbols{
    {"#f", false_},
    {"false", false_},
    {"#t", true_},
    {"true", true_},
    {"nil", nil},
};

// functions

inline bool is_symbol_char(char c) {
    return (isalnum(c) || non_alnum_symbol_chars.count(c));
}

inline bool is_special_symbol(string& symbol) {
    return special_symbols.count(symbol);
}

shared_ptr<value_number> convert_to_number(string& symbol) {
    size_t pos;

    try {
        double number = stod(symbol, &pos);

        if (pos == symbol.length()) {
            return make_number(number);
        } else {
            return nullptr;
        }
    } catch (invalid_argument&) {
        return nullptr;
    }
}

shared_ptr<value> parse_symbol(istream& is) {
    string symbol;

    char c;
    while (is.get(c)) {
        if (is_symbol_char(c)) {
            // append the symbol char
            symbol += c;
        } else {
            // undo the non-symbol char
            is.unget();
            break;
        }
    }

    if (auto number = convert_to_number(symbol)) {
        return number;  // number
    } else if (is_special_symbol(symbol)) {
        return special_symbols.at(symbol);  // special symbol
    } else {
        return make_symbol(symbol);  // ordinary symbol
    }
}

shared_ptr<value_string> parse_string(istream& is) {
    string string;

    char c;
    while (is.get(c)) {
        if (c != string_delim_char) {
            string += c;  // append the string char
        } else {
            is.unget();  // undo the non-string char
            break;
        }
    }

    return make_string(string);
}

void quote_item(shared_ptr<value>& item) {
    // transform an item x to the list (quote x)
    item = make_vpair(
        make_symbol(quote_symbol),
        make_vpair(item, nil));
}

void add_to_list(
    shared_ptr<value_pair>& head,
    shared_ptr<value_pair>& tail,
    shared_ptr<value>& item) {
    // make a new pair with the item as car
    auto pair = make_vpair(item, nil);
    if (!head) {
        head = pair;  // first item: set the head
    } else {
        tail->cdr(pair);  // next item: set the cdr
    }
    tail = pair;  // move the tail
}

shared_ptr<value> replace_dots_in_list(shared_ptr<value> v) {
    shared_ptr<value> running = v;
    shared_ptr<value_pair> previous = nullptr;

    while (running != nil) {
        // iterate over the items of the list v
        auto pair = to<value_pair>(running);
        if (pair->car()->type() == value_t::symbol &&
            to<value_symbol>(pair->car())->symbol() == dot_symbol) {
            // current item is a dot symbol
            if (pair->cdr() == nil) {
                // no next item: (x .)
                throw parsing_error(
                    "unfollowed %s in %s",
                    dot_symbol.c_str(), v->str().c_str());
            }
            auto cdr = pair->pcdr();
            if (cdr->cdr() != nil) {
                // 2+ next items: (x . y z)
                throw parsing_error(
                    "2+ items after %s in %s",
                    dot_symbol.c_str(), v->str().c_str());
            }
            if (cdr->car()->type() == value_t::symbol &&
                to<value_symbol>(cdr->car())->symbol() == dot_symbol) {
                // next item is a dot symbol: (x . .)
                throw parsing_error(
                    "%s followed by %s in %s",
                    dot_symbol.c_str(), dot_symbol.c_str(), v->str().c_str());
            }

            if (!previous) {
                // (. then x) -> x
                return cdr->car();
            } else {
                // (x then . then y) -> (x . y)
                // (x then y then . then z) -> (x y . z)
                previous->cdr(cdr->car());
            }

            break;
        }

        previous = pair;        // remember the current pair
        running = pair->cdr();  // move to the next pair
    }

    return v;
}

shared_ptr<value> parse_list(istream& is) {
    shared_ptr<value_pair> head = nullptr;
    shared_ptr<value_pair> tail = nullptr;

    size_t number_of_quotes = 0;
    shared_ptr<value> item = nullptr;

    char c;
    bool done = false;
    while (!done && is >> c) {  // skip the whitespace
        switch (c) {
            case list_open_char:
                item = parse_list(is);  // parse nested list
                if (!(is >> c) || c != list_close_char) {
                    // nested list must end with closing char
                    throw parsing_error("unterminated list");
                }
                break;
            case list_close_char:
                is.unget();   // undo the close char
                done = true;  // end of the list
                break;
            case quote_char:
                // next item will be quoted
                number_of_quotes += 1;
                break;
            case string_delim_char:
                item = parse_string(is);  // parse nested string
                if (!(is >> c) || c != string_delim_char) {
                    // nested string must end with string delimiter
                    throw parsing_error("unterminated string");
                }
                break;
            case comment_char:
                // skip everything till the end of the line
                do {
                    is.get(c);
                } while (is && c != '\n');
                break;
            default:
                if (is_symbol_char(c)) {
                    is.unget();               // undo the symbol char
                    item = parse_symbol(is);  // parse nested symbol
                } else {
                    throw parsing_error("unexpected character: '%c'", c);
                }
        }

        if (item) {
            while (number_of_quotes > 0) {
                quote_item(item);
                number_of_quotes--;
            }

            add_to_list(head, tail, item);
            item = nullptr;  // reset
        }
    }

    if (number_of_quotes > 0) {
        // some quote char(s) left unfollowed
        throw parsing_error("unfollowed quote");
    }

    if (!head) {
        // empty list
        return nil;
    } else {
        // non-empty list
        if (is) {
            // buffer is not empty: inner list
            return replace_dots_in_list(head);
        } else {
            // buffer is empty: outermost list
            return head;
        }
    }
}

}  // namespace

shared_ptr<value_pair> parse_values_from(istream& is) {
    // parse the whole string content as a list
    shared_ptr<value> result = parse_list(is);

    if (is) {
        // there are chars left in the stream
        throw parsing_error("premature end of list");
    }

    return to<value_pair>(result);
}

shared_ptr<value_pair> parse_values_from(const string& str) {
    istringstream s{str};
    return parse_values_from(s);
}

shared_ptr<value_pair> parse_values_from(const char* str) {
    return parse_values_from(string(str));
}

shared_ptr<value_pair> parse_values_from(const path& p) {
    if (!exists(p)) {
        throw parsing_error("the path does not exist: '%s'", p.c_str());
    } else if (!is_regular_file(p)) {
        throw parsing_error("the path is not a file: '%s'", p.c_str());
    }

    ifstream f{p};

    if (!f.is_open()) {
        throw parsing_error("failed to open the file: '%s'", p.c_str());
    }

    return parse_values_from(f);
}
