#ifndef EVAL_H_
#define EVAL_H_

#include "machine.h"
#include "value.h"

typedef struct eval eval;

struct eval {
    machine* machine;
    value* env;
};

void eval_init(eval* e, char* path_to_code);
void eval_cleanup(eval* e);

value* eval_evaluate(eval* e, value* v);
void eval_reset_env(eval* e);

#endif  // EVAL_H_
