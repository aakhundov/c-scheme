#include "parse.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"
#include "value.h"

static const char* EXP_CHARS = "eE";
static const char* SIGN_CHARS = "+-";
static const char* DIGIT_CHARS = "0123456789";
static const char* WHITESPACE_CHARS = " \t\r\n\v";
static const char* SYMBOL_CHARS =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "_+-*/%^\\=<>!&|?.";

static char LIST_OPEN_CHAR = '(';
static char LIST_CLOSE_CHAR = ')';
static char COMMENT_CHAR = ';';
static char STRING_CHAR = '"';
static char QUOTE_CHAR = '\'';
static char* QUOTE_SYMBOL = "quote";
static char* DOT_SYMBOL = ".";

static int parse_token(char* input, value** v);

static int is_number(char* symbol) {
    char* running = symbol;

    int digit_seen = 0;
    int exp_seen = 0;
    int dot_seen = 0;

    while (*running != '\0') {
        if (strchr(DIGIT_CHARS, *running)) {
            digit_seen = 1;
        } else if (strchr(SIGN_CHARS, *running)) {
            if (running != symbol && !strchr(EXP_CHARS, *(running - 1))) {
                return 0;
            }
        } else if (strchr(EXP_CHARS, *running)) {
            if (exp_seen || !digit_seen) {
                return 0;
            }
            digit_seen = 0;
            exp_seen = 1;
        } else if (*running == '.') {
            if (dot_seen || exp_seen) {
                return 0;
            }
            dot_seen = 1;
        } else {
            return 0;
        }
        running++;
    }

    return digit_seen;
}

static value* make_number(char* symbol) {
    errno = 0;
    double result = strtod(symbol, NULL);

    if (errno == 0) {
        return value_new_number(result);
    } else {
        return value_new_error("malformed number: %s", symbol);
    }
}

static int is_special(char* symbol) {
    return (
        strcmp(symbol, "nil") == 0 ||
        strcmp(symbol, "true") == 0 ||
        strcmp(symbol, "false") == 0);
}

static value* make_special(char* symbol) {
    if (strcmp(symbol, "nil") == 0) {
        return NULL;
    } else if (strcmp(symbol, "true") == 0) {
        return value_new_bool(1);
    } else if (strcmp(symbol, "false") == 0) {
        return value_new_bool(0);
    } else {
        return value_new_error("unknown special symbol: %s", symbol);
    }
}

static value* make_string(char* content) {
    size_t length = strlen(content);

    // remove trailing quote
    content[length - 1] = '\0';
    // remove leading quote
    content++;

    char* unescaped = str_unescape(content);
    value* result = value_new_string(unescaped);
    free(unescaped);

    return result;
}

static int parse_symbol(char* input, value** v) {
    char* running = input;
    while (*running != '\0' && strchr(SYMBOL_CHARS, *running)) {
        running++;
    }

    size_t length = running - input;
    char* symbol = malloc(length + 1);
    strncpy(symbol, input, length);
    symbol[length] = '\0';

    if (is_number(symbol)) {
        *v = make_number(symbol);
    } else if (is_special(symbol)) {
        *v = make_special(symbol);
    } else {
        *v = value_new_symbol(symbol);
    }

    free(symbol);

    return length;
}

static int parse_string(char* input, value** v) {
    char* running = input + 1;
    while (!(*running == STRING_CHAR && *(running - 1) != '\\')) {
        if (*running == '\0') {
            *v = value_new_error("unterminated string");
            return running - input;
        }
        running++;
    }
    running++;  // skip the trailing quote

    size_t length = running - input;
    char* string = malloc(length + 1);
    strncpy(string, input, length);
    string[length] = '\0';

    *v = make_string(string);

    free(string);

    return length;
}

static value** add_child(value** parent, value* child) {
    if (*parent == NULL) {
        *parent = value_new_pair(child, NULL);
        return parent;
    } else {
        assert((*parent)->cdr == NULL);
        (*parent)->cdr = value_new_pair(child, NULL);
        return &((*parent)->cdr);
    }
}

static void replace_dot_in_list(value* v) {
    value* prev = NULL;
    while (v != NULL) {
        if (v->car != NULL && v->car->type == VALUE_SYMBOL && strstr(v->car->symbol, DOT_SYMBOL)) {
            value* error = NULL;
            if (prev == NULL) {
                error = value_new_error("nothing before %s", DOT_SYMBOL);
            } else if (v->cdr == NULL) {
                error = value_new_error("unfollowed %s", DOT_SYMBOL);
            } else if (v->cdr->cdr != NULL) {
                error = value_new_error("%s followed by 2+ items", DOT_SYMBOL);
            } else if (v->cdr->car->type == VALUE_SYMBOL && strstr(v->cdr->car->symbol, DOT_SYMBOL)) {
                error = value_new_error("%s followed by %s", DOT_SYMBOL, DOT_SYMBOL);
            }

            if (error == NULL) {
                value* new_cdr = v->cdr->car;
                v->cdr->car = NULL;
                value_dispose(v);
                prev->cdr = new_cdr;
            } else {
                while (v->cdr != NULL) {
                    v = v->cdr;
                }
                add_child(&v, error);
            }

            break;
        } else {
            prev = v;
            v = v->cdr;
        }
    }
}

static int parse_list(char* input, value** v, char terminal) {
    *v = NULL;

    value** pair = v;
    char* running = input;
    while (*running != terminal) {
        if (*running == '\0') {
            // non-terminal end of the input
            value* error = value_new_error("missing %c", terminal);
            pair = add_child(pair, error);
            break;
        } else if (strchr(WHITESPACE_CHARS, *running)) {
            // skip all whitespace chars
            running++;
        } else if (*running == LIST_CLOSE_CHAR) {
            // non-terminal closing symbol
            value* error = value_new_error("premature %c", *running);
            pair = add_child(pair, error);
            break;
        } else if (*running == COMMENT_CHAR) {
            // skip the comment till the end of the line
            while (!strchr("\r\n\0", *running)) {
                running++;
            }
        } else {
            value* child = NULL;
            running += parse_token(running, &child);
            pair = add_child(pair, child);
            if (child != NULL && child->type == VALUE_ERROR) {
                break;
            }
        }
    }

    // ignore dots in the outermost list
    if (terminal != '\0') {
        replace_dot_in_list(*v);
    }

    return running - input;
}

static int parse_quoted(char* input, value** v) {
    char* running = input + 1;  // skip the '
    while (*running != '\0' && strchr(WHITESPACE_CHARS, *running)) {
        // skip all whitespaces
        running++;
    }

    if (*running != '\0') {
        *v = NULL;

        value* quoted = NULL;
        running += parse_token(running, &quoted);
        value* quote = value_new_symbol(QUOTE_SYMBOL);

        value** pair = v;
        pair = add_child(pair, quote);
        pair = add_child(pair, quoted);
    } else {
        *v = value_new_error("unfollowed %c", QUOTE_CHAR);
    }

    return running - input;
}

static int parse_token(char* input, value** v) {
    char* running = input;
    if (*input == LIST_OPEN_CHAR) {
        running += parse_list(running + 1, v, LIST_CLOSE_CHAR) + 2;
    } else if (*input == QUOTE_CHAR) {
        running += parse_quoted(running, v);
    } else if (*input == STRING_CHAR) {
        running += parse_string(running, v);
    } else if (strchr(SYMBOL_CHARS, *running)) {
        running += parse_symbol(running, v);
    } else {
        *v = value_new_error("unexpected symbol '%c'", *running);
        // skip to the end of the input
        running += strlen(running);
    }

    return running - input;
}

value* find_error(value* v) {
    if (v != NULL) {
        if (v->type == VALUE_ERROR) {
            return v;
        } else if (v->type == VALUE_PAIR) {
            value* error = NULL;
            if ((error = find_error(v->car)) != NULL) {
                return error;
            } else if ((error = find_error(v->cdr)) != NULL) {
                return error;
            }
        }
    }

    return NULL;
}

value* parse_values(char* input) {
    value* result = NULL;
    // parse till the end of the input
    parse_list(input, &result, '\0');

    value* error;
    if ((error = find_error(result)) != NULL) {
        value* temp = value_new_error(error->symbol);
        value_dispose(result);
        result = temp;
    }

    return result;
}
