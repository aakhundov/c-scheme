#ifndef ENV_H_
#define ENV_H_

#include "map.h"
#include "pool.h"
#include "value.h"

map_record* env_lookup(value* env, char* name, int recursive);
void env_add(value* env, char* name, value* v, pool* p);
value* env_extend(value* env, value* parent_env);

#endif  // ENV_H_
