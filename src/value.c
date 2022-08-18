#include "value.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

void value_init_number(value* v, double number) {
    assert(v != NULL);

    v->type = VALUE_NUMBER;
    v->number = number;
}

void value_init_symbol(value* v, char* symbol) {
    assert(v != NULL);

    v->type = VALUE_SYMBOL;
    v->symbol = malloc(strlen(symbol) + 1);
    strcpy(v->symbol, symbol);
}

void value_init_string(value* v, char* string) {
    assert(v != NULL);

    v->type = VALUE_STRING;
    v->symbol = malloc(strlen(string) + 1);
    strcpy(v->symbol, string);
}

static void value_init_symbol_from_args(value* v, char* format, va_list args) {
    assert(v != NULL);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    value_init_symbol(v, buffer);
}

void value_init_error_from_args(value* v, char* error, va_list args) {
    assert(v != NULL);

    value_init_symbol_from_args(v, error, args);
    v->type = VALUE_ERROR;
}

void value_init_error(value* v, char* error, ...) {
    assert(v != NULL);

    va_list args;
    va_start(args, error);
    value_init_error_from_args(v, error, args);
    va_end(args);
}

void value_init_info_from_args(value* v, char* info, va_list args) {
    assert(v != NULL);

    value_init_symbol_from_args(v, info, args);
    v->type = VALUE_INFO;
}

void value_init_info(value* v, char* info, ...) {
    assert(v != NULL);

    va_list args;
    va_start(args, info);
    value_init_info_from_args(v, info, args);
    va_end(args);
}

void value_init_pair(value* v, value* car, value* cdr) {
    assert(v != NULL);

    v->type = VALUE_PAIR;
    v->car = car;
    v->cdr = cdr;
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

void value_cleanup(value* v) {
    if (v != NULL) {
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
}

void value_dispose(value* v) {
    if (v != NULL) {
        value_cleanup(v);

        if (v->type == VALUE_PAIR) {
            value_dispose(v->car);
            value_dispose(v->cdr);
        }

        free(v);
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

int value_to_bool(value* v) {
    if (v == NULL) {
        return 0;
    } else {
        switch (v->type) {
            case VALUE_NUMBER:
                return v->number != 0;
            case VALUE_SYMBOL:
            case VALUE_STRING:
                return strlen(v->symbol) > 0;
            default:
                return 1;
        }
    }
}

int value_to_str(value* v, char* buffer) {
    if (v == NULL) {
        return sprintf(buffer, "()");
    } else {
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
                return sprintf(buffer, "<unknown type: %d>", v->type);
        }
    }
}

void value_copy(value* dest, value* source) {
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
    if (source == NULL) {
        return NULL;
    } else {
        value* dest = value_new();
        value_copy(dest, source);

        if (dest->type == VALUE_PAIR) {
            dest->car = value_clone(dest->car);
            dest->cdr = value_clone(dest->cdr);
        }

        return dest;
    }
}
