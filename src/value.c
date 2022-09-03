#include "value.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "const.h"
#include "map.h"
#include "str.h"

static void value_init_number(value* v, double number) {
    assert(v != NULL);

    v->type = VALUE_NUMBER;
    v->number = number;
}

static void value_init_symbol(value* v, char* symbol) {
    assert(v != NULL);

    v->type = VALUE_SYMBOL;
    v->symbol = malloc(strlen(symbol) + 1);
    strcpy(v->symbol, symbol);
}

static void value_init_string(value* v, char* string) {
    assert(v != NULL);

    v->type = VALUE_STRING;
    v->symbol = malloc(strlen(string) + 1);
    strcpy(v->symbol, string);
}

static void value_init_bool(value* v, int truth) {
    assert(v != NULL);

    v->type = VALUE_BOOL;
    v->number = truth;
}

static void value_init_builtin(value* v, void* ptr, char* name) {
    assert(v != NULL);

    v->type = VALUE_BUILTIN;
    v->ptr = ptr;
    v->symbol = malloc(strlen(name) + 1);
    strcpy(v->symbol, name);
}

static void value_init_symbol_from_args(value* v, char* format, va_list args) {
    assert(v != NULL);

    if (args != NULL) {
        static char buffer[BUFFER_SIZE];
        vsnprintf(buffer, sizeof(buffer), format, args);
        value_init_symbol(v, buffer);
    } else {
        value_init_symbol(v, format);
    }
}

static void value_init_error_from_args(value* v, char* error, va_list args) {
    assert(v != NULL);

    value_init_symbol_from_args(v, error, args);
    v->type = VALUE_ERROR;
}

static void value_init_info_from_args(value* v, char* info, va_list args) {
    assert(v != NULL);

    value_init_symbol_from_args(v, info, args);
    v->type = VALUE_INFO;
}

static void value_init_pair(value* v, value* car, value* cdr) {
    assert(v != NULL);

    v->type = VALUE_PAIR;
    v->car = car;
    v->cdr = cdr;
}

static void value_init_lambda(value* v, value* car, value* cdr) {
    assert(v != NULL);

    v->type = VALUE_LAMBDA;
    v->car = car;
    v->cdr = cdr;
}

static void value_init_compiled(value* v, value* car, value* cdr) {
    assert(v != NULL);

    v->type = VALUE_COMPILED;
    v->car = car;
    v->cdr = cdr;
}

static void value_init_code(value* v, value* car, value* cdr) {
    assert(v != NULL);

    v->type = VALUE_CODE;
    v->car = car;
    v->cdr = cdr;
}

static void value_init_env(value* v) {
    assert(v != NULL);

    v->type = VALUE_ENV;
    v->ptr = map_new();
    v->car = NULL;
    v->cdr = NULL;
}

static value* value_new() {
    value* v = malloc(sizeof(value));

    v->gen = 0;
    v->next = NULL;

    return v;
}

value* value_new_number(double number) {
    value* v = value_new();
    value_init_number(v, number);

    return v;
}

value* value_new_symbol(char* symbol) {
    value* v = value_new();
    value_init_symbol(v, symbol);

    return v;
}

value* value_new_string(char* string) {
    value* v = value_new();
    value_init_string(v, string);

    return v;
}

value* value_new_bool(int truth) {
    value* v = value_new();
    value_init_bool(v, truth);

    return v;
}

value* value_new_builtin(void* ptr, char* name) {
    value* v = value_new();
    value_init_builtin(v, ptr, name);

    return v;
}

value* value_new_error(char* error, ...) {
    value* v = value_new();

    va_list args;
    va_start(args, error);
    value_init_error_from_args(v, error, args);
    va_end(args);

    return v;
}

value* value_new_error_from_args(char* error, va_list args) {
    value* v = value_new();
    value_init_error_from_args(v, error, args);

    return v;
}

value* value_new_info(char* info, ...) {
    value* v = value_new();

    va_list args;
    va_start(args, info);
    value_init_info_from_args(v, info, args);
    va_end(args);

    return v;
}

value* value_new_info_from_args(char* info, va_list args) {
    value* v = value_new();
    value_init_info_from_args(v, info, args);

    return v;
}

value* value_new_pair(value* car, value* cdr) {
    value* v = value_new();
    value_init_pair(v, car, cdr);

    return v;
}

value* value_new_lambda(value* car, value* cdr) {
    value* v = value_new();
    value_init_lambda(v, car, cdr);

    return v;
}

value* value_new_compiled(value* car, value* cdr) {
    value* v = value_new();
    value_init_compiled(v, car, cdr);

    return v;
}

value* value_new_code(value* car, value* cdr) {
    value* v = value_new();
    value_init_code(v, car, cdr);

    return v;
}

value* value_new_env() {
    value* v = value_new();
    value_init_env(v);

    return v;
}

int is_compound_type(value_type t) {
    return (
        t == VALUE_PAIR ||
        t == VALUE_LAMBDA ||
        t == VALUE_COMPILED ||
        t == VALUE_CODE ||
        t == VALUE_ENV);
}

char* get_type_name(value_type t) {
    switch (t) {
        case VALUE_NUMBER:
            return "number";
        case VALUE_SYMBOL:
            return "symbol";
        case VALUE_STRING:
            return "string";
        case VALUE_BOOL:
            return "bool";
        case VALUE_BUILTIN:
            return "builtin";
        case VALUE_ERROR:
            return "error";
        case VALUE_INFO:
            return "info";
        case VALUE_PAIR:
            return "pair";
        case VALUE_LAMBDA:
            return "lambda";
        case VALUE_COMPILED:
            return "compiled";
        case VALUE_CODE:
            return "code";
        case VALUE_ENV:
            return "env";
        default:
            return "unknown";
    }
}

void value_update_gen(value* v, size_t gen) {
    while (v != NULL && v->gen != gen) {
        v->gen = gen;
        if (is_compound_type(v->type)) {
            value_update_gen(v->car, gen);
            v = v->cdr;
        } else {
            break;
        }
    }
}

void value_cleanup(value* v) {
    if (v != NULL) {
        switch (v->type) {
            case VALUE_NUMBER:
            case VALUE_BOOL:
            case VALUE_PAIR:
            case VALUE_LAMBDA:
            case VALUE_COMPILED:
            case VALUE_CODE:
                break;
            case VALUE_SYMBOL:
            case VALUE_STRING:
            case VALUE_ERROR:
            case VALUE_INFO:
            case VALUE_BUILTIN:
                free(v->symbol);
                break;
            case VALUE_ENV:
                map_dispose((map*)v->ptr);
                break;
        }
    }
}

static void break_value_cycles(value* v) {
    while (v != NULL) {
        v->gen = -2;
        if (is_compound_type(v->type)) {
            if (v->car != NULL && v->car->gen == -2) {
                // break the cycle
                v->car = NULL;
            } else {
                break_value_cycles(v->car);
            }

            if (v->cdr != NULL && v->cdr->gen == -2) {
                // break the cycle
                v->cdr = NULL;
                break;
            } else {
                v = v->cdr;
            }
        } else {
            break;
        }
    }
}

static void value_dispose_rec(value* v) {
    if (v != NULL) {
        value_cleanup(v);

        if (is_compound_type(v->type)) {
            value_dispose_rec(v->car);
            value_dispose_rec(v->cdr);
        }

        free(v);
    }
}

void value_dispose(value* v) {
    value_update_gen(v, -1);
    break_value_cycles(v);
    value_dispose_rec(v);
}

static int number_to_str(value* v, char* buffer) {
    long num = (long)v->number;
    if (num == v->number) {
        return sprintf(buffer, "%ld", num);
    } else {
        return sprintf(buffer, "%.12g", v->number);
    }
}

static int string_to_str(value* v, char* buffer) {
    char* escaped = str_escape(v->symbol);
    int result = sprintf(buffer, "\"%s\"", escaped);
    free(escaped);

    return result;
}

static int bool_to_str(value* v, char* buffer) {
    if (v->number) {
        return sprintf(buffer, "true");
    } else {
        return sprintf(buffer, "false");
    }
}

static int builtin_to_str(value* v, char* buffer) {
    return sprintf(buffer, "<builtin '%s'>", v->symbol);
}

static int value_to_str_rec(value* v, char* buffer);

static int pair_to_str(value* v, char* buffer) {
    char* running = buffer;

    size_t depth = 0;
    value* v_running = v;
    running += sprintf(running, "(");
    running += value_to_str_rec(v->car, running);
    while ((v_running = v_running->cdr) != NULL) {
        if (v_running->type == VALUE_PAIR) {
            if (v_running->gen == -2) {
                // marked value: cycle
                running += sprintf(running, " . " CYCLE_MARK);
                break;
            } else {
                v_running->gen = -2;  // mark the value
                running += sprintf(running, " ");
                running += value_to_str_rec(v_running->car, running);
                depth += 1;  // increment the depth
            }
        } else {
            running += sprintf(running, " . ");
            running += value_to_str_rec(v_running, running);
            break;
        }
    }
    running += sprintf(running, ")");
    *running = '\0';

    // unmark the values marked in this call
    // as far as the depth incremented above
    v_running = v->cdr;
    for (size_t i = 0; i < depth; i++) {
        v_running->gen = -1;
        v_running = v_running->cdr;
    }

    return running - buffer;
}

static int lambda_to_str(value* v, char* buffer) {
    static char params[BUFFER_SIZE];
    static char body[BUFFER_SIZE];

    value_to_str_rec(v->car->car, params);
    value_to_str_rec(v->car->cdr, body);

    // drop outmost braces
    body[strlen(body) - 1] = '\0';

    return sprintf(buffer, "(lambda %s %s)", params, body + 1);
}

static int value_to_str_rec(value* v, char* buffer) {
    if (v == NULL) {
        return sprintf(buffer, "()");
    } else if (v->gen == -2) {
        // marked value: cycle
        return sprintf(buffer, CYCLE_MARK);
    } else {
        int result;
        switch (v->type) {
            case VALUE_NUMBER:
                result = number_to_str(v, buffer);
                break;
            case VALUE_SYMBOL:
                result = sprintf(buffer, "%s", v->symbol);
                break;
            case VALUE_STRING:
                result = string_to_str(v, buffer);
                break;
            case VALUE_BOOL:
                result = bool_to_str(v, buffer);
                break;
            case VALUE_BUILTIN:
                result = builtin_to_str(v, buffer);
                break;
            case VALUE_ERROR:
                result = sprintf(buffer, "\x1B[31m%s\x1B[0m", v->symbol);
                break;
            case VALUE_INFO:
                result = sprintf(buffer, "\x1B[32m%s\x1B[0m", v->symbol);
                break;
            case VALUE_PAIR:
                v->gen = -2;  // mark the value
                result = pair_to_str(v, buffer);
                v->gen = -1;  // unmark the value
                break;
            case VALUE_LAMBDA:
                result = lambda_to_str(v, buffer);
                break;
            default:
                result = sprintf(buffer, "<%s>", get_type_name(v->type));
                break;
        }
        return result;
    }
}

int value_to_str(value* v, char* buffer) {
    value_update_gen(v, -1);  // prepare
    int result = value_to_str_rec(v, buffer);
    value_update_gen(v, 0);  // clear

    return result;
}

static int value_to_pretty_str_rec(value* v, char* buffer, size_t line_len, int indent) {
    static char v_str[BUFFER_SIZE];
    value_to_str(v, v_str);

    if (v == NULL ||
        v->type != VALUE_PAIR ||
        strlen(v_str) <= line_len ||
        strstr(v_str, CYCLE_MARK)) {
        // the full expression in one line
        return sprintf(buffer, "%*s%s\n", indent * INDENT_SPACES, "", v_str);
    } else {
        char* running = buffer;

        // print the opening bracket
        running += sprintf(running, "%*s(\n", indent * INDENT_SPACES, "");

        while (v != NULL) {
            if (v->type == VALUE_PAIR) {
                // recursively print the car
                running += value_to_pretty_str_rec(v->car, running, line_len, indent + 1);
            } else {
                // print the dot
                running += sprintf(running, "%*s.\n", (indent + 1) * INDENT_SPACES, "");
                // recursively print the terminating cdr and break
                running += value_to_pretty_str_rec(v, running, line_len, indent + 1);
                break;
            }
            v = v->cdr;
        }

        // print the closing bracket
        running += sprintf(running, "%*s)\n", indent * INDENT_SPACES, "");

        return running - buffer;
    }
}

int value_to_pretty_str(value* v, char* buffer, size_t line_len) {
    int length = value_to_pretty_str_rec(v, buffer, line_len, 0);
    buffer[length - 1] = '\0';  // remove trailing \n

    return length - 1;
}

int value_is_true(value* v) {
    if (v == NULL) {
        return 0;
    } else {
        switch (v->type) {
            case VALUE_NUMBER:
            case VALUE_BOOL:
                return v->number != 0;
            case VALUE_BUILTIN:
                return v->ptr != NULL;
            case VALUE_SYMBOL:
            case VALUE_STRING:
                return strlen(v->symbol) > 0;
            default:
                return 1;
        }
    }
}

static int value_equal_rec(value* v1, value* v2) {
    if (v1 == v2) {
        return 1;
    } else if (v1 == NULL || v2 == NULL) {
        return 0;
    } else if (v1->gen == -2 || v2->gen == -2) {
        return 0;
    } else if (v1->type != v2->type) {
        return 0;
    } else {
        int result;
        switch (v1->type) {
            case VALUE_NUMBER:
            case VALUE_BOOL:
                result = v1->number == v2->number;
                break;
            case VALUE_BUILTIN:
                result = v1->ptr == v2->ptr;
                break;
            case VALUE_SYMBOL:
            case VALUE_STRING:
            case VALUE_ERROR:
            case VALUE_INFO:
                result = strcmp(v1->symbol, v2->symbol) == 0;
                break;
            case VALUE_PAIR:
            case VALUE_LAMBDA:
                v1->gen = -2;
                v2->gen = -2;
                result = (value_equal_rec(v1->car, v2->car) &&
                          value_equal_rec(v1->cdr, v2->cdr));
                v1->gen = -1;
                v2->gen = -1;
                break;
            default:
                result = 0;
                break;
        }
        return result;
    }
}

int value_equal(value* v1, value* v2) {
    value_update_gen(v1, -1);  // prepare
    value_update_gen(v2, -1);  // prepare
    int result = value_equal_rec(v1, v2);
    value_update_gen(v1, 0);  // clear
    value_update_gen(v2, 0);  // clear

    return result;
}

static void value_copy(value* dest, value* source) {
    assert(dest != NULL);
    assert(source != NULL);

    switch (source->type) {
        case VALUE_NUMBER:
            value_init_number(dest, source->number);
            break;
        case VALUE_SYMBOL:
            value_init_symbol(dest, source->symbol);
            break;
        case VALUE_STRING:
            value_init_string(dest, source->symbol);
            break;
        case VALUE_BOOL:
            value_init_bool(dest, source->number);
            break;
        case VALUE_BUILTIN:
            value_init_builtin(dest, source->ptr, source->symbol);
            break;
        case VALUE_ERROR:
            value_init_error_from_args(dest, source->symbol, NULL);
            break;
        case VALUE_INFO:
            value_init_info_from_args(dest, source->symbol, NULL);
            break;
        case VALUE_PAIR:
            value_init_pair(dest, source->car, source->cdr);
            break;
        case VALUE_LAMBDA:
            value_init_lambda(dest, source->car, source->cdr);
            break;
        case VALUE_COMPILED:
            value_init_compiled(dest, source->car, source->cdr);
            break;
        case VALUE_CODE:
        case VALUE_ENV:
            // can't copy env or code
            assert(0);
            break;
    }
}

static value* value_clone_rec(value* source) {
    if (source == NULL) {
        return NULL;
    } else if (source->type == VALUE_ENV || source->type == VALUE_CODE) {
        // envs and code are not allowed to be cloned
        // (maybe should be allowed in the future)
        return NULL;
    } else if (source->gen != -1) {
        // "broken heart": already cloned
        return (value*)source->gen;
    } else {
        // empty new value
        value* dest = value_new();

        // shallow copy from source
        value_copy(dest, source);

        // temporarily setup a "broken heart"
        // to avoid cycles in the recursion
        source->gen = (size_t)dest;

        if (is_compound_type(dest->type)) {
            // deep copy through the recursive calls
            dest->car = value_clone_rec(dest->car);
            dest->cdr = value_clone_rec(dest->cdr);
        }

        return dest;
    }
}

value* value_clone(value* source) {
    value_update_gen(source, -1);  // prepare
    value* dest = value_clone_rec(source);
    value_update_gen(source, 0);  // clear

    return dest;
}
