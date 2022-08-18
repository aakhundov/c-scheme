#ifndef MACHINE_H_
#define MACHINE_H_

#include "pool.h"
#include "value.h"

typedef struct machine machine;
typedef value* (*builtin)(machine* m, value* args);

struct machine {
    pool* pool;
    value* root;

    value* registers;
    value* constants;
    value* labels;
    value* code;
    value* ops;

    value* stack;
    value* flag;
    value* pc;
    value* val;
};

void machine_init(machine* m, value* code, char* output_register_name);
void machine_cleanup(machine* m);

void machine_add_op(machine* m, char* name, builtin op);
void machine_write_to_register(machine* m, char* name, value* v);
value* machine_read_from_register(machine* m, char* name);
value* machine_read_output(machine* m);
void machine_run(machine* m);

#endif  // MACHINE_H_
