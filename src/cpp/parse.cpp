#include "parse.hpp"

#include <cctype>
#include <exception>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "value.hpp"

namespace {

// constants

const char list_open_char{'('};
const char list_close_char{')'};
const char comment_char{';'};
const char quote_char{'\''};
const char string_delim_char{'"'};

const std::string quote_symbol{"quote"};

const std::unordered_set<char> non_alnum_symbol_chars{
    '_', '!', '?', '.', '#', '+', '-', '*', '/',
    '%', '^', '=', '<', '>', '&', '|', '\\'};

const std::unordered_map<std::string, std::shared_ptr<value>> special_symbols{
    {"#f", false_},
    {"false", false_},
    {"#t", true_},
    {"true", true_},
    {"nil", nil},
};

// exceptions

class parsing_error : public format_exception {
   public:
    parsing_error(const char* format, ...);
};

parsing_error::parsing_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    format_exception(format, args);
    va_end(args);
}

// functions

inline bool is_symbol_char(char c) {
    return (std::isalnum(c) || non_alnum_symbol_chars.count(c));
}

inline bool is_special_symbol(std::string& symbol) {
    return special_symbols.count(symbol);
}

std::shared_ptr<value_number> convert_to_number(std::string& symbol) {
    try {
        return make_number(std::stod(symbol));
    } catch (std::invalid_argument&) {
        return nullptr;
    }
}

std::shared_ptr<value> parse_symbol(std::istream& is) {
    std::string symbol;

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

std::shared_ptr<value_string> parse_string(std::istream& is) {
    std::string string;

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

void quote_item(std::shared_ptr<value>& item) {
    // transform an item x to the list (quote x)
    item = make_value_pair(quote_symbol, make_value_pair(item, nil));
}

void add_to_list(
    std::shared_ptr<value_pair>& head,
    std::shared_ptr<value_pair>& tail,
    std::shared_ptr<value>& item) {
    // make a new pair with the item as car
    auto pair = make_value_pair(item, nil);
    if (!head) {
        head = pair;  // first item: set the head
    } else {
        tail->cdr(pair);  // next item: set the cdr
    }
    tail = pair;  // move the tail
}

std::shared_ptr<value> parse_list(std::istream& is) {
    std::shared_ptr<value_pair> head = nullptr;
    std::shared_ptr<value_pair> tail = nullptr;

    size_t number_of_quotes = 0;
    std::shared_ptr<value> item = nullptr;

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
        return head;
    }
}

}  // namespace

std::shared_ptr<value> parse_values(std::istream& is) {
    try {
        // parse the whole string content as a list
        std::shared_ptr<value> result = parse_list(is);

        if (is) {
            // there are chars left in the stream
            throw parsing_error("premature end of list");
        }

        return result;
    } catch (parsing_error& e) {
        // something has gone wrong
        return make_error(e.what());
    }
}
