#include "machine.h"

#include <assert.h>
#include <string.h>

#include "pool.h"
#include "value.h"

typedef enum {
    INST_ASSIGN = 0,
    INST_CALL = 1,
    INST_TEST = 2,
    INST_BRANCH = 3,
    INST_GOTO = 4,
    INST_SAVE = 5,
    INST_RESTORE = 6,
} instruction_type;

static value* get_or_create_record(machine* m, value* table, char* name) {
    value* pair = table->cdr;
    value* record = NULL;
    while (pair != NULL) {
        record = pair->car;
        if (strcmp(record->cdr->symbol, name) == 0) {
            return record;
        }
        pair = pair->cdr;
    }

    record = pool_new_pair(m->pool, NULL, pool_new_symbol(m->pool, name));
    table->cdr = pool_new_pair(m->pool, record, table->cdr);

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

static value* get_constant(machine* m, value* source) {
    value* constant = pool_import(m->pool, source);
    m->constants->cdr = pool_new_pair(m->pool, constant, m->constants->cdr);

    return m->constants->cdr;
}

static void create_backbone(machine* m, char* output_register_name) {
    m->registers = pool_new_pair(m->pool, NULL, NULL);
    m->constants = pool_new_pair(m->pool, m->registers, NULL);
    m->labels = pool_new_pair(m->pool, m->constants, NULL);
    m->code = pool_new_pair(m->pool, m->labels, NULL);
    m->ops = pool_new_pair(m->pool, m->code, NULL);
    m->stack = pool_new_pair(m->pool, m->ops, NULL);
    m->flag = pool_new_pair(m->pool, m->stack, NULL);
    m->pc = pool_new_pair(m->pool, m->flag, NULL);
    m->root = pool_new_pair(m->pool, m->pc, NULL);

    m->flag->cdr = pool_new_number(m->pool, 0);
    m->val = get_register(m, output_register_name);
}

static void push_to_stack(machine* m, value* v) {
    m->stack->cdr = pool_new_pair(m->pool, v, m->stack->cdr);
}

static value* pop_from_stack(machine* m) {
    assert(m->stack->cdr != NULL);

    value* v = m->stack->cdr->car;
    m->stack->cdr = m->stack->cdr->cdr;

    return v;
}

static void clear_stack(machine* m) {
    m->stack->cdr = NULL;
}

static void set_flag(machine* m, value* v) {
    m->flag->cdr->number = value_to_bool(v);
}

static int get_flag(machine* m) {
    return (int)m->flag->cdr->number;
}

static value* make_args(machine* m, value* arg_list) {
    value* result = NULL;
    value* tail = NULL;

    while (arg_list != NULL) {
        value* arg_pair = arg_list->car;
        assert(arg_pair->type == VALUE_PAIR);
        assert(arg_pair->car != NULL);
        assert(arg_pair->cdr != NULL);
        value* arg_type = arg_pair->car;
        assert(arg_type->type == VALUE_SYMBOL);
        value* arg_value = arg_pair->cdr->car;

        value* src = NULL;
        if (strcmp(arg_type->symbol, "reg") == 0) {
            assert(arg_value->type == VALUE_SYMBOL);
            src = get_register(m, arg_value->symbol);
        } else if (strcmp(arg_type->symbol, "const") == 0) {
            src = get_constant(m, arg_value);
        }
        assert(src != NULL);

        if (result == NULL) {
            result = pool_new_pair(m->pool, src, NULL);
            tail = result;
        } else {
            tail->cdr = pool_new_pair(m->pool, src, NULL);
            tail = tail->cdr;
        }

        arg_list = arg_list->cdr;
    }

    return result;
}

static value* process_assign(machine* m, value* source) {
    value* dst_reg = source->car;
    assert(dst_reg->type == VALUE_SYMBOL);
    value* src_pair = source->cdr->car;
    assert(src_pair->type == VALUE_PAIR);
    assert(src_pair->car != NULL);
    assert(src_pair->cdr != NULL);
    value* src_type = src_pair->car;
    assert(src_type->type == VALUE_SYMBOL);
    value* src_value = src_pair->cdr->car;

    if (strcmp(src_type->symbol, "op") == 0) {
        assert(src_value->type == VALUE_SYMBOL);

        // call the op and assign
        // the result to the register
        return pool_new_pair(
            m->pool,
            pool_new_number(m->pool, INST_CALL),
            pool_new_pair(
                m->pool,
                get_register(m, dst_reg->symbol),
                pool_new_pair(
                    m->pool,
                    get_op(m, src_value->symbol),
                    make_args(m, source->cdr->cdr))));
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
            src = get_constant(m, src_value);
        }
        assert(src != NULL);

        // assign the src
        return pool_new_pair(
            m->pool,
            pool_new_number(m->pool, INST_ASSIGN),
            pool_new_pair(
                m->pool,
                get_register(m, dst_reg->symbol),
                src));
    }
}

static value* process_perform(machine* m, value* source) {
    value* op_pair = source->car;
    assert(op_pair->type == VALUE_PAIR);
    assert(op_pair->car != NULL);
    assert(op_pair->cdr != NULL);
    value* op_symbol = op_pair->car;
    assert(op_symbol->type == VALUE_SYMBOL);
    assert(strcmp(op_symbol->symbol, "op") == 0);
    value* op_name = op_pair->cdr->car;
    assert(op_name->type == VALUE_SYMBOL);

    // call the op without assigning
    // the result to a register
    return pool_new_pair(
        m->pool,
        pool_new_number(m->pool, INST_CALL),
        pool_new_pair(
            m->pool,
            NULL,  // no dst register
            pool_new_pair(
                m->pool,
                get_op(m, op_name->symbol),
                make_args(m, source->cdr))));
}

static value* process_test(machine* m, value* source) {
    value* op_pair = source->car;
    assert(op_pair->type == VALUE_PAIR);
    assert(op_pair->car != NULL);
    assert(op_pair->cdr != NULL);
    value* op_symbol = op_pair->car;
    assert(op_symbol->type == VALUE_SYMBOL);
    assert(strcmp(op_symbol->symbol, "op") == 0);
    value* op_name = op_pair->cdr->car;
    assert(op_name->type == VALUE_SYMBOL);

    // call the op and assign
    // the result to the flag
    return pool_new_pair(
        m->pool,
        pool_new_number(m->pool, INST_TEST),
        pool_new_pair(
            m->pool,
            get_op(m, op_name->symbol),
            make_args(m, source->cdr)));
}

static value* process_branch(machine* m, value* source) {
    value* label_pair = source->car;
    assert(label_pair->type == VALUE_PAIR);
    assert(label_pair->car != NULL);
    assert(label_pair->cdr != NULL);
    value* label_symbol = label_pair->car;
    assert(label_symbol->type == VALUE_SYMBOL);
    assert(strcmp(label_symbol->symbol, "label") == 0);
    value* label_name = label_pair->cdr->car;
    assert(label_name->type == VALUE_SYMBOL);

    // jump to the label
    return pool_new_pair(
        m->pool,
        pool_new_number(m->pool, INST_BRANCH),
        get_label(m, label_name->symbol));
}

static value* process_goto(machine* m, value* source) {
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
        pool_new_number(m->pool, INST_GOTO),
        target);
}

static value* process_save(machine* m, value* source) {
    value* src_reg = source->car;
    assert(src_reg->type == VALUE_SYMBOL);

    // save the register
    // to the stack
    return pool_new_pair(
        m->pool,
        pool_new_number(m->pool, INST_SAVE),
        get_register(m, src_reg->symbol));
}

static value* process_restore(machine* m, value* source) {
    value* dst_reg = source->car;
    assert(dst_reg->type == VALUE_SYMBOL);

    // restore the register
    // from the stack
    return pool_new_pair(
        m->pool,
        pool_new_number(m->pool, INST_RESTORE),
        get_register(m, dst_reg->symbol));
}

static void process_code(machine* m, value* source) {
    value* code = m->code;
    value* label = NULL;

    while (source != NULL) {
        value* line = source->car;
        if (line->type == VALUE_SYMBOL) {
            // create the label
            label = get_label(m, line->symbol);
        } else {
            assert(line->type == VALUE_PAIR);
            assert(line->car != NULL);
            assert(line->cdr != NULL);
            assert(line->car->type == VALUE_SYMBOL);

            value* inst = NULL;
            if (strcmp(line->car->symbol, "assign") == 0) {
                inst = process_assign(m, line->cdr);
            } else if (strcmp(line->car->symbol, "perform") == 0) {
                inst = process_perform(m, line->cdr);
            } else if (strcmp(line->car->symbol, "test") == 0) {
                inst = process_test(m, line->cdr);
            } else if (strcmp(line->car->symbol, "branch") == 0) {
                inst = process_branch(m, line->cdr);
            } else if (strcmp(line->car->symbol, "goto") == 0) {
                inst = process_goto(m, line->cdr);
            } else if (strcmp(line->car->symbol, "save") == 0) {
                inst = process_save(m, line->cdr);
            } else if (strcmp(line->car->symbol, "restore") == 0) {
                inst = process_restore(m, line->cdr);
            }
            assert(inst != NULL);

            code->cdr = pool_new_pair(m->pool, inst, NULL);
            code = code->cdr;

            if (label != NULL) {
                // point the label to the
                // following code statement
                label->car = code;
                label = NULL;
            }
        }

        source = source->cdr;
    }
}

static void execute_assign(machine* m, value* inst) {
    value* dst_reg = inst->car;
    value* src = inst->cdr;

    // assign to the dst register from
    // src register, label, or const
    dst_reg->car = src->car;
    // advance the pc
    m->pc = m->pc->cdr;
}

static void execute_call(machine* m, value* inst) {
    value* dst = inst->car;
    value* op_record = inst->cdr->car;
    value* args = inst->cdr->cdr;

    // call the builtin op
    value* result = NULL;
    if (op_record->car == NULL) {
        result = pool_new_error(m->pool, "undefined op '%s'", op_record->cdr->symbol);
    } else {
        builtin op = (builtin)(long)op_record->car->number;
        result = op(m, args);
    }

    if (result != NULL && result->type == VALUE_ERROR) {
        // set the output register to the error
        m->val->car = result;
        // halt immediately
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

static void execute_test(machine* m, value* inst) {
    value* op_record = inst->car;
    value* args = inst->cdr;

    // call the builtin op
    value* result = NULL;
    if (op_record->car == NULL) {
        result = pool_new_error(m->pool, "undefined op '%s'", op_record->cdr->symbol);
    } else {
        builtin op = (builtin)(long)op_record->car->number;
        result = op(m, args);
    }

    if (result != NULL && result->type == VALUE_ERROR) {
        // set the output register to the error and halt
        m->val->car = result;
        // halt immediately
        m->pc = NULL;
    } else {
        // set the flag from the result
        set_flag(m, result);
        // advance the pc
        m->pc = m->pc->cdr;
    }
}

static void execute_branch(machine* m, value* inst) {
    value* label = inst;

    if (get_flag(m)) {
        // flag is 1: jump to the label
        m->pc = label->car;
    } else {
        // flag is 0: advance the pc
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

    // push the src register to the stack
    push_to_stack(m, src->car);
}

static void execute_restore(machine* m, value* inst) {
    value* dst = inst;

    // pop the src register from the stack
    dst->car = pop_from_stack(m);
}

static void (*execute_fns[])(machine*, value*) = {
    execute_assign,
    execute_call,
    execute_test,
    execute_branch,
    execute_goto,
    execute_save,
    execute_restore,
};

static void execute_next_instruction(machine* m) {
    value* inst = m->pc->car;
    instruction_type type = (int)inst->car->number;
    execute_fns[type](m, inst->cdr);
}

void machine_init(machine* m, value* code, char* output_register_name) {
    m->pool = malloc(sizeof(pool));
    pool_init(m->pool);

    create_backbone(m, output_register_name);
    pool_register_root(m->pool, m->root);
    process_code(m, code);
}

void machine_cleanup(machine* m) {
    pool_unregister_root(m->pool, m->root);

    pool_cleanup(m->pool);
    free(m->pool);
}

void machine_add_op(machine* m, char* name, builtin op) {
    value* op_record = get_op(m, name);
    op_record->car = pool_new_number(m->pool, (long)op);
}

void machine_write_to_register(machine* m, char* name, value* v) {
    value* reg_record = get_register(m, name);
    reg_record->car = pool_import(m->pool, v);
}

value* machine_read_from_register(machine* m, char* name) {
    value* reg_record = get_register(m, name);
    return pool_export(m->pool, reg_record->car);
}

value* machine_read_output(machine* m) {
    return pool_export(m->pool, m->val->car);
}

void machine_run(machine* m) {
    clear_stack(m);

    m->pc = m->code->cdr;
    while (m->pc != NULL) {
        execute_next_instruction(m);
    }

    pool_collect_garbage(m->pool);
}
