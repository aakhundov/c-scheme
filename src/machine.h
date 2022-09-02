#ifndef MACHINE_H_
#define MACHINE_H_

#include "pool.h"
#include "value.h"

typedef struct machine machine;
typedef struct machine_stats machine_stats;
typedef value* (*builtin)(machine* m, value* args);

typedef enum {
    TRACE_OFF = 0,
    TRACE_GENERAL = 1,
    TRACE_SUMMARY = 2,
    TRACE_COUNTS = 3,
    TRACE_INSTRUCTIONS = 4,
    TRACE_ALL = 10,
} machine_trace_level;

struct machine_stats {
    long start_time;
    long end_time;

    long num_inst;
    long num_inst_assign;
    long num_inst_call;
    long num_inst_goto;
    long num_inst_branch;
    long num_inst_save;
    long num_inst_restore;

    long stack_depth;
    long stack_depth_max;

    long garbage_before;
    long garbage_after;
    long garbage_collected_times;
    long garbage_collected_values;

    int flag;

    value* cnt_op_calls;
    value* cnt_register_assigns;
    value* cnt_register_saves;
    value* cnt_label_jumps;
};

struct machine {
    pool* pool;
    value* root;

    value* registers;
    value* constants;
    value* labels;
    value* ops;

    value* code_head;
    value* code_tail;

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

value* machine_append_code(machine* m, value* code);
void machine_set_code_position(machine* m, value* code);

void machine_set_trace(machine* m, machine_trace_level level);
void machine_interrupt(machine* m);

#endif  // MACHINE_H_
