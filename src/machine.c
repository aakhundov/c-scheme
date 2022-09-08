#include "machine.h"

#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "const.h"
#include "env.h"
#include "pool.h"
#include "value.h"

typedef enum {
    INST_ASSIGN = 0,
    INST_CALL = 1,
    INST_BRANCH = 2,
    INST_GOTO = 3,
    INST_SAVE = 4,
    INST_RESTORE = 5,
} instruction_type;

static double get_time() {
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    return t.tv_sec + t.tv_nsec / 1000000000.0;
}

static value* get_or_create_record(machine* m, value* table, const char* name) {
    value* pair = table->cdr;
    value* record = NULL;
    value* key = NULL;

    while (pair != NULL) {
        record = pair->car;
        key = record->cdr;
        if (strcmp(key->symbol, name) == 0) {
            // found: the key matches the name
            return record;
        }
        pair = pair->cdr;
    }

    // find the previous pair in the
    // lexicographic order wrt. the name
    value* prev = table;
    while (prev->cdr != NULL && strcmp(prev->cdr->car->cdr->symbol, name) < 0) {
        // the record's key is "less" than the name
        prev = prev->cdr;
    }

    key = pool_new_symbol(m->pool, name);                   // new key
    record = pool_new_pair(m->pool, NULL, key);             // NULL value
    prev->cdr = pool_new_pair(m->pool, record, prev->cdr);  // add to the table

    return record;
}

static value* get_register(machine* m, const char* name) {
    return get_or_create_record(m, m->registers, name);
}

static value* get_label(machine* m, const char* name) {
    return get_or_create_record(m, m->labels, name);
}

static value* get_op(machine* m, const char* name) {
    return get_or_create_record(m, m->ops, name);
}

static value* make_constant(machine* m, value* source) {
    value* constant = pool_import(m->pool, source);  // clone locally
    m->constants->cdr = pool_new_pair(m->pool, constant, m->constants->cdr);

    return m->constants->cdr;
}

static void create_backbone(machine* m, const char* output_register_name) {
    // chain of containers to keep the machine state:
    // cars are used as links, cdrs are for the content
    m->registers = pool_new_pair(m->pool, NULL, NULL);
    m->constants = pool_new_pair(m->pool, m->registers, NULL);
    m->labels = pool_new_pair(m->pool, m->constants, NULL);
    m->ops = pool_new_pair(m->pool, m->labels, NULL);
    m->code_head = pool_new_pair(m->pool, m->ops, NULL);
    m->code_tail = pool_new_pair(m->pool, m->code_head, NULL);
    m->stack = pool_new_pair(m->pool, m->code_tail, NULL);
    m->pc = pool_new_pair(m->pool, m->stack, NULL);
    m->root = pool_new_pair(m->pool, m->pc, NULL);  // memory root

    m->val = get_register(m, output_register_name);  // output register
    m->code_tail = m->code_head;                     // initially no code
}

static void push_to_stack(machine* m, value* v) {
    // add a new record to the stack with the value as car
    m->stack->cdr = pool_new_pair(m->pool, v, m->stack->cdr);
}

static value* pop_from_stack(machine* m) {
    assert(m->stack->cdr != NULL);

    value* v = m->stack->cdr->car;       // pop the value
    m->stack->cdr = m->stack->cdr->cdr;  // evict the record

    return v;
}

static void clear_stack(machine* m) {
    m->stack->cdr = NULL;
}

static value* call_op(machine* m, value* op, value* args) {
    value* result = NULL;
    if (op->car == NULL) {
        // the op record is not bound to a machine op
        result = pool_new_error(m->pool, "machine op %s is unbound", op->cdr->symbol);
    } else {
        // call the machine op
        result = ((machine_op)op->car->ptr)(m, args);
    }

    return result;
}

static value* update_count_env(machine* m, value* env, value* table) {
    value* pair = table->cdr;
    while (pair != NULL) {
        value* record = pair->car;
        char* name = record->cdr->symbol;

        if (env_lookup(env, name, 0) == NULL) {
            // add the name's count (0) to the env if absent
            value* count = pool_new_number(m->pool, 0);
            env_add_value(env, name, count, m->pool);
        }

        pair = pair->cdr;
    }

    return env;
}

static void reset_count_env(value* env, value* table) {
    value* pair = table->cdr;
    while (pair != NULL) {
        value* record = pair->car;
        value* name = record->cdr;

        // reset each name's count to zero
        // (assuming every name is present in the env)
        map_record* r = env_lookup(env, name->symbol, 0);
        value* count = env_get_value(r);
        count->number = 0;

        pair = pair->cdr;
    }
}

static void increment_count(value* env, char* name) {
    // increment the name's current count
    map_record* r = env_lookup(env, name, 0);
    if (r != NULL) {  // ignore if the name isn't there
        value* count = env_get_value(r);
        count->number += 1;
    }
}

static void print_counts(value* env, value* table, const char* format) {
    value* pair = table->cdr;
    while (pair != NULL) {
        value* record = pair->car;
        value* name = record->cdr;

        // retrieve the name's current count
        map_record* r = env_lookup(env, name->symbol, 0);
        value* count = env_get_value(r);

        if (count->number > 0) {
            // print only if the count is positive
            printf(format, name->symbol, (long)count->number);
        }

        pair = pair->cdr;
    }
}

static void init_stats(machine* m) {
    m->stats.cnt_op_calls = pool_new_env(m->pool);
    m->stats.cnt_register_assigns = pool_new_env(m->pool);
    m->stats.cnt_register_saves = pool_new_env(m->pool);
    m->stats.cnt_label_jumps = pool_new_env(m->pool);

    pool_register_root(m->pool, m->stats.cnt_op_calls);
    pool_register_root(m->pool, m->stats.cnt_register_assigns);
    pool_register_root(m->pool, m->stats.cnt_register_saves);
    pool_register_root(m->pool, m->stats.cnt_label_jumps);
}

static void update_stats(machine* m) {
    update_count_env(m, m->stats.cnt_op_calls, m->ops);
    update_count_env(m, m->stats.cnt_register_assigns, m->registers);
    update_count_env(m, m->stats.cnt_register_saves, m->registers);
    update_count_env(m, m->stats.cnt_label_jumps, m->labels);
}

static void cleanup_stats(machine* m) {
    pool_unregister_root(m->pool, m->stats.cnt_op_calls);
    pool_unregister_root(m->pool, m->stats.cnt_register_assigns);
    pool_unregister_root(m->pool, m->stats.cnt_register_saves);
    pool_unregister_root(m->pool, m->stats.cnt_label_jumps);
}

static void reset_stats(machine* m) {
    machine_stats* s = &m->stats;

    s->start_time = 0;
    s->end_time = 0;

    s->num_inst = 0;
    s->num_inst_assign = 0;
    s->num_inst_call = 0;
    s->num_inst_goto = 0;
    s->num_inst_branch = 0;
    s->num_inst_save = 0;
    s->num_inst_restore = 0;

    s->stack_depth = 0;
    s->stack_depth_max = 0;

    s->garbage_before = 0;
    s->garbage_after = 0;
    s->garbage_collected_times = 0;
    s->garbage_collected_values = 0;
    s->garbage_collection_time = 0;

    s->flag = 0;

    reset_count_env(m->stats.cnt_op_calls, m->ops);
    reset_count_env(m->stats.cnt_register_assigns, m->registers);
    reset_count_env(m->stats.cnt_register_saves, m->registers);
    reset_count_env(m->stats.cnt_label_jumps, m->labels);
}

static value* make_args(machine* m, value* arg_list) {
    value* args = NULL;
    value* tail = NULL;

    while (arg_list != NULL) {
        // the args are represented
        // as a list of (type value) pairs
        value* arg_pair = arg_list->car;
        assert(arg_pair->type == VALUE_PAIR);
        assert(arg_pair->car != NULL);
        assert(arg_pair->cdr != NULL);
        value* arg_type = arg_pair->car;
        assert(arg_type->type == VALUE_SYMBOL);
        value* arg_value = arg_pair->cdr->car;

        value* src = NULL;
        if (strcmp(arg_type->symbol, "reg") == 0) {
            // the arg comes from the (reg name)
            assert(arg_value->type == VALUE_SYMBOL);
            src = get_register(m, arg_value->symbol);
        } else if (strcmp(arg_type->symbol, "label") == 0) {
            // the arg comes from the (label value)
            assert(arg_value->type == VALUE_SYMBOL);
            src = get_label(m, arg_value->symbol);
        } else if (strcmp(arg_type->symbol, "const") == 0) {
            // the arg comes from the (const value)
            src = make_constant(m, arg_value);
        }
        assert(src != NULL);

        if (args == NULL) {
            // first arg in the list
            args = pool_new_pair(m->pool, src, NULL);
            tail = args;
        } else {
            // next arg appended to the end of the list
            tail->cdr = pool_new_pair(m->pool, src, NULL);
            tail = tail->cdr;
        }

        arg_list = arg_list->cdr;
    }

    return args;
}

static value* process_assign(machine* m, value* source) {
    // destination register
    value* dst_reg = source->car;
    assert(dst_reg->type == VALUE_SYMBOL);
    // source pair: (op name), (reg name),
    // (label name), or (const value)
    value* src_pair = source->cdr->car;
    assert(src_pair->type == VALUE_PAIR);
    assert(src_pair->car != NULL);
    assert(src_pair->cdr != NULL);
    value* src_type = src_pair->car;
    assert(src_type->type == VALUE_SYMBOL);
    value* src_value = src_pair->cdr->car;

    if (strcmp(src_type->symbol, "op") == 0) {
        assert(src_value->type == VALUE_SYMBOL);

        // (arg1) (arg2) ...
        value* op_args = source->cdr->cdr;

        // call the op and assign
        // the result to the register
        return pool_new_pair(
            m->pool,
            pool_new_number(m->pool, INST_CALL),  // instruction
            pool_new_pair(
                m->pool,
                get_register(m, dst_reg->symbol),  // dst register
                pool_new_pair(
                    m->pool,
                    get_op(m, src_value->symbol),  // src op
                    make_args(m, op_args))));      // src op's args
    } else {
        value* src = NULL;
        if (strcmp(src_type->symbol, "reg") == 0) {
            // the src is the register
            assert(src_value->type == VALUE_SYMBOL);
            src = get_register(m, src_value->symbol);
        } else if (strcmp(src_type->symbol, "label") == 0) {
            // the src is the label
            assert(src_value->type == VALUE_SYMBOL);
            src = get_label(m, src_value->symbol);
        } else if (strcmp(src_type->symbol, "const") == 0) {
            // the src is the const
            src = make_constant(m, src_value);
        }
        assert(src != NULL);

        // assign the src
        return pool_new_pair(
            m->pool,
            pool_new_number(m->pool, INST_ASSIGN),  // instruction
            pool_new_pair(
                m->pool,
                get_register(m, dst_reg->symbol),  // dst register
                src));                             // src register, label, or const
    }
}

static value* process_perform(machine* m, value* source) {
    // (op name) pair
    value* op_pair = source->car;
    assert(op_pair->type == VALUE_PAIR);
    assert(op_pair->car != NULL);
    assert(op_pair->cdr != NULL);
    value* op_symbol = op_pair->car;
    assert(op_symbol->type == VALUE_SYMBOL);
    assert(strcmp(op_symbol->symbol, "op") == 0);
    value* op_name = op_pair->cdr->car;
    assert(op_name->type == VALUE_SYMBOL);

    // (arg1) (arg2) ...
    value* op_args = source->cdr;

    // call the op without assigning
    // the result to a register
    return pool_new_pair(
        m->pool,
        pool_new_number(m->pool, INST_CALL),  // instruction
        pool_new_pair(
            m->pool,
            NULL,  // no dst register
            pool_new_pair(
                m->pool,
                get_op(m, op_name->symbol),  // op
                make_args(m, op_args))));    // op's args
}

static value* process_branch(machine* m, value* source) {
    // (label name) pair
    value* label_pair = source->car;
    assert(label_pair->type == VALUE_PAIR);
    assert(label_pair->car != NULL);
    assert(label_pair->cdr != NULL);
    value* label_symbol = label_pair->car;
    assert(label_symbol->type == VALUE_SYMBOL);
    assert(strcmp(label_symbol->symbol, "label") == 0);
    value* label_name = label_pair->cdr->car;
    assert(label_name->type == VALUE_SYMBOL);

    // (op name) pair
    assert(source->cdr != NULL);
    assert(source->cdr->car != NULL);
    value* op_pair = source->cdr->car;
    assert(op_pair->type == VALUE_PAIR);
    assert(op_pair->car != NULL);
    assert(op_pair->cdr != NULL);
    value* op_symbol = op_pair->car;
    assert(op_symbol->type == VALUE_SYMBOL);
    assert(strcmp(op_symbol->symbol, "op") == 0);
    value* op_name = op_pair->cdr->car;
    assert(op_name->type == VALUE_SYMBOL);

    // (arg1) (arg2) ...
    value* op_args = source->cdr->cdr;

    return pool_new_pair(
        m->pool,
        pool_new_number(m->pool, INST_BRANCH),  // instruction
        pool_new_pair(
            m->pool,
            get_label(m, label_name->symbol),  // label
            pool_new_pair(
                m->pool,
                get_op(m, op_name->symbol),  // op
                make_args(m, op_args))));    // op's args
}

static value* process_goto(machine* m, value* source) {
    // (reg name) or (label name) pair
    value* taret_pair = source->car;
    assert(taret_pair->type == VALUE_PAIR);
    assert(taret_pair->car != NULL);
    assert(taret_pair->cdr != NULL);
    value* target_type = taret_pair->car;
    assert(target_type->type == VALUE_SYMBOL);
    value* target_name = taret_pair->cdr->car;
    assert(target_name->type == VALUE_SYMBOL);

    value* target = NULL;
    if (strcmp(target_type->symbol, "reg") == 0) {
        // the target is the register
        target = get_register(m, target_name->symbol);
    } else if (strcmp(target_type->symbol, "label") == 0) {
        // the target is the label
        target = get_label(m, target_name->symbol);
    }
    assert(target != NULL);

    // jump to the target
    return pool_new_pair(
        m->pool,
        pool_new_number(m->pool, INST_GOTO),  // instruction
        target);                              // target: register or label
}

static value* process_save(machine* m, value* source) {
    // register to save from
    value* src_reg = source->car;
    assert(src_reg->type == VALUE_SYMBOL);

    // save the register
    // to the stack
    return pool_new_pair(
        m->pool,
        pool_new_number(m->pool, INST_SAVE),  // instruction
        get_register(m, src_reg->symbol));    // src register
}

static value* process_restore(machine* m, value* source) {
    // register to restore into
    value* dst_reg = source->car;
    assert(dst_reg->type == VALUE_SYMBOL);

    // restore the register
    // from the stack
    return pool_new_pair(
        m->pool,
        pool_new_number(m->pool, INST_RESTORE),  // instruction
        get_register(m, dst_reg->symbol));       // dst register
}

static void instrument_code(machine* m, value* line) {
    value* running = line;
    while (running != NULL) {
        // locate all (reg name) pairs
        if (running->car->type == VALUE_PAIR) {
            assert(running->car->car->type == VALUE_SYMBOL);
            char* type = running->car->car->symbol;
            if (strcmp(type, "reg") == 0) {
                // replace "name" in (reg name) with the real register
                assert(running->car->cdr->car->type == VALUE_SYMBOL);
                char* name = running->car->cdr->car->symbol;
                running->car->cdr = get_register(m, name);
            }
        }
        running = running->cdr;
    }
}

static value* append_code(machine* m, const value* source) {
    value* head = m->code_tail;
    value* tail = m->code_tail;
    value* pending_labels = NULL;

    while (source != NULL) {
        value* line = source->car;
        if (line->type == VALUE_SYMBOL) {
            // creat a new label and add it
            // to the chain of pending labels
            value* label = get_label(m, line->symbol);
            pending_labels = pool_new_pair(m->pool, label, pending_labels);
        } else {
            // line is a list starting with a statement
            assert(line->type == VALUE_PAIR);
            assert(line->car != NULL);
            assert(line->cdr != NULL);
            assert(line->car->type == VALUE_SYMBOL);

            value* instruction = NULL;
            char* statement = line->car->symbol;
            if (strcmp(statement, "assign") == 0) {
                instruction = process_assign(m, line->cdr);
            } else if (strcmp(statement, "perform") == 0) {
                instruction = process_perform(m, line->cdr);
            } else if (strcmp(statement, "branch") == 0) {
                instruction = process_branch(m, line->cdr);
            } else if (strcmp(statement, "goto") == 0) {
                instruction = process_goto(m, line->cdr);
            } else if (strcmp(statement, "save") == 0) {
                instruction = process_save(m, line->cdr);
            } else if (strcmp(statement, "restore") == 0) {
                instruction = process_restore(m, line->cdr);
            }
            assert(instruction != NULL);

            // save and instrument the line locally
            value* local_line = pool_import(m->pool, line);
            instrument_code(m, local_line);

            // append the new instruction along with
            // the local line to the end of the code
            tail->cdr = pool_new_code(
                m->pool,
                pool_new_pair(
                    m->pool,
                    instruction,
                    local_line),
                NULL);
            tail = tail->cdr;

            // while any labels are pending
            while (pending_labels != NULL) {
                // point the first pending label in the
                // chain to the current instruction
                value* label = pending_labels->car;
                label->car = tail;

                // move to the next pending label
                pending_labels = pending_labels->cdr;
            }
        }

        source = source->cdr;
    }

    // add new items introduced in the code
    // (registers, labels, ops) to the stats
    update_stats(m);

    // set the new tail
    m->code_tail = tail;

    // return the head of the appended code
    return head->cdr;
}

static void execute_assign(machine* m, value* inst) {
    value* dst_reg = inst->car;  // dst register
    value* src = inst->cdr;      // src: register, label, or const

    // assign to the dst register from
    // src register, label, or const
    dst_reg->car = src->car;
    // advance the pc
    m->pc = m->pc->cdr;

    if (m->trace >= TRACE_SUMMARY) {
        m->stats.num_inst_assign += 1;

        if (m->trace >= TRACE_COUNTS) {
            increment_count(m->stats.cnt_register_assigns, dst_reg->cdr->symbol);
        }
    }
}

static void execute_call(machine* m, value* inst) {
    value* dst_reg = inst->car;    // dst register (if any)
    value* op = inst->cdr->car;    // op record (pointer . name)
    value* args = inst->cdr->cdr;  // op's args

    // advance the pc before the call
    // to allow the op to modify the pc
    m->pc = m->pc->cdr;

    // call the op
    value* result = call_op(m, op, args);

    if (result != NULL && result->type == VALUE_ERROR) {
        // set the output register to the error
        // and halt immediately
        m->val->car = result;
        m->pc = NULL;
    } else {
        if (dst_reg != NULL) {
            // set the register from the result
            dst_reg->car = result;
        }
    }

    if (m->trace >= TRACE_SUMMARY) {
        m->stats.num_inst_call += 1;

        if (m->trace >= TRACE_COUNTS) {
            increment_count(m->stats.cnt_op_calls, op->cdr->symbol);

            if (dst_reg != NULL) {
                increment_count(m->stats.cnt_register_assigns, dst_reg->cdr->symbol);
            }
        }
    }
}

static void execute_branch(machine* m, value* inst) {
    value* label = inst->car;
    value* op = inst->cdr->car;    // op record (pointer . name)
    value* args = inst->cdr->cdr;  // op's args

    value* result = call_op(m, op, args);

    if (result != NULL && result->type == VALUE_ERROR) {
        // set the output register to the error
        // and halt immediately
        m->val->car = result;
        m->pc = NULL;
    } else if (value_is_true(result)) {
        // jump to the label
        m->pc = label->car;
    } else {
        // advance the pc
        m->pc = m->pc->cdr;
    }

    if (m->trace >= TRACE_SUMMARY) {
        m->stats.num_inst_branch += 1;

        if (m->trace >= TRACE_COUNTS) {
            increment_count(m->stats.cnt_op_calls, op->cdr->symbol);

            if (m->pc == NULL) {
                m->stats.flag = -1;
            } else if (value_is_true(result)) {
                m->stats.flag = 1;
                increment_count(m->stats.cnt_label_jumps, label->cdr->symbol);
            } else {
                m->stats.flag = 0;
            }
        }
    }
}

static void execute_goto(machine* m, value* inst) {
    value* target = inst;

    // jump to the target register or label
    m->pc = target->car;

    if (m->trace >= TRACE_SUMMARY) {
        m->stats.num_inst_goto += 1;

        if (m->trace >= TRACE_COUNTS) {
            // the count will be incremented if the target is a label
            increment_count(m->stats.cnt_label_jumps, target->cdr->symbol);
        }
    }
}

static void execute_save(machine* m, value* inst) {
    value* src_reg = inst;

    if (m->stats.stack_depth >= MAX_STACK_VALUES) {
        // return and error and halt the program
        m->val->car = pool_new_error(m->pool, "stack limit exceeded");
        m->pc = NULL;
    } else {
        // push the src register to the stack
        push_to_stack(m, src_reg->car);
        m->stats.stack_depth += 1;
        // advance the pc
        m->pc = m->pc->cdr;
    }

    if (m->trace >= TRACE_SUMMARY) {
        m->stats.num_inst_save += 1;
        if (m->stats.stack_depth > m->stats.stack_depth_max) {
            m->stats.stack_depth_max = m->stats.stack_depth;
        }

        if (m->trace >= TRACE_COUNTS) {
            increment_count(m->stats.cnt_register_saves, src_reg->cdr->symbol);
        }
    }
}

static void execute_restore(machine* m, value* inst) {
    value* dst_reg = inst;

    // pop the src register from the stack
    dst_reg->car = pop_from_stack(m);
    m->stats.stack_depth -= 1;
    // advance the pc
    m->pc = m->pc->cdr;

    if (m->trace >= TRACE_SUMMARY) {
        m->stats.num_inst_restore += 1;
    }
}

// array of execution functions for quick dispatch
static void (*execution_fns[])(machine*, value*) = {
    execute_assign,
    execute_call,
    execute_branch,
    execute_goto,
    execute_save,
    execute_restore,
};

static void trace_before_inst(machine* m, value* line, value* instruction) {
    static char message[BUFFER_SIZE];

    // print the instrumented line
    value_to_str(line, message);
    printf("\x1B[34m%05zu\x1B[0m %s", m->stats.num_inst, message);
}

static void trace_after_inst(machine* m, value* line, value* instruction) {
    static char message[BUFFER_SIZE];

    switch ((int)instruction->car->number) {
        case INST_ASSIGN:
            // print the destination register's content
            value_to_str(instruction->cdr->car->car, message);
        case INST_CALL:
            if (instruction->cdr->car != NULL) {
                // print the destination register's content
                value_to_str(instruction->cdr->car->car, message);
            } else {
                // print nothing
                message[0] = '\0';
            }
            break;
        case INST_BRANCH:
            // print the result of the previous test
            if (m->stats.flag == -1) {
                sprintf(message, "%s", "error");
            } else {
                sprintf(message, "%s", (m->stats.flag ? "true" : "false"));
            }
            break;
        case INST_SAVE:
        case INST_RESTORE:
            // print the associated register's content
            value_to_str(instruction->cdr->car, message);
            break;
        default:
            // print nothing
            message[0] = '\0';
    }

    if (message[0]) {
        printf(" \x1B[34m==>\x1B[0m %s\n", message);
    } else {
        printf("\n");
    }
}

static void trace_report(machine* m) {
    if (m->trace >= TRACE_GENERAL) {
        machine_stats* s = &m->stats;

        static const char* line = "\x1B[34m+---------------------------+-----------------+\x1B[0m\n";
        static const char* header = "\x1B[34m| %-43s |\x1B[0m\n";
        static const char* row = "\x1B[34m|\x1B[0m %-25.25s \x1B[34m|\x1B[0m %'15ld \x1B[34m|\x1B[0m\n";

        setlocale(LC_ALL, "en_US.UTF-8");

        long execution_time = (s->end_time - s->start_time) * 1000;
        long execution_memory = s->garbage_after - s->garbage_before + s->garbage_collected_values;

        printf("%s", line);
        printf(header, "GENERAL");
        printf("%s", line);
        printf(row, "time, ms", execution_time);
        printf(row, "memory, values", execution_memory);
        printf(row, "instructions", s->num_inst);
        printf("%s", line);

        if (m->trace >= TRACE_SUMMARY) {
            printf("%s", line);

            printf(header, "INSTRUCTIONS");
            printf("%s", line);
            printf(row, "assign", s->num_inst_assign);
            printf(row, "call", s->num_inst_call);
            printf(row, "branch", s->num_inst_branch);
            printf(row, "goto", s->num_inst_goto);
            printf(row, "save", s->num_inst_save);
            printf(row, "restore", s->num_inst_restore);
            printf("%s", line);

            printf(header, "STACK");
            printf("%s", line);
            printf(row, "final depth", s->stack_depth);
            printf(row, "maximum depth", s->stack_depth_max);
            printf("%s", line);

            printf(header, "GARBAGE");
            printf("%s", line);
            printf(row, "times collected", s->garbage_collected_times);
            printf(row, "collected values", s->garbage_collected_values);
            printf(row, "collection time, ms", (long)(s->garbage_collection_time * 1000));
            printf(row, "before", s->garbage_before);
            printf(row, "after", s->garbage_after);
            printf("%s", line);
        }

        if (m->trace >= TRACE_COUNTS) {
            printf("%s", line);

            printf(header, "OP CALLS");
            printf("%s", line);
            print_counts(m->stats.cnt_op_calls, m->ops, row);
            printf("%s", line);

            printf(header, "REGISTER ASSIGNS");
            printf("%s", line);
            print_counts(m->stats.cnt_register_assigns, m->registers, row);
            printf("%s", line);

            printf(header, "REGISTER SAVES");
            printf("%s", line);
            print_counts(m->stats.cnt_register_saves, m->registers, row);
            printf("%s", line);

            printf(header, "LABEL JUMPS");
            printf("%s", line);
            print_counts(m->stats.cnt_label_jumps, m->labels, row);
            printf("%s", line);
        }
    }
}

static void execute_next_instruction(machine* m) {
    value* instruction = m->pc->car->car;  // current instruction
    value* line = NULL;

    if (m->stop) {
        m->val->car = pool_new_error(m->pool, "keyboard interrupt");
        m->pc = NULL;
        return;
    }

    if (m->trace >= TRACE_GENERAL) {
        m->stats.num_inst += 1;

        if (m->trace >= TRACE_INSTRUCTIONS) {
            line = m->pc->car->cdr;
            trace_before_inst(m, line, instruction);
        }
    }

    instruction_type type = (int)instruction->car->number;
    execution_fns[type](m, instruction->cdr);

    if (m->trace >= TRACE_INSTRUCTIONS) {
        trace_after_inst(m, line, instruction);
    }

    if (m->pool->size >= MAX_GARBAGE_VALUES) {
        size_t pool_size_before = 0;
        double start_time = 0;
        if (m->trace >= TRACE_GENERAL) {
            pool_size_before = m->pool->size;
            start_time = get_time();
        }

        // interim garbage collection:
        // can be inefficient if many
        // of the values are in use
        pool_collect_garbage(m->pool);

        if (m->trace >= TRACE_GENERAL) {
            m->stats.garbage_collected_times += 1;
            m->stats.garbage_collected_values += (pool_size_before - m->pool->size);
            m->stats.garbage_collection_time += (get_time() - start_time);
        }
    }
}

machine* machine_new(value* code, const char* output_register_name) {
    machine* m = malloc(sizeof(machine));

    m->pool = pool_new();
    create_backbone(m, output_register_name);
    pool_register_root(m->pool, m->root);

    init_stats(m);

    append_code(m, code);

    m->stop = 0;
    m->trace = 0;

    return m;
}

void machine_dispose(machine* m) {
    cleanup_stats(m);
    pool_unregister_root(m->pool, m->root);
    pool_dispose(m->pool);

    free(m);
}

void machine_bind_op(machine* m, const char* name, machine_op fn) {
    value* op = get_op(m, name);
    op->car = pool_new_primitive(m->pool, fn, name);

    // add newly bound op to the op calls count env
    // if it wasn't in the code  processed so far
    update_count_env(m, m->stats.cnt_op_calls, m->ops);
}

void machine_copy_to_register(machine* m, const char* name, value* v) {
    value* dst_reg = get_register(m, name);
    dst_reg->car = pool_import(m->pool, v);
}

value* machine_append_code(machine* m, const value* code) {
    return append_code(m, code);
}

void machine_set_code_position(machine* m, value* pos) {
    m->pc = pos;
}

value* machine_copy_from_register(machine* m, const char* name) {
    value* src_reg = get_register(m, name);

    return pool_export(m->pool, src_reg->car);
}

value* machine_get_register(machine* m, const char* name) {
    return get_register(m, name);
}

value* machine_get_label(machine* m, const char* name) {
    return get_label(m, name);
}

value* machine_export_output(machine* m) {
    return pool_export(m->pool, m->val->car);
}

void machine_run(machine* m) {
    clear_stack(m);
    reset_stats(m);

    if (m->trace >= TRACE_GENERAL) {
        m->stats.start_time = get_time();
        m->stats.garbage_before = m->pool->size;
    }

    m->stop = 0;
    m->pc = m->code_head->cdr;
    while (m->pc != NULL) {
        execute_next_instruction(m);
    }

    if (m->trace >= TRACE_GENERAL) {
        m->stats.end_time = get_time();
        m->stats.garbage_after = m->pool->size;

        trace_report(m);
    }
}

void machine_set_trace(machine* m, const machine_trace_level level) {
    m->trace = level;
}

void machine_interrupt(machine* m) {
    m->stop = 1;
}
