#ifndef PARSE_H_
#define PARSE_H_

#include "value.h"

value* parse_from_str(char* input);
value* parse_from_file(char* path);

int recover_str(value* v, char* buffer);

#endif  // PARSE_H_
