#include "value.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
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
        case VALUE_CODE:
            return "code";
        case VALUE_ENV:
            return "env";
        default:
            return "unknown";
    }
}

void value_update_gen(value* v, size_t gen) {
    if (v != NULL && v->gen != gen) {
        v->gen = gen;
        if (is_compound_type(v->type)) {
            value_update_gen(v->car, gen);
            value_update_gen(v->cdr, gen);
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

static void break_cycles(value* v) {
    if (v != NULL) {
        v->gen = -1;
        if (is_compound_type(v->type)) {
            if (v->car != NULL && v->car->gen == -1) {
                // break the cycle
                v->car = NULL;
            } else {
                break_cycles(v->car);
            }

            if (v->cdr != NULL && v->cdr->gen == -1) {
                // break the cycle
                v->cdr = NULL;
            } else {
                break_cycles(v->cdr);
            }
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
    value_update_gen(v, 0);
    break_cycles(v);
    value_dispose_rec(v);
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

int value_equal(value* v1, value* v2) {
    if (v1 == NULL && v2 == NULL) {
        return 1;
    } else if (v1 == NULL || v2 == NULL) {
        return 0;
    } else if (v1->type != v2->type) {
        return 0;
    } else {
        switch (v1->type) {
            case VALUE_NUMBER:
            case VALUE_BOOL:
                return v1->number == v2->number;
            case VALUE_BUILTIN:
                return v1->ptr == v2->ptr;
            case VALUE_SYMBOL:
            case VALUE_STRING:
            case VALUE_ERROR:
            case VALUE_INFO:
                return strcmp(v1->symbol, v2->symbol) == 0;
            case VALUE_PAIR:
            case VALUE_LAMBDA:
                // may run into cycles
                return (
                    value_equal(v1->car, v2->car) &&
                    value_equal(v1->cdr, v2->cdr));
            default:
                return v1 == v2;
        }
    }
}

static int number_to_str(value* v, char* buffer) {
    long num = (long)v->number;
    if (num == v->number) {
        return sprintf(buffer, "%ld", num);
    } else {
        return sprintf(buffer, "%g", v->number);
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

static int pair_to_str(value* v, char* buffer) {
    char* running = buffer;

    running += sprintf(running, "(");
    running += value_to_str(v->car, running);
    while ((v = v->cdr) != NULL) {
        if (v->type == VALUE_PAIR) {
            running += sprintf(running, " ");
            running += value_to_str(v->car, running);
        } else {
            running += sprintf(running, " . ");
            running += value_to_str(v, running);
            break;
        }
    }
    running += sprintf(running, ")");
    *running = '\0';

    return running - buffer;
}

static int lambda_to_str(value* v, char* buffer) {
    static char params[BUFFER_SIZE];
    static char body[BUFFER_SIZE];

    value_to_str(v->car->car, params);
    value_to_str(v->car->cdr, body);

    // drop outmost braces
    body[strlen(body) - 1] = '\0';

    return sprintf(buffer, "(lambda %s %s)", params, body + 1);
}

int value_to_str(value* v, char* buffer) {
    if (v == NULL) {
        return sprintf(buffer, "()");
    } else {
        switch (v->type) {
            case VALUE_NUMBER:
                return number_to_str(v, buffer);
            case VALUE_SYMBOL:
                return sprintf(buffer, "%s", v->symbol);
            case VALUE_STRING:
                return string_to_str(v, buffer);
            case VALUE_BOOL:
                return bool_to_str(v, buffer);
            case VALUE_BUILTIN:
                return builtin_to_str(v, buffer);
            case VALUE_ERROR:
                return sprintf(buffer, "\x1B[31m%s\x1B[0m", v->symbol);
            case VALUE_INFO:
                return sprintf(buffer, "\x1B[32m%s\x1B[0m", v->symbol);
            case VALUE_PAIR:
                return pair_to_str(v, buffer);
            case VALUE_LAMBDA:
                return lambda_to_str(v, buffer);
            default:
                return sprintf(buffer, "<%s>", get_type_name(v->type));
        }
    }
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
    } else if (source->gen != 0) {
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
    size_t old_gen = (source != NULL ? source->gen : 0);

    value_update_gen(source, 0);
    value* dest = value_clone_rec(source);
    value_update_gen(source, old_gen);

    return dest;
}
