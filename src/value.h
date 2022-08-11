#ifndef VALUE_H_
#define VALUE_H_

#include <stdarg.h>

typedef enum {
    VALUE_NUMBER = 0,
    VALUE_SYMBOL = 1,
    VALUE_ERROR = 3,
    VALUE_PAIR = 4
} value_type;

typedef struct value value;

struct value {
    value_type type;

    double number;
    char* symbol;

    value* car;
    value* cdr;
};

value* value_new_number(double number);
value* value_new_symbol(char* symbol);
value* value_new_error(char* error, ...);
value* value_new_error_from_args(char* error, va_list args);
value* value_new_pair(value* car, value* cdr);
value* value_new_null();

void value_dispose(value* v);

int value_is_null(value* v);
int value_to_str(value* v, char* buffer);
value* value_add_child(value* parent, value* child);

#endif  // VALUE_H_
