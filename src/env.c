#include "env.h"

#include <stdio.h>
#include <string.h>

#include "pool.h"
#include "value.h"

value* lookup_in_env(value* env, char* name, int recursive) {
    if (env == NULL) {
        return NULL;
    } else {
        value* pair = env->car;
        while (pair != NULL) {
            value* record = pair->car;
            if (strcmp(record->car->symbol, name) == 0) {
                return record;
            }
            pair = pair->cdr;
        }

        if (recursive) {
            return lookup_in_env(env->cdr, name, recursive);
        } else {
            return NULL;
        }
    }
}

void add_to_env(value* env, char* name, value* v, pool* p) {
    value* record = pool_new_pair(p, pool_new_symbol(p, name), v);
    env->car = pool_new_pair(p, record, env->car);
}

value* extend_env(value* env, value* parent_env) {
    env->cdr = parent_env;

    return env;
}
