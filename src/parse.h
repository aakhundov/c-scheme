#ifndef PARSE_H_
#define PARSE_H_

#include "value.h"

value* parse_from_str(const char* input);
value* parse_from_file(const char* path);

int recover_str(value* v, char* buffer);

#endif  // PARSE_H_
