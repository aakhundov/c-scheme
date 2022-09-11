#ifndef PRIM_H_
#define PRIM_H_

#include "value.h"

void init_primitives();
void cleanup_primitives();

int is_primitive(const char* name);
value* get_primitive(const char* name);

#endif  // PRIM_H_
