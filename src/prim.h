#ifndef PRIM_H_
#define PRIM_H_

#include "machine.h"

void init_primitives();
void cleanup_primitives();

int is_primitive(char* name);
value* get_primitive(char* name);

#endif  // PRIM_H_
