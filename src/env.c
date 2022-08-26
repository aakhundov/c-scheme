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

void env_add(value* env, char* name, value* v, pool* p) {
    map_add((map*)env->ptr, name, v);

    // to keep a tracable link to the val during GC
    env->car = pool_new_pair(p, v, env->car);
}

value* env_extend(value* env, value* parent_env) {
    env->cdr = parent_env;

    return env;
}
