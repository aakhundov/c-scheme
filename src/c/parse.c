#include "parse.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "const.h"
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
    "_+-*/%^\\=<>!&|?.#";

static char LIST_OPEN_CHAR = '(';
static char LIST_CLOSE_CHAR = ')';
static char COMMENT_CHAR = ';';
static char STRING_CHAR = '"';
static char QUOTE_CHAR = '\'';
static char* QUOTE_SYMBOL = "quote";
static char* DOT_SYMBOL = ".";

static int parse_token(const char* input, value** v, size_t* line, size_t* col);

static value* make_parsing_error(const size_t* line, const size_t* col, const char* format, ...) {
    va_list args;
    va_start(args, format);
    value* error = value_new_error_from_args(format, args);
    va_end(args);

    value* result = value_new_error("%s at %zu:%zu", error->symbol, *line, *col);
    value_dispose(error);

    return result;
}

static int is_number(const char* symbol) {
    const char* running = symbol;

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

static value* make_number(const char* symbol, size_t* line, size_t* col) {
    errno = 0;
    double result = strtod(symbol, NULL);

    if (errno == 0) {
        return value_new_number(result);
    } else {
        return make_parsing_error(line, col, "malformed number: %s", symbol);
    }
}

static int is_special(const char* symbol) {
    return (
        strcmp(symbol, "nil") == 0 ||
        strcmp(symbol, "true") == 0 ||
        strcmp(symbol, "false") == 0);
}

static value* make_special(const char* symbol) {
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

static int parse_symbol(const char* input, value** v, size_t* line, size_t* col) {
    const char* running = input;
    while (*running != '\0' && strchr(SYMBOL_CHARS, *running)) {
        if (*running == '\n') {
            *line += 1;
            *col = 0;
        }
        running++;
        *col += 1;
    }

    size_t length = running - input;
    char* symbol = malloc(length + 1);
    strncpy(symbol, input, length);
    symbol[length] = '\0';

    if (is_number(symbol)) {
        *v = make_number(symbol, line, col);
    } else if (is_special(symbol)) {
        *v = make_special(symbol);
    } else {
        *v = value_new_symbol(symbol);
    }

    free(symbol);

    return length;
}

static int parse_string(const char* input, value** v, size_t* line, size_t* col) {
    const char* running = input + 1;  // skip the leading quote
    *col += 1;                        // leading quote
    while (!(*running == STRING_CHAR && *(running - 1) != '\\')) {
        if (*running == '\0') {
            *v = make_parsing_error(line, col, "unterminated string");
            return running - input;
        }
        if (*running == '\n') {
            *line += 1;
            *col = 0;
        }
        running++;
        *col += 1;
    }
    running++;  // skip the trailing quote
    *col += 1;  // trailing quote

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

static value* replace_dot_in_list(value* v) {
    static char buffer[BUFFER_SIZE];

    value* prev = NULL;
    value* running = v;
    while (running != NULL) {
        if (running->car != NULL &&
            running->car->type == VALUE_SYMBOL &&
            strstr(running->car->symbol, DOT_SYMBOL)) {
            value* error = NULL;
            if (running->cdr == NULL) {
                value_to_str(v, buffer);
                error = value_new_error("unfollowed %s in %s", DOT_SYMBOL, buffer);
            } else if (running->cdr->cdr != NULL) {
                value_to_str(v, buffer);
                error = value_new_error("%s followed by 2+ items in %s", DOT_SYMBOL, buffer);
            } else if (running->cdr->car != NULL &&
                       running->cdr->car->type == VALUE_SYMBOL &&
                       strstr(running->cdr->car->symbol, DOT_SYMBOL)) {
                value_to_str(v, buffer);
                error = value_new_error("%s followed by %s in %s", DOT_SYMBOL, DOT_SYMBOL, buffer);
            } else if (prev == NULL) {
                value* next = running->cdr->car;
                running->cdr->car = NULL;
                value_dispose(running);

                return next;
            }

            if (error == NULL) {
                value* new_cdr = running->cdr->car;
                running->cdr->car = NULL;
                value_dispose(running);
                prev->cdr = new_cdr;
            } else {
                while (running->cdr != NULL) {
                    running = running->cdr;
                }
                add_child(&running, error);
            }

            break;
        } else {
            prev = running;
            running = running->cdr;
        }
    }

    return v;
}

static int parse_list(const char* input, value** v, char terminal, size_t* line, size_t* col) {
    *v = NULL;

    value** pair = v;
    const char* running = input;
    while (*running != terminal) {
        if (*running == '\0') {
            // non-terminal end of the input
            value* error = make_parsing_error(line, col, "missing %c", terminal);
            pair = add_child(pair, error);
            break;
        } else if (strchr(WHITESPACE_CHARS, *running)) {
            // skip all whitespace chars
            if (*running == '\n') {
                *line += 1;
                *col = 0;
            }
            running++;
            *col += 1;
        } else if (*running == LIST_CLOSE_CHAR) {
            // non-terminal closing symbol
            value* error = make_parsing_error(line, col, "premature %c", *running);
            pair = add_child(pair, error);
            break;
        } else if (*running == COMMENT_CHAR) {
            // skip the comment till the end of the line
            while (!strchr("\r\n\0", *running)) {
                running++;
                *col += 1;
            }
        } else {
            value* child = NULL;
            running += parse_token(running, &child, line, col);
            pair = add_child(pair, child);
            if (child != NULL && child->type == VALUE_ERROR) {
                break;
            }
        }
    }

    // ignore dots in the outermost list
    if (terminal != '\0') {
        *v = replace_dot_in_list(*v);
    }

    return running - input;
}

static int parse_quoted(const char* input, value** v, size_t* line, size_t* col) {
    const char* running = input + 1;  // skip the '
    *col += 1;                        // the '
    while (*running != '\0' && strchr(WHITESPACE_CHARS, *running)) {
        // skip all whitespaces
        if (*running == '\n') {
            *line += 1;
            *col = 0;
        }
        running++;
        *col += 1;
    }

    if (*running != '\0') {
        *v = NULL;

        value* quoted = NULL;
        running += parse_token(running, &quoted, line, col);
        value* quote = value_new_symbol(QUOTE_SYMBOL);

        value** pair = v;
        pair = add_child(pair, quote);
        pair = add_child(pair, quoted);
    } else {
        *v = make_parsing_error(line, col, "unfollowed %c", QUOTE_CHAR);
    }

    return running - input;
}

static int parse_token(const char* input, value** v, size_t* line, size_t* col) {
    const char* running = input;
    if (*input == LIST_OPEN_CHAR) {
        *col += 1;  // LIST_OPEN_CHAR
        running += parse_list(running + 1, v, LIST_CLOSE_CHAR, line, col) + 2;
        *col += 1;  // LIST_CLOSE_CHAR
    } else if (*input == QUOTE_CHAR) {
        running += parse_quoted(running, v, line, col);
    } else if (*input == STRING_CHAR) {
        running += parse_string(running, v, line, col);
    } else if (strchr(SYMBOL_CHARS, *running)) {
        running += parse_symbol(running, v, line, col);
    } else {
        *v = make_parsing_error(line, col, "unexpected symbol '%c'", *running);
        // skip to the end of the input
        running += strlen(running);
    }

    return running - input;
}

const value* find_error(const value* v) {
    if (v != NULL) {
        if (v->type == VALUE_ERROR) {
            return v;
        } else if (v->type == VALUE_PAIR) {
            const value* error = NULL;
            if ((error = find_error(v->car)) != NULL) {
                return error;
            } else if ((error = find_error(v->cdr)) != NULL) {
                return error;
            }
        }
    }

    return NULL;
}

value* parse_from_str(const char* input) {
    value* result = NULL;
    size_t line = 1, col = 1;

    // parse till the end of the input
    parse_list(input, &result, '\0', &line, &col);

    const value* error;
    if ((error = find_error(result)) != NULL) {
        value* temp = value_new_error(error->symbol);
        value_dispose(result);
        result = temp;
    }

    return result;
}

value* parse_from_file(const char* path) {
    FILE* file = fopen(path, "r");
    if (file) {
        // open and read the file
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        char* content = malloc(length + 1);

        value* result;
        if (fread(content, 1, length, file)) {
            content[length] = '\0';
            // parse the full content of the file
            result = parse_from_str(content);
        } else {
            result = value_new_error("failed to read from file: %s", path);
        }

        // cleanup
        free(content);
        fclose(file);

        return result;
    } else {
        return value_new_error("failed to open file: %s", path);
    }
}

static value* recover_str_rec(value* v) {
    static char buffer[BUFFER_SIZE];

    if (v != NULL) {
        if (is_compound_type(v->type)) {
            v->car = recover_str_rec(v->car);
            v->cdr = recover_str_rec(v->cdr);
        }
        if (v != NULL &&
            v->type == VALUE_PAIR &&
            v->car != NULL &&
            v->car->type == VALUE_SYMBOL &&
            strcmp(v->car->symbol, QUOTE_SYMBOL) == 0 &&
            v->cdr != NULL) {
            // replace the (quote x) expression by
            // its string representation with 'x
            buffer[0] = QUOTE_CHAR;
            value_to_str(v->cdr->car, buffer + 1);

            // v is no longer needed,
            // as it gets substituted
            value_dispose(v);

            // replace v with str(v) = 'x
            v = value_new_symbol(buffer);
        }
    }

    return v;
}

int recover_str(value* v, char* buffer) {
    // temporary new value
    value* new_v = value_clone(v);
    new_v = recover_str_rec(new_v);
    int result = value_to_str(new_v, buffer);
    value_dispose(new_v);

    return result;
}
