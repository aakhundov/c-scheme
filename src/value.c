#include "value.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

void value_init_number(value* v, double number) {
    v->type = VALUE_NUMBER;
    v->number = number;
}

void value_init_symbol(value* v, char* symbol) {
    v->type = VALUE_SYMBOL;
    v->symbol = malloc(strlen(symbol) + 1);
    strcpy(v->symbol, symbol);
}

void value_init_string(value* v, char* string) {
    v->type = VALUE_STRING;
    v->symbol = malloc(strlen(string) + 1);
    strcpy(v->symbol, string);
}

static void value_init_symbol_from_args(value* v, char* format, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    value_init_symbol(v, buffer);
}

void value_init_error_from_args(value* v, char* error, va_list args) {
    value_init_symbol_from_args(v, error, args);
    v->type = VALUE_ERROR;
}

void value_init_error(value* v, char* error, ...) {
    va_list args;
    va_start(args, error);
    value_init_error_from_args(v, error, args);
    va_end(args);
}

void value_init_info_from_args(value* v, char* info, va_list args) {
    value_init_symbol_from_args(v, info, args);
    v->type = VALUE_INFO;
}

void value_init_info(value* v, char* info, ...) {
    va_list args;
    va_start(args, info);
    value_init_info_from_args(v, info, args);
    va_end(args);
}

void value_init_pair(value* v, value* car, value* cdr) {
    v->type = VALUE_PAIR;
    v->car = car;
    v->cdr = cdr;
}

void value_init_null_pair(value* v) {
    value_init_pair(v, NULL, NULL);
}

value* value_new_number(double number) {
    value* v = malloc(sizeof(value));
    value_init_number(v, number);

    return v;
}

value* value_new_symbol(char* symbol) {
    value* v = malloc(sizeof(value));
    value_init_symbol(v, symbol);

    return v;
}

value* value_new_string(char* string) {
    value* v = malloc(sizeof(value));
    value_init_string(v, string);

    return v;
}

value* value_new_error(char* error, ...) {
    value* v = malloc(sizeof(value));

    va_list args;
    va_start(args, error);
    value_init_error_from_args(v, error, args);
    va_end(args);

    return v;
}

value* value_new_error_from_args(char* error, va_list args) {
    value* v = malloc(sizeof(value));
    value_init_error_from_args(v, error, args);

    return v;
}

value* value_new_info(char* info, ...) {
    value* v = malloc(sizeof(value));

    va_list args;
    va_start(args, info);
    value_init_info_from_args(v, info, args);
    va_end(args);

    return v;
}

value* value_new_info_from_args(char* info, va_list args) {
    value* v = malloc(sizeof(value));
    value_init_info_from_args(v, info, args);

    return v;
}

value* value_new_pair(value* car, value* cdr) {
    value* v = malloc(sizeof(value));
    value_init_pair(v, car, cdr);

    return v;
}

value* value_new_null_pair() {
    value* v = malloc(sizeof(value));
    value_init_null_pair(v);

    return v;
}

void value_cleanup(value* v) {
    switch (v->type) {
        case VALUE_NUMBER:
        case VALUE_PAIR:
            break;
        case VALUE_SYMBOL:
        case VALUE_STRING:
        case VALUE_ERROR:
        case VALUE_INFO:
            free(v->symbol);
            break;
    }
}

void value_dispose(value* v) {
    if (v->type == VALUE_PAIR) {
        if (v->car != NULL) {
            value_dispose(v->car);
        }
        if (v->cdr != NULL) {
            value_dispose(v->cdr);
        }
    } else {
        value_cleanup(v);
    }

    free(v);
}

int value_is_null_pair(value* v) {
    if (v->type == VALUE_PAIR && v->car == NULL && v->cdr == NULL) {
        return 1;
    } else {
        return 0;
    }
}

static int string_to_str(value* v, char* buffer) {
    char* escaped = str_escape(v->symbol);
    int result = sprintf(buffer, "\"%s\"", escaped);
    free(escaped);

    return result;
}

static int pair_to_str(value* v, char* buffer) {
    char* running = buffer;

    running += sprintf(running, "(");
    if (!value_is_null_pair(v)) {
        while (1) {
            running += value_to_str(v->car, running);

            if (v->cdr->type == VALUE_PAIR) {
                if (!value_is_null_pair(v->cdr)) {
                    running += sprintf(running, " ");
                    v = v->cdr;
                } else {
                    break;
                }
            } else {
                running += sprintf(running, " . ");
                running += value_to_str(v->cdr, running);
                break;
            }
        }
    }
    running += sprintf(running, ")");
    *running = '\0';

    return running - buffer;
}

int value_to_str(value* v, char* buffer) {
    switch (v->type) {
        case VALUE_NUMBER:
            return sprintf(buffer, "%g", v->number);
        case VALUE_SYMBOL:
            return sprintf(buffer, "%s", v->symbol);
        case VALUE_STRING:
            return string_to_str(v, buffer);
        case VALUE_ERROR:
            return sprintf(buffer, "\x1B[31m%s\x1B[0m", v->symbol);
        case VALUE_INFO:
            return sprintf(buffer, "\x1B[32m%s\x1B[0m", v->symbol);
        case VALUE_PAIR:
            return pair_to_str(v, buffer);
        default:
            return sprintf(buffer, "unknown value type: %d", v->type);
    }
}

value* value_add_child(value* parent, value* child) {
    parent->car = child;
    parent->cdr = value_new_null_pair();

    return parent->cdr;
}

void value_copy(value* dest, value* source) {
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
        case VALUE_ERROR:
            value_init_error(dest, source->symbol);
            break;
        case VALUE_INFO:
            value_init_info(dest, source->symbol);
            break;
        case VALUE_PAIR:
            value_init_pair(dest, source->car, source->cdr);
            break;
    }
}

value* value_clone(value* source) {
    value* dest = malloc(sizeof(value));
    value_copy(dest, source);

    if (dest->type == VALUE_PAIR) {
        dest->car = value_clone(dest->car);
        dest->cdr = value_clone(dest->cdr);
    }

    return dest;
}
