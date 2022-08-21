#ifndef POOL_H_
#define POOL_H_

#include <stdlib.h>

#include "value.h"

typedef struct pool pool;

struct pool {
    value* roots;
    value* chain;
    size_t size;
    size_t gen;
};

void pool_init(pool* p);
void pool_cleanup(pool* p);

void pool_register_root(pool* p, value* root);
void pool_unregister_root(pool* p, value* root);
void pool_collect_garbage(pool* p);

value* pool_new_number(pool* p, double number);
value* pool_new_symbol(pool* p, char* symbol);
value* pool_new_string(pool* p, char* string);
value* pool_new_bool(pool* p, int truth);
value* pool_new_builtin(pool* p, void* ptr);
value* pool_new_error(pool* p, char* error, ...);
value* pool_new_error_from_args(pool* p, char* error, va_list args);
value* pool_new_info(pool* p, char* info, ...);
value* pool_new_info_from_args(pool* p, char* info, va_list args);
value* pool_new_pair(pool* p, value* car, value* cdr);
value* pool_new_lambda(pool* p, value* car, value* cdr);
value* pool_new_code(pool* p, value* car, value* cdr);
value* pool_new_env(pool* p);

value* pool_import(pool* p, value* source);
value* pool_export(pool* p, value* source);

#endif  // POOL_H_
