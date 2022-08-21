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
    VALUE_BUILTIN = 7,
    VALUE_LAMBDA = 8,
    VALUE_CODE = 9,
    VALUE_ENV = 10
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

// without malloc
void value_init_number(value* v, double number);
void value_init_symbol(value* v, char* symbol);
void value_init_string(value* v, char* string);
void value_init_bool(value* v, int truth);
void value_init_builtin(value* v, void* ptr, char* name);
void value_init_error(value* v, char* error, ...);
void value_init_error_from_args(value* v, char* error, va_list args);
void value_init_info(value* v, char* info, ...);
void value_init_info_from_args(value* v, char* info, va_list args);
void value_init_pair(value* v, value* car, value* cdr);
void value_init_lambda(value* v, value* car, value* cdr);
void value_init_code(value* v, value* car, value* cdr);
void value_init_env(value* v, value* car, value* cdr);

// with malloc
value* value_new_number(double number);
value* value_new_symbol(char* symbol);
value* value_new_string(char* string);
value* value_new_bool(int truth);
value* value_new_builtin(void* ptr, char* name);
value* value_new_error(char* error, ...);
value* value_new_error_from_args(char* error, va_list args);
value* value_new_info(char* info, ...);
value* value_new_info_from_args(char* info, va_list args);
value* value_new_pair(value* car, value* cdr);
value* value_new_lambda(value* car, value* cdr);
value* value_new_code(value* car, value* cdr);
value* value_new_env();

void value_cleanup(value* v);  // without free
void value_dispose(value* v);  // with free

int is_compound_type(value_type t);
char* get_type_name(value_type t);

int value_is_true(value* v);
int value_equal(value* v1, value* v2);
int value_to_str(value* v, char* buffer);
void value_copy(value* dest, value* source);  // without malloc (shallow copy)
value* value_clone(value* source);            // with malloc (deep copy)

#endif  // VALUE_H_
