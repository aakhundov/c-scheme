#ifndef MACHINE_H_
#define MACHINE_H_

#include "pool.h"
#include "value.h"

typedef struct machine machine;
typedef struct machine_stats machine_stats;
typedef value* (*builtin)(machine* m, value* args);

typedef enum {
    TRACE_OFF = 0,
    TRACE_SUMMARY = 1,
    TRACE_ALL = 10
} machine_trace_level;

struct machine_stats {
    size_t num_inst;
    size_t num_inst_stack;
    size_t num_inst_op_call;
    size_t stack_depth;
    size_t stack_depth_max;
    size_t garbage_before;
    size_t garbage_after;

    int flag;
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
    value* pc;
    value* val;

    machine_stats stats;

    volatile int stop;
    volatile int trace;
};

machine* machine_new(value* code, char* output_register_name);
void machine_dispose(machine* m);

void machine_bind_op(machine* m, char* name, builtin fn);
void machine_copy_to_register(machine* m, char* name, value* v);
value* machine_copy_from_register(machine* m, char* name);
value* machine_get_register(machine* m, char* name);
value* machine_get_label(machine* m, char* name);
value* machine_export_output(machine* m);
void machine_run(machine* m);

void machine_set_trace(machine* m, machine_trace_level level);
void machine_interrupt(machine* m);

#endif  // MACHINE_H_
