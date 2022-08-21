#ifndef MACHINE_H_
#define MACHINE_H_

#include "pool.h"
#include "value.h"

typedef struct machine machine;
typedef struct machine_stats machine_stats;
typedef value* (*builtin)(machine* m, value* args);

struct machine_stats {
    size_t num_inst;
    size_t num_inst_stack;
    size_t num_inst_op_call;
    size_t stack_depth;
    size_t stack_depth_max;
    size_t garbage_before;
    size_t garbage_after;
};

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
    value* trace;

    machine_stats stats;
};

void machine_new(machine** m, value* code, char* output_register_name);
void machine_dispose(machine** m);

void machine_set_trace(machine* m, int on);
void machine_bind_op(machine* m, char* name, builtin fn);
void machine_copy_to_register(machine* m, char* name, value* v);
value* machine_copy_from_register(machine* m, char* name);
value* machine_get_register(machine* m, char* name);
value* machine_export_output(machine* m);
void machine_run(machine* m);

#endif  // MACHINE_H_
