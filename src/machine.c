#include "machine.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

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

static const size_t MAX_STACK_VALUES = 100000;
static const size_t MAX_GARBAGE_VALUES = 1000000;

static value* get_or_create_record(machine* m, value* table, char* name) {
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

    key = pool_new_symbol(m->pool, name);                     // new key
    record = pool_new_pair(m->pool, NULL, key);               // NULL value
    table->cdr = pool_new_pair(m->pool, record, table->cdr);  // add to the table

    return record;
}

static value* get_register(machine* m, char* name) {
    return get_or_create_record(m, m->registers, name);
}

static value* get_label(machine* m, char* name) {
    return get_or_create_record(m, m->labels, name);
}

static value* get_op(machine* m, char* name) {
    return get_or_create_record(m, m->ops, name);
}

static value* make_constant(machine* m, value* source) {
    value* constant = pool_import(m->pool, source);  // clone locally
    m->constants->cdr = pool_new_pair(m->pool, constant, m->constants->cdr);

    return m->constants->cdr;
}

static void create_backbone(machine* m, char* output_register_name) {
    // chain of containers to keep the machine state:
    // cars are used as links, cdrs are for the content
    m->registers = pool_new_pair(m->pool, NULL, NULL);
    m->constants = pool_new_pair(m->pool, m->registers, NULL);
    m->labels = pool_new_pair(m->pool, m->constants, NULL);
    m->code = pool_new_pair(m->pool, m->labels, NULL);
    m->ops = pool_new_pair(m->pool, m->code, NULL);
    m->stack = pool_new_pair(m->pool, m->ops, NULL);
    m->pc = pool_new_pair(m->pool, m->stack, NULL);
    m->trace = pool_new_pair(m->pool, m->pc, NULL);
    m->root = pool_new_pair(m->pool, m->trace, NULL);  // memory root

    m->val = get_register(m, output_register_name);  // output register
    m->trace->cdr = pool_new_number(m->pool, 0);     // trace flag
}

static void push_to_stack(machine* m, value* v) {
    // add a new record to the stack with the value as car
    m->stack->cdr = pool_new_pair(m->pool, v, m->stack->cdr);

    m->stats.num_inst_stack += 1;
    m->stats.stack_depth += 1;
    if (m->stats.stack_depth > m->stats.stack_depth_max) {
        m->stats.stack_depth_max = m->stats.stack_depth;
    }
}

static value* pop_from_stack(machine* m) {
    assert(m->stack->cdr != NULL);

    value* v = m->stack->cdr->car;       // pop the value
    m->stack->cdr = m->stack->cdr->cdr;  // evict the record

    m->stats.num_inst_stack += 1;
    m->stats.stack_depth -= 1;

    return v;
}

static void clear_stack(machine* m) {
    m->stack->cdr = NULL;
    m->stats.stack_depth = 0;
}

static int in_trace_mode(machine* m) {
    return m->trace->cdr->number == 1;
}

static value* call_op(machine* m, value* op, value* args) {
    // call the builtin op
    value* result = NULL;
    if (op->car == NULL) {
        // the op record is not bound to a builtin function
        result = pool_new_error(m->pool, "machine op %s is unbound", op->cdr->symbol);
    } else {
        // call the builtin
        result = ((builtin)op->car->ptr)(m, args);
    }

    m->stats.num_inst_op_call += 1;

    return result;
}

static void reset_stats(machine* m) {
    m->stats.num_inst = 0;
    m->stats.num_inst_stack = 0;
    m->stats.num_inst_op_call = 0;
    m->stats.stack_depth = 0;
    m->stats.stack_depth_max = 0;
    m->stats.garbage_before = 0;
    m->stats.garbage_after = 0;
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

static void process_code(machine* m, value* source) {
    value* code = m->code;
    value* label = NULL;

    while (source != NULL) {
        value* line = source->car;
        if (line->type == VALUE_SYMBOL) {
            // create the label and add a pointer
            // to the code in the next iteration
            label = get_label(m, line->symbol);
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
            code->cdr = pool_new_code(
                m->pool,
                pool_new_pair(
                    m->pool,
                    instruction,
                    local_line),
                NULL);
            code = code->cdr;

            if (label != NULL) {
                // point the label above to
                // the current instruction
                label->car = code;
                label = NULL;
            }
        }

        source = source->cdr;
    }
}

static void execute_assign(machine* m, value* inst) {
    value* dst_reg = inst->car;  // dst register
    value* src = inst->cdr;      // src: register, label, or const

    // assign to the dst register from
    // src register, label, or const
    dst_reg->car = src->car;
    // advance the pc
    m->pc = m->pc->cdr;
}

static void execute_call(machine* m, value* inst) {
    value* dst = inst->car;        // dst register (if any)
    value* op = inst->cdr->car;    // op record (pointer . name)
    value* args = inst->cdr->cdr;  // op's args

    value* result = call_op(m, op, args);

    if (result != NULL && result->type == VALUE_ERROR) {
        // set the output register to the error
        // and halt immediately
        m->val->car = result;
        m->pc = NULL;
    } else {
        if (dst != NULL) {
            // set the register from the result
            dst->car = result;
        }
        // advance the pc
        m->pc = m->pc->cdr;
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
        m->stats.flag = -1;
        m->val->car = result;
        m->pc = NULL;
    } else if (value_is_true(result)) {
        // jump to the label
        m->stats.flag = 1;
        m->pc = label->car;
    } else {
        // advance the pc
        m->stats.flag = 0;
        m->pc = m->pc->cdr;
    }
}

static void execute_goto(machine* m, value* inst) {
    value* target = inst;

    // jump to the target register or label
    m->pc = target->car;
}

static void execute_save(machine* m, value* inst) {
    value* src = inst;

    if (m->stats.stack_depth >= MAX_STACK_VALUES) {
        // clear the stack
        clear_stack(m);
        // return and error and halt the program
        m->val->car = pool_new_error(m->pool, "stack limit exceeded");
        m->pc = NULL;
    } else {
        // push the src register to the stack
        push_to_stack(m, src->car);
        // advance the pc
        m->pc = m->pc->cdr;
    }
}

static void execute_restore(machine* m, value* inst) {
    value* dst = inst;

    // pop the src register from the stack
    dst->car = pop_from_stack(m);
    // advance the pc
    m->pc = m->pc->cdr;
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

static void trace_before(machine* m, value* line, value* instruction) {
    static char message[16348];
    // print the instrumented line
    value_to_str(line, message);
    printf("\x1B[34m%05zu\x1B[0m %s", m->stats.num_inst, message);
}

static void trace_after(machine* m, value* line, value* instruction) {
    static char message[16348];
    switch ((int)instruction->car->number) {
        case INST_ASSIGN:
            // print the destination register's content
            value_to_str(instruction->cdr->car->car, message);
        case INST_CALL:
            if (instruction->cdr->car != NULL) {
                // print the destination register's content
                value_to_str(instruction->cdr->car->car, message);
            } else {
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
            message[0] = '\0';
    }

    if (message[0]) {
        printf(" \x1B[34m==>\x1B[0m %s\n", message);
    } else {
        printf("\n");
    }
}

static void report_stats(machine* m) {
    machine_stats s = m->stats;

    printf("\n");
    printf("instructions:\n");
    printf("  - total: %zu\n", s.num_inst);
    printf("  - op calls: %zu\n", s.num_inst_op_call);
    printf("  - stack: %zu\n", s.num_inst_stack);
    printf("stack:\n");
    printf("  - final depth: %zu\n", s.stack_depth);
    printf("  - maximum depth: %zu\n", s.stack_depth_max);
    printf("garbage:\n");
    printf("  - # of values: %zu\n", m->pool->size);
    printf("\n");
}

static void execute_next_instruction(machine* m) {
    value* instruction = m->pc->car->car;  // current instruction
    value* line = m->pc->car->cdr;         // current line of code

    m->stats.num_inst += 1;

    if (in_trace_mode(m)) {
        trace_before(m, line, instruction);
    }

    instruction_type type = (int)instruction->car->number;
    execution_fns[type](m, instruction->cdr);

    if (in_trace_mode(m)) {
        trace_after(m, line, instruction);
    }

    if (m->pool->size >= MAX_GARBAGE_VALUES) {
        // interim garbage collection:
        // can be inefficient if much
        // of the garbage is needed
        pool_collect_garbage(m->pool);
    }
}

void machine_new(machine** m, value* code, char* output_register_name) {
    *m = malloc(sizeof(machine));

    pool_new(&((*m)->pool));
    create_backbone(*m, output_register_name);
    pool_register_root((*m)->pool, (*m)->root);
    process_code(*m, code);
}

void machine_dispose(machine** m) {
    pool_unregister_root((*m)->pool, (*m)->root);
    pool_dispose(&((*m)->pool));

    free(*m);
    *m = NULL;
}

void machine_set_trace(machine* m, int on) {
    m->trace->cdr->number = (on ? 1 : 0);
}

void machine_bind_op(machine* m, char* name, builtin fn) {
    value* op = get_op(m, name);
    op->car = pool_new_builtin(m->pool, fn, name);
}

void machine_copy_to_register(machine* m, char* name, value* v) {
    value* dst_reg = get_register(m, name);
    dst_reg->car = pool_import(m->pool, v);
}

value* machine_copy_from_register(machine* m, char* name) {
    value* src_reg = get_register(m, name);

    return pool_export(m->pool, src_reg->car);
}

value* machine_get_register(machine* m, char* name) {
    return get_register(m, name);
}

value* machine_export_output(machine* m) {
    return pool_export(m->pool, m->val->car);
}

void machine_run(machine* m) {
    reset_stats(m);
    clear_stack(m);

    m->pc = m->code->cdr;
    while (m->pc != NULL) {
        execute_next_instruction(m);
    }

    if (in_trace_mode(m)) {
        report_stats(m);
    }
}
