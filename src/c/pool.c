#include "pool.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

#include "value.h"

static void add_to_chain(pool* p, value* v) {
    v->gen = p->gen;
    v->next = p->chain->next;
    p->chain->next = v;
    p->size++;
}

static void add_to_chain_rec(pool* p, value* v) {
    if (v != NULL && v->gen != p->gen) {
        add_to_chain(p, v);

        if (is_compound_type(v->type)) {
            add_to_chain_rec(p, v->car);
            add_to_chain_rec(p, v->cdr);
        }
    }
}

static void sweep_chain(pool* p) {
    value* prev = p->chain;
    value* curr = p->chain->next;
    while (curr != NULL) {
        if (curr->gen == p->gen) {
            // used value: keep
            prev = curr;
        } else {
            // useless value: sweep
            prev->next = curr->next;
            // only cleanup, not dispose:
            // to avoid recursive dispose
            value_cleanup(curr);
            // free manually
            free(curr);
            p->size--;
        }
        curr = prev->next;
    }
}

pool* pool_new() {
    pool* p = malloc(sizeof(pool));

    p->size = 0;
    p->gen = 1;
    p->roots = NULL;
    p->chain = value_new_number(0);  // dummy head value
    p->chain->next = NULL;           // initially empty chain

    return p;
}

void pool_dispose(pool* p) {
    // collect the entire chain
    // because no values are marked
    p->gen++;
    sweep_chain(p);

    // unlink externally set roots
    // not to dispose them recursively
    value* pair = p->roots;
    while (pair != NULL) {
        pair->car = NULL;
        pair = pair->cdr;
    }

    value_dispose(p->roots);
    value_dispose(p->chain);

    free(p);
}

void pool_register_root(pool* p, value* root) {
    assert(root != NULL);

    p->roots = value_new_pair(root, p->roots);
}

void pool_unregister_root(pool* p, value* root) {
    value* prev = NULL;
    value* curr = p->roots;
    while (curr != NULL) {
        if (curr->car == root) {
            if (prev == NULL) {
                p->roots = curr->cdr;
            } else {
                prev->cdr = curr->cdr;
            }

            // manually free the
            // root container pair
            curr->car = NULL;
            curr->cdr = NULL;
            value_dispose(curr);
            break;
        } else {
            prev = curr;
            curr = curr->cdr;
        }
    }
}

void pool_collect_garbage(pool* p) {
    p->gen++;

    value* pair = p->roots;
    while (pair != NULL) {
        value* root = pair->car;
        value_update_gen(root, p->gen);
        pair = pair->cdr;
    }

    sweep_chain(p);
}

value* pool_new_number(pool* p, const double number) {
    value* v = value_new_number(number);

    add_to_chain(p, v);

    return v;
}

value* pool_new_symbol(pool* p, const char* symbol) {
    value* v = value_new_symbol(symbol);

    add_to_chain(p, v);

    return v;
}

value* pool_new_string(pool* p, const char* string) {
    value* v = value_new_string(string);

    add_to_chain(p, v);

    return v;
}

value* pool_new_bool(pool* p, const int truth) {
    value* v = value_new_bool(truth);

    add_to_chain(p, v);

    return v;
}

value* pool_new_primitive(pool* p, void* ptr, const char* name) {
    value* v = value_new_primitive(ptr, name);

    add_to_chain(p, v);

    return v;
}

value* pool_new_error(pool* p, const char* error, ...) {
    va_list args;
    va_start(args, error);
    value* v = value_new_error_from_args(error, args);
    va_end(args);

    add_to_chain(p, v);

    return v;
}

value* pool_new_error_from_args(pool* p, const char* error, va_list args) {
    value* v = value_new_error_from_args(error, args);

    add_to_chain(p, v);

    return v;
}

value* pool_new_info(pool* p, const char* info, ...) {
    va_list args;
    va_start(args, info);
    value* v = value_new_info_from_args(info, args);
    va_end(args);

    add_to_chain(p, v);

    return v;
}

value* pool_new_info_from_args(pool* p, const char* info, va_list args) {
    value* v = value_new_info_from_args(info, args);

    add_to_chain(p, v);

    return v;
}

value* pool_new_pair(pool* p, value* car, value* cdr) {
    value* v = value_new_pair(car, cdr);

    add_to_chain(p, v);

    return v;
}

value* pool_new_lambda(pool* p, value* car, value* cdr) {
    value* v = value_new_lambda(car, cdr);

    add_to_chain(p, v);

    return v;
}

value* pool_new_compiled(pool* p, value* car, value* cdr) {
    value* v = value_new_compiled(car, cdr);

    add_to_chain(p, v);

    return v;
}

value* pool_new_code(pool* p, value* car, value* cdr) {
    value* v = value_new_code(car, cdr);

    add_to_chain(p, v);

    return v;
}

value* pool_new_env(pool* p) {
    value* v = value_new_env();

    add_to_chain(p, v);

    return v;
}

value* pool_import(pool* p, value* source) {
    value* dest = value_clone(source);

    add_to_chain_rec(p, dest);

    return dest;
}

value* pool_export(pool* p, value* source) {
    return value_clone(source);
}
