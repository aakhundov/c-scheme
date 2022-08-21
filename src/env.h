#ifndef ENV_H_
#define ENV_H_

#include "pool.h"
#include "value.h"

value* lookup_in_env(value* env, char* name, int recursive);
void add_to_env(value* env, char* name, value* v, pool* p);
value* extend_env(value* env, value* parent_env);

#endif  // ENV_H_
