#include "eval.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "env.h"
#include "machine.h"
#include "parse.h"
#include "pool.h"
#include "value.h"

static int is_tagged_list(value* v, char* tag) {
    return (
        v != NULL &&
        v->type == VALUE_PAIR &&
        v->car != NULL &&
        v->car->type == VALUE_SYMBOL &&
        strcmp(v->car->symbol, tag) == 0);
}

static value* make_lambda(pool* p, value* params, value* body) {
    return pool_new_pair(
        p,
        pool_new_symbol(p, "lambda"),
        pool_new_pair(p, params, body));
}

static value* op_is_self_evaluating(machine* m, value* args) {
    value* exp = args->car->car;

    return pool_new_bool(
        m->pool,
        (exp == NULL ||
         exp->type == VALUE_NUMBER ||
         exp->type == VALUE_STRING ||
         exp->type == VALUE_BOOL));
}

static value* op_is_variable(machine* m, value* args) {
    value* exp = args->car->car;

    return pool_new_bool(
        m->pool,
        (exp != NULL && exp->type == VALUE_SYMBOL));
}

static value* op_is_quoted(machine* m, value* args) {
    value* exp = args->car->car;

    return pool_new_bool(
        m->pool,
        is_tagged_list(exp, "quote"));
}

static value* op_is_assignment(machine* m, value* args) {
    value* exp = args->car->car;

    return pool_new_bool(
        m->pool,
        is_tagged_list(exp, "set!"));
}

static value* op_is_definition(machine* m, value* args) {
    value* exp = args->car->car;

    return pool_new_bool(
        m->pool,
        is_tagged_list(exp, "define"));
}

static value* op_is_if(machine* m, value* args) {
    value* exp = args->car->car;

    return pool_new_bool(
        m->pool,
        is_tagged_list(exp, "if"));
}

static value* op_is_lambda(machine* m, value* args) {
    value* exp = args->car->car;

    return pool_new_bool(
        m->pool,
        is_tagged_list(exp, "lambda"));
}

static value* op_is_begin(machine* m, value* args) {
    value* exp = args->car->car;

    return pool_new_bool(
        m->pool,
        is_tagged_list(exp, "begin"));
}

static value* op_is_application(machine* m, value* args) {
    value* exp = args->car->car;

    return pool_new_bool(
        m->pool,
        (exp != NULL && exp->type == VALUE_PAIR));
}

static value* op_is_true(machine* m, value* args) {
    value* exp = args->car->car;

    return pool_new_bool(m->pool, value_is_true(exp));
}

static value* op_get_text_of_quotation(machine* m, value* args) {
    value* exp = args->car->car;

    return exp->cdr->car;
}

static value* op_get_assignment_variable(machine* m, value* args) {
    value* exp = args->car->car;

    return exp->cdr->car;
}

static value* op_get_assignment_value(machine* m, value* args) {
    value* exp = args->car->car;

    return exp->cdr->cdr->car;
}

static value* op_get_definition_variable(machine* m, value* args) {
    value* exp = args->car->car;

    if (exp->cdr->car->type == VALUE_SYMBOL) {
        return exp->cdr->car;
    } else {
        return exp->cdr->car->car;
    }
}

static value* op_get_definition_value(machine* m, value* args) {
    value* exp = args->car->car;

    if (exp->cdr->car->type == VALUE_SYMBOL) {
        return exp->cdr->cdr->car;
    } else {
        return make_lambda(m->pool, exp->cdr->car->cdr, exp->cdr->cdr);
    }
}

static value* op_get_if_predicate(machine* m, value* args) {
    value* exp = args->car->car;

    return exp->cdr->car;
}

static value* op_get_if_consequent(machine* m, value* args) {
    value* exp = args->car->car;

    return exp->cdr->cdr->car;
}

static value* op_get_if_alternative(machine* m, value* args) {
    value* exp = args->car->car;

    if (exp->cdr->cdr->cdr != NULL) {
        return exp->cdr->cdr->cdr->car;
    } else {
        return pool_new_bool(m->pool, 0);
    }
}

static value* op_get_lambda_parameters(machine* m, value* args) {
    value* exp = args->car->car;

    return exp->cdr->car;
}

static value* op_get_lambda_body(machine* m, value* args) {
    value* exp = args->car->car;

    return exp->cdr->cdr;
}

static value* op_get_begin_actions(machine* m, value* args) {
    value* exp = args->car->car;

    return exp->cdr;
}

static value* op_is_last_exp(machine* m, value* args) {
    value* seq = args->car->car;

    return pool_new_bool(m->pool, seq->cdr == NULL);
}

static value* op_get_first_exp(machine* m, value* args) {
    value* seq = args->car->car;

    return seq->car;
}

static value* op_get_rest_exps(machine* m, value* args) {
    value* seq = args->car->car;

    return seq->cdr;
}

static value* op_get_operator(machine* m, value* args) {
    value* compound = args->car->car;

    return compound->car;
}

static value* op_get_operands(machine* m, value* args) {
    value* compound = args->car->car;

    return compound->cdr;
}

static value* op_is_no_operands(machine* m, value* args) {
    value* operands = args->car->car;

    return pool_new_bool(m->pool, operands == NULL);
}

static value* op_is_last_operand(machine* m, value* args) {
    value* operands = args->car->car;

    return pool_new_bool(m->pool, operands->cdr == NULL);
}

static value* op_get_first_operand(machine* m, value* args) {
    value* operands = args->car->car;

    return operands->car;
}

static value* op_get_rest_operands(machine* m, value* args) {
    value* operands = args->car->car;

    return operands->cdr;
}

static value* op_make_empty_arglist(machine* m, value* args) {
    return NULL;
}

static value* op_adjoin_arg(machine* m, value* args) {
    value* new_value = args->car->car;
    value* arg_list = args->cdr->car->car;

    value* new_arg = pool_new_pair(m->pool, new_value, NULL);

    if (arg_list == NULL) {
        // initialize the list
        arg_list = new_arg;
    } else {
        // append to the list
        value* last_arg = arg_list;
        while (last_arg->cdr != NULL) {
            last_arg = last_arg->cdr;
        }
        last_arg->cdr = new_arg;
    }

    return arg_list;
}

static value* op_is_primitive_procedure(machine* m, value* args) {
    value* proc = args->car->car;

    return pool_new_bool(
        m->pool,
        (proc != NULL && proc->type == VALUE_BUILTIN));
}

static value* op_is_compound_procedure(machine* m, value* args) {
    value* proc = args->car->car;

    return pool_new_bool(
        m->pool,
        (proc != NULL && proc->type == VALUE_LAMBDA));
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

static value* op_make_compound_procedure(machine* m, value* args) {
    value* params = args->car->car;
    value* body = args->cdr->car->car;
    value* env = args->cdr->cdr->car->car;

    return pool_new_lambda(
        m->pool,
        pool_new_pair(
            m->pool,
            params,
            body),
        env);
}

static value* op_get_procedure_parameters(machine* m, value* args) {
    value* proc = args->car->car;

    return proc->car->car;
}

static value* op_get_procedure_body(machine* m, value* args) {
    value* proc = args->car->car;

    return proc->car->cdr;
}

static value* op_get_procedure_environment(machine* m, value* args) {
    value* proc = args->car->car;

    return proc->cdr;
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

static value* op_signal_error(machine* m, value* args) {
    value* text = args->car->car;

    static char buffer[16384];
    static char message[16384];

    char* running = message;
    running += sprintf(running, "%s", text->symbol);

    // append all further
    // params to the message
    value* param = args->cdr;
    while (param != NULL) {
        value_to_str(param->car->car, buffer);
        running += sprintf(running, " '%s'", buffer);
        param = param->cdr;
    }

    return pool_new_error(m->pool, message);
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
    machine_bind_op(m, "assignment?", op_is_assignment);
    machine_bind_op(m, "definition?", op_is_definition);
    machine_bind_op(m, "if?", op_is_if);
    machine_bind_op(m, "lambda?", op_is_lambda);
    machine_bind_op(m, "begin?", op_is_begin);
    machine_bind_op(m, "application?", op_is_application);
    machine_bind_op(m, "true?", op_is_true);

    machine_bind_op(m, "text-of-quotation", op_get_text_of_quotation);
    machine_bind_op(m, "assignment-variable", op_get_assignment_variable);
    machine_bind_op(m, "assignment-value", op_get_assignment_value);
    machine_bind_op(m, "definition-variable", op_get_definition_variable);
    machine_bind_op(m, "definition-value", op_get_definition_value);
    machine_bind_op(m, "if-predicate", op_get_if_predicate);
    machine_bind_op(m, "if-consequent", op_get_if_consequent);
    machine_bind_op(m, "if-alternative", op_get_if_alternative);
    machine_bind_op(m, "lambda-parameters", op_get_lambda_parameters);
    machine_bind_op(m, "lambda-body", op_get_lambda_body);
    machine_bind_op(m, "begin-actions", op_get_begin_actions);

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
    machine_bind_op(m, "apply-primitive-procedure", op_apply_primitive_procedure);
    machine_bind_op(m, "make-compound-procedure", op_make_compound_procedure);
    machine_bind_op(m, "procedure-parameters", op_get_procedure_parameters);
    machine_bind_op(m, "procedure-body", op_get_procedure_body);
    machine_bind_op(m, "procedure-environment", op_get_procedure_environment);

    machine_bind_op(m, "lookup-variable-value", op_lookup_variable_value);
    machine_bind_op(m, "set-variable-value!", op_set_variable_value);
    machine_bind_op(m, "define-variable!", op_define_variable);
    machine_bind_op(m, "extend-environment", op_extend_environment);

    machine_bind_op(m, "signal-error", op_signal_error);
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

void eval_init(eval* e, char* path_to_code) {
    value* code = parse_from_file(path_to_code);
    assert(code->type != VALUE_ERROR);

    e->machine = malloc(sizeof(machine));
    machine_init(e->machine, code, "val");
    value_dispose(code);

    bind_machine_ops(e);

    e->env = machine_get_register(e->machine, "env");
    e->env->car = make_global_environment(e);
}

void eval_cleanup(eval* e) {
    machine_cleanup(e->machine);
    free(e->machine);
}

value* eval_evaluate(eval* e, value* v) {
    machine_copy_to_register(e->machine, "exp", v);
    machine_run(e->machine);

    return machine_export_output(e->machine);
}

void eval_reset_env(eval* e) {
    e->env->car = make_global_environment(e);
}
