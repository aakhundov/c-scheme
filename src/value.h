#ifndef VALUE_H_
#define VALUE_H_

#include <stdarg.h>
#include <stdlib.h>

typedef enum {
    VALUE_NUMBER = 0,
    VALUE_SYMBOL = 1,
    VALUE_STRING = 2,
    VALUE_BOOL = 3,
    VALUE_ERROR = 4,
    VALUE_INFO = 5,
    VALUE_PAIR = 6,
    VALUE_PRIMITIVE = 7,
    VALUE_LAMBDA = 8,
    VALUE_COMPILED = 9,
    VALUE_CODE = 10,
    VALUE_ENV = 11
} value_type;

typedef struct value value;

struct value {
    value_type type;

    double number;
    char* symbol;
    void* ptr;

    value* car;
    value* cdr;

    value* next;
    size_t gen;
};

value* value_new_number(double number);
value* value_new_symbol(char* symbol);
value* value_new_string(char* string);
value* value_new_bool(int truth);
value* value_new_primitive(void* ptr, char* name);
value* value_new_error(char* error, ...);
value* value_new_error_from_args(char* error, va_list args);
value* value_new_info(char* info, ...);
value* value_new_info_from_args(char* info, va_list args);
value* value_new_pair(value* car, value* cdr);
value* value_new_lambda(value* car, value* cdr);
value* value_new_compiled(value* car, value* cdr);
value* value_new_code(value* car, value* cdr);
value* value_new_env();

void value_cleanup(value* v);  // without free
void value_dispose(value* v);  // with free
void value_update_gen(value* v, size_t gen);

int is_compound_type(value_type t);
char* get_type_name(value_type t);

int value_to_str(value* v, char* buffer);
int value_to_pretty_str(value* v, char* buffer, size_t line_len);

int value_is_true(value* v);
int value_equal(value* v1, value* v2);
value* value_clone(value* source);

#endif  // VALUE_H_
