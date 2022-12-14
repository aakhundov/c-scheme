#ifndef ENV_H_
#define ENV_H_

#include "map.h"
#include "pool.h"
#include "value.h"

map_record* env_lookup(const value* env, const char* name, const int recursive);

value* env_get_value(const map_record* r);
void env_update_value(map_record* r, value* v);
void env_add_value(value* env, const char* name, value* v, pool* p);

value* env_extend(value* env, value* parent_env);

#endif  // ENV_H_
