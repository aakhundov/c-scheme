#ifndef COMP_H_
#define COMP_H_

#include "pool.h"
#include "value.h"

value* compile(pool* p, value* exp, const char* target, const char* linkage);

#endif  // COMP_H_
