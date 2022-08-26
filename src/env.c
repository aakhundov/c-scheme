#include "env.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "map.h"
#include "pool.h"
#include "value.h"

map_record* env_lookup(value* env, char* name, int recursive) {
    while (env != NULL) {
        map_record* r = map_get((map*)env->ptr, name);

        if (r != NULL) {
            return r;
        } else if (recursive) {
            env = env->cdr;
        } else {
            env = NULL;
        }
    }

    return NULL;
}

value* env_get_value(map_record* r) {
    return r->val->car;
}

void env_update_value(map_record* r, value* v) {
    r->val->car = v;
}

void env_add_value(value* env, char* name, value* v, pool* p) {
    // to keep a tracable link to the val during GC
    env->car = pool_new_pair(p, v, env->car);
    map_add((map*)env->ptr, name, env->car);
}

value* env_extend(value* env, value* parent_env) {
    env->cdr = parent_env;

    return env;
}
