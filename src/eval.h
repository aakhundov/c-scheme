#ifndef EVAL_H_
#define EVAL_H_

#include "machine.h"
#include "value.h"

typedef struct eval eval;

struct eval {
    machine* machine;
    value* env;
};

eval* eval_new(const char* path_to_code);
void eval_dispose(eval* e);

value* eval_evaluate(eval* e, value* v);
void eval_reset_env(eval* e);

#endif  // EVAL_H_
