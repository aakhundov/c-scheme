#include "eval.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "env.h"
#include "machine.h"
#include "parse.h"
#include "pool.h"
#include "syntax.h"
#include "value.h"

static value* op_is_self_evaluating(machine* m, value* args) {
    value* exp = args->car->car;

    return is_self_evaluating(m->pool, exp);
}

static value* op_is_variable(machine* m, value* args) {
    value* exp = args->car->car;

    return is_variable(m->pool, exp);
}

static value* op_is_quoted(machine* m, value* args) {
    value* exp = args->car->car;

    return is_quoted(m->pool, exp);
}

static value* op_get_text_of_quotation(machine* m, value* args) {
    value* exp = args->car->car;

    return get_text_of_quotation(m->pool, exp);
}

static value* op_is_assignment(machine* m, value* args) {
    value* exp = args->car->car;

    return is_assignment(m->pool, exp);
}

static value* op_get_assignment_variable(machine* m, value* args) {
    value* exp = args->car->car;

    return get_assignment_variable(m->pool, exp);
}

static value* op_get_assignment_value(machine* m, value* args) {
    value* exp = args->car->car;

    return get_assignment_value(m->pool, exp);
}

static value* op_is_definition(machine* m, value* args) {
    value* exp = args->car->car;

    return is_definition(m->pool, exp);
}

static value* op_get_definition_variable(machine* m, value* args) {
    value* exp = args->car->car;

    return get_definition_variable(m->pool, exp);
}

static value* op_get_definition_value(machine* m, value* args) {
    value* exp = args->car->car;

    return get_definition_value(m->pool, exp);
}

static value* op_is_if(machine* m, value* args) {
    value* exp = args->car->car;

    return is_if(m->pool, exp);
}

static value* op_get_if_predicate(machine* m, value* args) {
    value* exp = args->car->car;

    return get_if_predicate(m->pool, exp);
}

static value* op_get_if_consequent(machine* m, value* args) {
    value* exp = args->car->car;

    return get_if_consequent(m->pool, exp);
}

static value* op_get_if_alternative(machine* m, value* args) {
    value* exp = args->car->car;

    return get_if_alternative(m->pool, exp);
}

static value* op_is_lambda(machine* m, value* args) {
    value* exp = args->car->car;

    return is_lambda(m->pool, exp);
}

static value* op_get_lambda_parameters(machine* m, value* args) {
    value* exp = args->car->car;

    return get_lambda_parameters(m->pool, exp);
}

static value* op_get_lambda_body(machine* m, value* args) {
    value* exp = args->car->car;

    return get_lambda_body(m->pool, exp);
}

static value* op_is_begin(machine* m, value* args) {
    value* exp = args->car->car;

    return is_begin(m->pool, exp);
}

static value* op_get_begin_actions(machine* m, value* args) {
    value* exp = args->car->car;

    return get_begin_actions(m->pool, exp);
}

static value* op_is_application(machine* m, value* args) {
    value* exp = args->car->car;

    return is_application(m->pool, exp);
}

static value* op_is_true(machine* m, value* args) {
    value* exp = args->car->car;

    return is_true(m->pool, exp);
}

static value* op_is_last_exp(machine* m, value* args) {
    value* seq = args->car->car;

    return is_last_exp(m->pool, seq);
}

static value* op_get_first_exp(machine* m, value* args) {
    value* seq = args->car->car;

    return get_first_exp(m->pool, seq);
}

static value* op_get_rest_exps(machine* m, value* args) {
    value* seq = args->car->car;

    return get_rest_exps(m->pool, seq);
}

static value* op_get_operator(machine* m, value* args) {
    value* compound = args->car->car;

    return get_operator(m->pool, compound);
}

static value* op_get_operands(machine* m, value* args) {
    value* compound = args->car->car;

    return get_operands(m->pool, compound);
}

static value* op_is_no_operands(machine* m, value* args) {
    value* operands = args->car->car;

    return is_no_operands(m->pool, operands);
}

static value* op_is_last_operand(machine* m, value* args) {
    value* operands = args->car->car;

    return is_last_operand(m->pool, operands);
}

static value* op_get_first_operand(machine* m, value* args) {
    value* operands = args->car->car;

    return get_first_operand(m->pool, operands);
}

static value* op_get_rest_operands(machine* m, value* args) {
    value* operands = args->car->car;

    return get_rest_operands(m->pool, operands);
}

static value* op_make_empty_arglist(machine* m, value* args) {
    return make_empty_arglist(m->pool);
}

static value* op_adjoin_arg(machine* m, value* args) {
    value* new_arg = args->car->car;
    value* arg_list = args->cdr->car->car;

    return adjoin_arg(m->pool, new_arg, arg_list);
}

static value* op_is_primitive_procedure(machine* m, value* args) {
    value* proc = args->car->car;

    return is_primitive_procedure(m->pool, proc);
}

static value* op_is_compound_procedure(machine* m, value* args) {
    value* proc = args->car->car;

    return is_compound_procedure(m->pool, proc);
}

static value* op_get_procedure_parameters(machine* m, value* args) {
    value* proc = args->car->car;

    return get_procedure_parameters(m->pool, proc);
}

static value* op_get_procedure_body(machine* m, value* args) {
    value* proc = args->car->car;

    return get_procedure_body(m->pool, proc);
}

static value* op_get_procedure_environment(machine* m, value* args) {
    value* proc = args->car->car;

    return get_procedure_environment(m->pool, proc);
}

static value* op_make_compound_procedure(machine* m, value* args) {
    value* params = args->car->car;
    value* body = args->cdr->car->car;
    value* env = args->cdr->cdr->car->car;

    return make_compound_procedure(m->pool, params, body, env);
}

static value* op_signal_error(machine* m, value* args) {
    value* error_message = args->car->car;
    value* error_args = NULL;

    value* rest = args->cdr;
    while (rest != NULL) {
        error_args = pool_new_pair(m->pool, rest->car->car, error_args);
        rest = rest->cdr;
    }

    return make_error(m->pool, error_message, error_args);
}

static value* op_apply_primitive_procedure(machine* m, value* args) {
    value* proc = args->car->car;
    value* arg_list = args->cdr->car->car;

    value* result = ((builtin)proc->ptr)(m, arg_list);
    if (result != NULL && result->type == VALUE_ERROR) {
        result = pool_new_error(m->pool, "%s: %s", proc->symbol, result->symbol);
    }

    return result;
}

static value* op_lookup_variable_value(machine* m, value* args) {
    value* name = args->car->car;
    value* env = args->cdr->car->car;

    value* record = lookup_in_env(env, name->symbol, 1);

    if (record == NULL) {
        return pool_new_error(m->pool, "'%s' is unbound", name->symbol);
    } else {
        return record->cdr;
    }
}

static value* op_set_variable_value(machine* m, value* args) {
    value* name = args->car->car;
    value* val = args->cdr->car->car;
    value* env = args->cdr->cdr->car->car;

    value* record = lookup_in_env(env, name->symbol, 1);

    if (record == NULL) {
        return pool_new_error(m->pool, "'%s' is unbound", name->symbol);
    } else {
        record->cdr = val;
        return pool_new_info(m->pool, "'%s' is updated", name->symbol);
    }
}

static value* op_define_variable(machine* m, value* args) {
    value* name = args->car->car;
    value* val = args->cdr->car->car;
    value* env = args->cdr->cdr->car->car;

    value* record = lookup_in_env(env, name->symbol, 0);

    if (record == NULL) {
        add_to_env(env, name->symbol, val, m->pool);
        return pool_new_info(m->pool, "'%s' is defined", name->symbol);
    } else {
        record->cdr = val;
        return pool_new_info(m->pool, "'%s' is updated", name->symbol);
    }
}

static value* op_extend_environment(machine* m, value* args) {
    value* names = args->car->car;
    value* values = args->cdr->car->car;
    value* parent_env = args->cdr->cdr->car->car;

    value* env = pool_new_env(m->pool);

    while (names != NULL) {
        char* name = names->car->symbol;
        value* val = values->car;
        add_to_env(env, name, val, m->pool);

        names = names->cdr;
        values = values->cdr;
    }

    return extend_env(env, parent_env);
}

static value* prim_car(machine* m, value* args) {
    if (args == NULL || args->cdr != NULL) {
        return pool_new_error(m->pool, "exactly 1 arg expected");
    } else if (args->car == NULL || args->car->type != VALUE_PAIR) {
        return pool_new_error(m->pool, "the arg is not a pair");
    }

    value* pair = args->car;

    return pair->car;
}

static value* prim_cdr(machine* m, value* args) {
    if (args == NULL || args->cdr != NULL) {
        return pool_new_error(m->pool, "exactly 1 arg expected");
    } else if (args->car == NULL || args->car->type != VALUE_PAIR) {
        return pool_new_error(m->pool, "the arg is not a pair");
    }

    value* pair = args->car;

    return pair->cdr;
}

static value* prim_cons(machine* m, value* args) {
    if (args == NULL || args->cdr == NULL || args->cdr->cdr != NULL) {
        return pool_new_error(m->pool, "exactly 2 args expected");
    }

    value* first = args->car;
    value* second = args->cdr->car;

    return pool_new_pair(m->pool, first, second);
}

static void bind_machine_ops(eval* e) {
    machine* m = e->machine;

    machine_bind_op(m, "self-evaluating?", op_is_self_evaluating);

    machine_bind_op(m, "variable?", op_is_variable);

    machine_bind_op(m, "quoted?", op_is_quoted);
    machine_bind_op(m, "text-of-quotation", op_get_text_of_quotation);

    machine_bind_op(m, "assignment?", op_is_assignment);
    machine_bind_op(m, "assignment-variable", op_get_assignment_variable);
    machine_bind_op(m, "assignment-value", op_get_assignment_value);

    machine_bind_op(m, "definition?", op_is_definition);
    machine_bind_op(m, "definition-variable", op_get_definition_variable);
    machine_bind_op(m, "definition-value", op_get_definition_value);

    machine_bind_op(m, "if?", op_is_if);
    machine_bind_op(m, "if-predicate", op_get_if_predicate);
    machine_bind_op(m, "if-consequent", op_get_if_consequent);
    machine_bind_op(m, "if-alternative", op_get_if_alternative);

    machine_bind_op(m, "lambda?", op_is_lambda);
    machine_bind_op(m, "lambda-parameters", op_get_lambda_parameters);
    machine_bind_op(m, "lambda-body", op_get_lambda_body);

    machine_bind_op(m, "begin?", op_is_begin);
    machine_bind_op(m, "begin-actions", op_get_begin_actions);

    machine_bind_op(m, "application?", op_is_application);
    machine_bind_op(m, "true?", op_is_true);

    machine_bind_op(m, "last-exp?", op_is_last_exp);
    machine_bind_op(m, "first-exp", op_get_first_exp);
    machine_bind_op(m, "rest-exps", op_get_rest_exps);

    machine_bind_op(m, "operator", op_get_operator);
    machine_bind_op(m, "operands", op_get_operands);
    machine_bind_op(m, "no-operands?", op_is_no_operands);
    machine_bind_op(m, "last-operand?", op_is_last_operand);
    machine_bind_op(m, "first-operand", op_get_first_operand);
    machine_bind_op(m, "rest-operands", op_get_rest_operands);

    machine_bind_op(m, "make-empty-arglist", op_make_empty_arglist);
    machine_bind_op(m, "adjoin-arg", op_adjoin_arg);

    machine_bind_op(m, "primitive-procedure?", op_is_primitive_procedure);
    machine_bind_op(m, "compound-procedure?", op_is_compound_procedure);
    machine_bind_op(m, "make-compound-procedure", op_make_compound_procedure);
    machine_bind_op(m, "procedure-parameters", op_get_procedure_parameters);
    machine_bind_op(m, "procedure-body", op_get_procedure_body);
    machine_bind_op(m, "procedure-environment", op_get_procedure_environment);

    machine_bind_op(m, "signal-error", op_signal_error);

    machine_bind_op(m, "apply-primitive-procedure", op_apply_primitive_procedure);
    machine_bind_op(m, "lookup-variable-value", op_lookup_variable_value);
    machine_bind_op(m, "set-variable-value!", op_set_variable_value);
    machine_bind_op(m, "define-variable!", op_define_variable);
    machine_bind_op(m, "extend-environment", op_extend_environment);
}

static void add_primitive(eval* e, value* env, char* name, builtin fn) {
    pool* p = e->machine->pool;

    add_to_env(env, name, pool_new_builtin(p, fn, name), p);
}

static value* make_global_environment(eval* e) {
    value* env = pool_new_env(e->machine->pool);

    add_primitive(e, env, "car", prim_car);
    add_primitive(e, env, "cdr", prim_cdr);
    add_primitive(e, env, "cons", prim_cons);

    return env;
}

void eval_new(eval** e, char* path_to_code) {
    *e = malloc(sizeof(eval));

    value* code = parse_from_file(path_to_code);
    assert(code->type != VALUE_ERROR);
    machine_new(&((*e)->machine), code, "val");
    value_dispose(code);

    bind_machine_ops(*e);

    (*e)->env = machine_get_register((*e)->machine, "env");
    (*e)->env->car = make_global_environment(*e);
}

void eval_dispose(eval** e) {
    machine_dispose(&((*e)->machine));

    free(*e);
    *e = NULL;
}

value* eval_evaluate(eval* e, value* v) {
    machine_copy_to_register(e->machine, "exp", v);
    machine_run(e->machine);

    return machine_export_output(e->machine);
}

void eval_reset_env(eval* e) {
    e->env->car = make_global_environment(e);
}
