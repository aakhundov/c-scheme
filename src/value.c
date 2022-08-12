#include "value.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

value* value_new_number(double number) {
    value* v = malloc(sizeof(value));

    v->type = VALUE_NUMBER;
    v->number = number;

    return v;
}

value* value_new_symbol(char* symbol) {
    value* v = malloc(sizeof(value));

    v->type = VALUE_SYMBOL;
    v->symbol = malloc(strlen(symbol) + 1);
    strcpy(v->symbol, symbol);

    return v;
}

static value* value_new_symbol_from_args(char* format, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);

    return value_new_symbol(buffer);
}

value* value_new_error_from_args(char* error, va_list args) {
    value* v = value_new_symbol_from_args(error, args);
    v->type = VALUE_ERROR;

    return v;
}

value* value_new_error(char* error, ...) {
    va_list args;
    va_start(args, error);
    value* v = value_new_error_from_args(error, args);
    va_end(args);

    return v;
}

value* value_new_info_from_args(char* info, va_list args) {
    value* v = value_new_symbol_from_args(info, args);
    v->type = VALUE_INFO;

    return v;
}

value* value_new_info(char* info, ...) {
    va_list args;
    va_start(args, info);
    value* v = value_new_info_from_args(info, args);
    va_end(args);

    return v;
}

value* value_new_string(char* string) {
    value* v = malloc(sizeof(value));

    v->type = VALUE_STRING;
    v->symbol = malloc(strlen(string) + 1);
    strcpy(v->symbol, string);

    return v;
}

value* value_new_pair(value* car, value* cdr) {
    value* v = malloc(sizeof(value));

    v->type = VALUE_PAIR;
    v->car = car;
    v->cdr = cdr;

    return v;
}

value* value_new_null_pair() {
    return value_new_pair(NULL, NULL);
}

void value_dispose(value* v) {
    switch (v->type) {
        case VALUE_NUMBER:
            break;
        case VALUE_SYMBOL:
        case VALUE_STRING:
        case VALUE_ERROR:
        case VALUE_INFO:
            free(v->symbol);
            break;
        case VALUE_PAIR:
            if (v->car != NULL) {
                value_dispose(v->car);
            }
            if (v->cdr != NULL) {
                value_dispose(v->cdr);
            }
            break;
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
