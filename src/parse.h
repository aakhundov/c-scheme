#ifndef PARSE_H_
#define PARSE_H_

#include "value.h"

value* parse_from_str(char* input);
value* parse_from_file(char* path);

#endif  // PARSE_H_
