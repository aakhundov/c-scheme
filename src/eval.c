#include "eval.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "comp.h"
#include "const.h"
#include "env.h"
#include "machine.h"
#include "map.h"
#include "parse.h"
#include "pool.h"
#include "syntax.h"
#include "value.h"

#define ASSERT_NUM_ARGS(p, args, expected_num_args)  \
    {                                                \
        size_t num_args = 0;                         \
        value* arg = args;                           \
        while (arg != NULL) {                        \
            num_args++;                              \
            arg = arg->cdr;                          \
        }                                            \
        if (num_args != expected_num_args) {         \
            return pool_new_error(                   \
                p, "expects %d arg%s, but got %d",   \
                expected_num_args,                   \
                (expected_num_args == 1 ? "" : "s"), \
                num_args);                           \
        }                                            \
    }

#define ASSERT_MIN_NUM_ARGS(p, args, min_expected_num_args) \
    {                                                       \
        size_t num_args = 0;                                \
        value* arg = args;                                  \
        while (arg != NULL) {                               \
            num_args++;                                     \
            arg = arg->cdr;                                 \
        }                                                   \
        if (num_args < min_expected_num_args) {             \
            return pool_new_error(                          \
                p, "expects at least %d arg%s, but got %d", \
                min_expected_num_args,                      \
                (min_expected_num_args == 1 ? "" : "s"),    \
                num_args);                                  \
        }                                                   \
    }

#define ASSERT_ARG_TYPE(p, args, ordinal, expected_type) \
    {                                                    \
        size_t i = 0;                                    \
        value* arg = args;                               \
        while (arg != NULL && i < ordinal) {             \
            arg = arg->cdr;                              \
            i++;                                         \
        }                                                \
        if (i < ordinal || arg == NULL) {                \
            return pool_new_error(                       \
                p, "arg #%d of type %s is missing",      \
                ordinal, get_type_name(expected_type));  \
        } else if (arg->car == NULL) {                   \
            return pool_new_error(                       \
                p, "arg #%d must be %s, but got ()",     \
                ordinal, get_type_name(expected_type));  \
        } else if (arg->car->type != expected_type) {    \
            static char buffer[BUFFER_SIZE];             \
            value_to_str(arg->car, buffer);              \
            return pool_new_error(                       \
                p, "arg #%d must be %s, but is %s %s",   \
                ordinal,                                 \
                get_type_name(expected_type),            \
                get_type_name(arg->car->type),           \
                buffer);                                 \
        }                                                \
    }

#define ASSERT_ALL_ARGS_TYPE(p, args, offset, expected_type)   \
    {                                                          \
        size_t i = 0;                                          \
        value* arg = args;                                     \
        while (arg != NULL) {                                  \
            if (i >= offset) {                                 \
                if (arg->car == NULL) {                        \
                    return pool_new_error(                     \
                        p, "arg #%d must be %s, but got ()",   \
                        i, get_type_name(expected_type));      \
                } else if (arg->car->type != expected_type) {  \
                    static char buffer[BUFFER_SIZE];           \
                    value_to_str(arg->car, buffer);            \
                    return pool_new_error(                     \
                        p, "arg #%d must be %s, but is %s %s", \
                        i,                                     \
                        get_type_name(expected_type),          \
                        get_type_name(arg->car->type),         \
                        buffer);                               \
                }                                              \
            }                                                  \
            arg = arg->cdr;                                    \
            i++;                                               \
        }                                                      \
    }

static value* op_check_quoted(machine* m, value* args) {
    value* exp = args->car->car;

    return check_quoted(m->pool, exp);
}

static value* op_get_text_of_quotation(machine* m, value* args) {
    value* exp = args->car->car;

    return get_text_of_quotation(m->pool, exp);
}

static value* op_check_assignment(machine* m, value* args) {
    value* exp = args->car->car;

    return check_assignment(m->pool, exp);
}

static value* op_get_assignment_variable(machine* m, value* args) {
    value* exp = args->car->car;

    return get_assignment_variable(m->pool, exp);
}

static value* op_get_assignment_value(machine* m, value* args) {
    value* exp = args->car->car;

    return get_assignment_value(m->pool, exp);
}

static value* op_check_definition(machine* m, value* args) {
    value* exp = args->car->car;

    return check_definition(m->pool, exp);
}

static value* op_get_definition_variable(machine* m, value* args) {
    value* exp = args->car->car;

    return get_definition_variable(m->pool, exp);
}

static value* op_get_definition_value(machine* m, value* args) {
    value* exp = args->car->car;

    return get_definition_value(m->pool, exp);
}

static value* op_check_if(machine* m, value* args) {
    value* exp = args->car->car;

    return check_if(m->pool, exp);
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

static value* op_check_lambda(machine* m, value* args) {
    value* exp = args->car->car;

    return check_lambda(m->pool, exp);
}

static value* op_get_lambda_parameters(machine* m, value* args) {
    value* exp = args->car->car;

    return get_lambda_parameters(m->pool, exp);
}

static value* op_get_lambda_body(machine* m, value* args) {
    value* exp = args->car->car;

    return get_lambda_body(m->pool, exp);
}

static value* op_check_let(machine* m, value* args) {
    value* exp = args->car->car;

    return check_let(m->pool, exp);
}

static value* op_transform_let(machine* m, value* args) {
    value* exp = args->car->car;

    return transform_let(m->pool, exp);
}

static value* op_check_begin(machine* m, value* args) {
    value* exp = args->car->car;

    return check_begin(m->pool, exp);
}

static value* op_get_begin_actions(machine* m, value* args) {
    value* exp = args->car->car;

    return get_begin_actions(m->pool, exp);
}

static value* op_check_cond(machine* m, value* args) {
    value* exp = args->car->car;

    return check_cond(m->pool, exp);
}

static value* op_transform_cond(machine* m, value* args) {
    value* exp = args->car->car;

    return transform_cond(m->pool, exp);
}

static value* op_check_and(machine* m, value* args) {
    value* exp = args->car->car;

    return check_and(m->pool, exp);
}

static value* op_get_and_expressions(machine* m, value* args) {
    value* exp = args->car->car;

    return get_and_expressions(m->pool, exp);
}

static value* op_check_or(machine* m, value* args) {
    value* exp = args->car->car;

    return check_or(m->pool, exp);
}

static value* op_get_or_expressions(machine* m, value* args) {
    value* exp = args->car->car;

    return get_or_expressions(m->pool, exp);
}

static value* op_check_eval(machine* m, value* args) {
    value* exp = args->car->car;

    return check_eval(m->pool, exp);
}

static value* op_get_eval_expression(machine* m, value* args) {
    value* exp = args->car->car;

    return get_eval_expression(m->pool, exp);
}

static value* op_check_apply(machine* m, value* args) {
    value* exp = args->car->car;

    return check_apply(m->pool, exp);
}

static value* op_get_apply_operator(machine* m, value* args) {
    value* exp = args->car->car;

    return get_apply_operator(m->pool, exp);
}

static value* op_get_apply_arguments(machine* m, value* args) {
    value* exp = args->car->car;

    return get_apply_arguments(m->pool, exp);
}

static value* op_check_apply_arguments(machine* m, value* args) {
    value* apply_args = args->car->car;

    return check_apply_arguments(m->pool, apply_args);
}

static value* op_check_application(machine* m, value* args) {
    value* apply_args = args->car->car;

    return check_application(m->pool, apply_args);
}

static value* op_is_true(machine* m, value* args) {
    value* exp = args->car->car;

    return pool_new_bool(m->pool, is_true(m->pool, exp));
}

static value* op_is_false(machine* m, value* args) {
    value* exp = args->car->car;

    return pool_new_bool(m->pool, is_false(m->pool, exp));
}

static value* op_make_true(machine* m, value* args) {
    return pool_new_bool(m->pool, 1);
}

static value* op_make_false(machine* m, value* args) {
    return pool_new_bool(m->pool, 0);
}

static value* op_has_no_exps(machine* m, value* args) {
    value* seq = args->car->car;

    return pool_new_bool(m->pool, has_no_exps(m->pool, seq));
}

static value* op_is_last_exp(machine* m, value* args) {
    value* seq = args->car->car;

    return pool_new_bool(m->pool, is_last_exp(m->pool, seq));
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

static value* op_has_no_operands(machine* m, value* args) {
    value* operands = args->car->car;

    return pool_new_bool(m->pool, has_no_operands(m->pool, operands));
}

static value* op_is_last_operand(machine* m, value* args) {
    value* operands = args->car->car;

    return pool_new_bool(m->pool, is_last_operand(m->pool, operands));
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

    return pool_new_bool(m->pool, is_primitive_procedure(m->pool, proc));
}

static value* op_is_compound_procedure(machine* m, value* args) {
    value* proc = args->car->car;

    return pool_new_bool(m->pool, is_compound_procedure(m->pool, proc));
}

static value* op_is_compiled_procedure(machine* m, value* args) {
    value* proc = args->car->car;

    return pool_new_bool(m->pool, is_compiled_procedure(m->pool, proc));
}

static value* op_get_compound_parameters(machine* m, value* args) {
    value* proc = args->car->car;

    return get_compound_parameters(m->pool, proc);
}

static value* op_get_compound_body(machine* m, value* args) {
    value* proc = args->car->car;

    return get_compound_body(m->pool, proc);
}

static value* op_get_compound_environment(machine* m, value* args) {
    value* proc = args->car->car;

    return get_compound_environment(m->pool, proc);
}

static value* op_make_compound_procedure(machine* m, value* args) {
    value* params = args->car->car;
    value* body = args->cdr->car->car;
    value* env = args->cdr->cdr->car->car;

    return make_compound_procedure(m->pool, params, body, env);
}

static value* op_get_compiled_entry(machine* m, value* args) {
    value* proc = args->car->car;

    return get_compiled_entry(m->pool, proc);
}

static value* op_get_compiled_environment(machine* m, value* args) {
    value* proc = args->car->car;

    return get_compiled_environment(m->pool, proc);
}

static value* op_make_compiled_procedure(machine* m, value* args) {
    value* entry = args->car->car;
    value* env = args->cdr->car->car;

    return make_compiled_procedure(m->pool, entry, env);
}

static value* op_signal_error(machine* m, value* args) {
    value* error_message = args->car->car;
    value* error_args = NULL;

    value* rest = args->cdr;
    while (rest != NULL) {
        error_args = pool_new_pair(m->pool, rest->car->car, error_args);
        rest = rest->cdr;
    }

    static char buffer[BUFFER_SIZE];
    format_args(error_message, error_args, buffer);

    return pool_new_error(m->pool, buffer);
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

    map_record* record = env_lookup(env, name->symbol, 1);

    if (record == NULL) {
        return pool_new_error(m->pool, "%s is unbound", name->symbol);
    } else {
        return env_get_value(record);
    }
}

static value* op_set_variable_value(machine* m, value* args) {
    value* name = args->car->car;
    value* val = args->cdr->car->car;
    value* env = args->cdr->cdr->car->car;

    map_record* record = env_lookup(env, name->symbol, 1);

    if (record == NULL) {
        return pool_new_error(m->pool, "%s is unbound", name->symbol);
    } else {
        env_update_value(record, val);
        return pool_new_info(m->pool, "%s is updated", name->symbol);
    }
}

static value* op_define_variable(machine* m, value* args) {
    value* name = args->car->car;
    value* val = args->cdr->car->car;
    value* env = args->cdr->cdr->car->car;

    map_record* record = env_lookup(env, name->symbol, 0);

    if (record == NULL) {
        env_add_value(env, name->symbol, val, m->pool);
        return pool_new_info(m->pool, "%s is defined", name->symbol);
    } else {
        env_update_value(record, val);
        return pool_new_info(m->pool, "%s is updated", name->symbol);
    }
}

static value* op_extend_environment(machine* m, value* args) {
    value* names = args->car->car;
    value* values = args->cdr->car->car;
    value* parent_env = args->cdr->cdr->car->car;

    value* n = names;
    value* v = values;
    while (n != NULL) {
        if (n->type == VALUE_SYMBOL) {
            n = NULL;
            v = NULL;
            break;
        } else if (v == NULL) {
            break;
        }
        n = n->cdr;
        v = v->cdr;
    }

    if (n != NULL || v != NULL) {
        static char lambda_buffer[BUFFER_SIZE];
        static char values_buffer[BUFFER_SIZE];

        value* lambda = machine_get_register(m, "proc")->car;
        value_to_str(lambda, lambda_buffer);
        value_to_str(values, values_buffer);

        return pool_new_error(
            m->pool, "the arguments %s don't match %s",
            values_buffer, lambda_buffer);
    } else {
        value* env = pool_new_env(m->pool);

        while (names != NULL) {
            if (names->type == VALUE_SYMBOL) {
                // the rest of the values are bound
                // to the parameter y in (x . y)
                env_add_value(env, names->symbol, values, m->pool);
                break;
            } else {
                env_add_value(env, names->car->symbol, values->car, m->pool);
            }
            names = names->cdr;
            values = values->cdr;
        }

        return env_extend(env, parent_env);
    }
}

static value* op_is_dispatch_table_ready(machine* m, value* args) {
    value* dispatch_reg = args->car;

    return pool_new_bool(m->pool, dispatch_reg->car != NULL);
}

static value* op_make_dispatch_table(machine* m, value* args) {
    return pool_new_env(m->pool);
}

static value* op_add_dispatch_record(machine* m, value* args) {
    value* dispatch = args->car->car;
    value* name = args->cdr->car->car;
    value* label = args->cdr->cdr->car;

    env_add_value(dispatch, name->symbol, label, m->pool);

    return dispatch;
}

static value* op_dispatch_on_type(machine* m, value* args) {
    value* exp = args->car->car;
    value* dispatch = args->cdr->car->car;

    map_record* record = NULL;
    if (is_self_evaluating(exp)) {
        // self-evaluating expression
        record = env_lookup(dispatch, "self", 0);
    } else if (is_variable(exp)) {
        // named variable
        record = env_lookup(dispatch, "var", 0);
    } else if (starts_with_symbol(exp)) {
        // maybe special form (list starting with a symbol)
        record = env_lookup(dispatch, exp->car->symbol, 0);
    }

    if (record == NULL) {
        // default dispatch (application)
        record = env_lookup(dispatch, "default", 0);
    }

    return env_get_value(record)->car;
}

static value* op_cons(machine* m, value* args) {
    value* first = args->car->car;
    value* second = args->cdr->car->car;

    return pool_new_pair(m->pool, first, second);
}

static void bind_machine_ops(eval* e) {
    machine* m = e->machine;

    machine_bind_op(m, "check-quoted", op_check_quoted);
    machine_bind_op(m, "text-of-quotation", op_get_text_of_quotation);

    machine_bind_op(m, "check-assignment", op_check_assignment);
    machine_bind_op(m, "assignment-variable", op_get_assignment_variable);
    machine_bind_op(m, "assignment-value", op_get_assignment_value);

    machine_bind_op(m, "check-definition", op_check_definition);
    machine_bind_op(m, "definition-variable", op_get_definition_variable);
    machine_bind_op(m, "definition-value", op_get_definition_value);

    machine_bind_op(m, "check-if", op_check_if);
    machine_bind_op(m, "if-predicate", op_get_if_predicate);
    machine_bind_op(m, "if-consequent", op_get_if_consequent);
    machine_bind_op(m, "if-alternative", op_get_if_alternative);

    machine_bind_op(m, "check-lambda", op_check_lambda);
    machine_bind_op(m, "lambda-parameters", op_get_lambda_parameters);
    machine_bind_op(m, "lambda-body", op_get_lambda_body);

    machine_bind_op(m, "check-let", op_check_let);
    machine_bind_op(m, "transform-let", op_transform_let);

    machine_bind_op(m, "check-begin", op_check_begin);
    machine_bind_op(m, "begin-actions", op_get_begin_actions);

    machine_bind_op(m, "check-cond", op_check_cond);
    machine_bind_op(m, "transform-cond", op_transform_cond);

    machine_bind_op(m, "check-and", op_check_and);
    machine_bind_op(m, "and-expressions", op_get_and_expressions);

    machine_bind_op(m, "check-or", op_check_or);
    machine_bind_op(m, "or-expressions", op_get_or_expressions);

    machine_bind_op(m, "check-eval", op_check_eval);
    machine_bind_op(m, "eval-expression", op_get_eval_expression);

    machine_bind_op(m, "check-apply", op_check_apply);
    machine_bind_op(m, "apply-operator", op_get_apply_operator);
    machine_bind_op(m, "apply-arguments", op_get_apply_arguments);
    machine_bind_op(m, "check-apply-args", op_check_apply_arguments);

    machine_bind_op(m, "check-application", op_check_application);

    machine_bind_op(m, "true?", op_is_true);
    machine_bind_op(m, "false?", op_is_false);
    machine_bind_op(m, "make-true", op_make_true);
    machine_bind_op(m, "make-false", op_make_false);

    machine_bind_op(m, "no-exps?", op_has_no_exps);
    machine_bind_op(m, "last-exp?", op_is_last_exp);
    machine_bind_op(m, "first-exp", op_get_first_exp);
    machine_bind_op(m, "rest-exps", op_get_rest_exps);

    machine_bind_op(m, "operator", op_get_operator);
    machine_bind_op(m, "operands", op_get_operands);
    machine_bind_op(m, "no-operands?", op_has_no_operands);
    machine_bind_op(m, "last-operand?", op_is_last_operand);
    machine_bind_op(m, "first-operand", op_get_first_operand);
    machine_bind_op(m, "rest-operands", op_get_rest_operands);

    machine_bind_op(m, "make-empty-arglist", op_make_empty_arglist);
    machine_bind_op(m, "adjoin-arg", op_adjoin_arg);

    machine_bind_op(m, "primitive-procedure?", op_is_primitive_procedure);
    machine_bind_op(m, "compound-procedure?", op_is_compound_procedure);
    machine_bind_op(m, "compiled-procedure?", op_is_compiled_procedure);

    machine_bind_op(m, "compound-parameters", op_get_compound_parameters);
    machine_bind_op(m, "compound-body", op_get_compound_body);
    machine_bind_op(m, "compound-environment", op_get_compound_environment);
    machine_bind_op(m, "make-compound-procedure", op_make_compound_procedure);

    machine_bind_op(m, "compiled-entry", op_get_compiled_entry);
    machine_bind_op(m, "compiled-environment", op_get_compiled_environment);
    machine_bind_op(m, "make-compiled-procedure", op_make_compiled_procedure);

    machine_bind_op(m, "signal-error", op_signal_error);

    machine_bind_op(m, "apply-primitive-procedure", op_apply_primitive_procedure);

    machine_bind_op(m, "lookup-variable-value", op_lookup_variable_value);
    machine_bind_op(m, "set-variable-value!", op_set_variable_value);
    machine_bind_op(m, "define-variable!", op_define_variable);
    machine_bind_op(m, "extend-environment", op_extend_environment);

    machine_bind_op(m, "dispatch-table-ready?", op_is_dispatch_table_ready);
    machine_bind_op(m, "make-dispatch-table", op_make_dispatch_table);
    machine_bind_op(m, "add-dispatch-record", op_add_dispatch_record);
    machine_bind_op(m, "dispatch-on-type", op_dispatch_on_type);

    machine_bind_op(m, "cons", op_cons);
}

static value* prim_car(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_PAIR);

    value* pair = args->car;

    return pair->car;
}

static value* prim_cdr(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_PAIR);

    value* pair = args->car;

    return pair->cdr;
}

static value* prim_cons(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 2);

    value* first = args->car;
    value* second = args->cdr->car;

    return pool_new_pair(m->pool, first, second);
}

static value* prim_list(machine* m, value* args) {
    value* result = NULL;
    value* running = NULL;
    while (args != NULL) {
        value* pair = pool_new_pair(m->pool, args->car, NULL);
        if (running == NULL) {
            result = pair;
        } else {
            running->cdr = pair;
        }
        running = pair;
        args = args->cdr;
    }

    return result;
}

static value* prim_set_car(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 2);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_PAIR);

    value* first = args->car;
    value* second = args->cdr->car;

    first->car = second;

    return pool_new_info(m->pool, "car is set");
}

static value* prim_set_cdr(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 2);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_PAIR);

    value* first = args->car;
    value* second = args->cdr->car;

    first->cdr = second;

    return pool_new_info(m->pool, "cdr is set");
}

static value* prim_add(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        result += args->car->number;
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_sub(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    if (args == NULL) {
        result = -result;
    } else {
        while (args != NULL) {
            result -= args->car->number;
            args = args->cdr;
        }
    }

    return pool_new_number(m->pool, result);
}

static value* prim_mul(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        result *= args->car->number;
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_div(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (args->car->number == 0) {
            return pool_new_error(m->pool, "division by zero");
        }
        result /= args->car->number;
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_remainder(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (args->car->number == 0) {
            return pool_new_error(m->pool, "division by zero");
        }
        result = fmod(result, args->car->number);
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_expt(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        result = pow(result, args->car->number);
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_min(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (args->car->number < result) {
            result = args->car->number;
        }
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_max(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (args->car->number > result) {
            result = args->car->number;
        }
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_abs(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = fabs(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_exp(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = exp(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_log(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    if (args->car->number <= 0) {
        return pool_new_error(m->pool, "can't tage log of a non-positive number");
    }

    double result = log(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_sin(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = sin(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_cos(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = cos(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_tan(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = tan(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_atan(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = atan(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_atan2(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double y = args->car->number;
    double x = args->cdr->car->number;

    double result = atan2(y, x);

    return pool_new_number(m->pool, result);
}

static value* prim_round(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = round(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_floor(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = floor(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_ceiling(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = ceil(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_eq(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (value != args->car->number) {
            return pool_new_bool(m->pool, 0);
        }
        args = args->cdr;
    }

    return pool_new_bool(m->pool, 1);
}

static value* prim_lt(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (value >= args->car->number) {
            return pool_new_bool(m->pool, 0);
        }
        value = args->car->number;
        args = args->cdr;
    }

    return pool_new_bool(m->pool, 1);
}

static value* prim_lte(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (value > args->car->number) {
            return pool_new_bool(m->pool, 0);
        }
        value = args->car->number;
        args = args->cdr;
    }

    return pool_new_bool(m->pool, 1);
}

static value* prim_gt(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (value <= args->car->number) {
            return pool_new_bool(m->pool, 0);
        }
        value = args->car->number;
        args = args->cdr;
    }

    return pool_new_bool(m->pool, 1);
}

static value* prim_gte(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (value < args->car->number) {
            return pool_new_bool(m->pool, 0);
        }
        value = args->car->number;
        args = args->cdr;
    }

    return pool_new_bool(m->pool, 1);
}

static value* prim_not(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    if (is_true(m->pool, arg)) {
        return pool_new_bool(m->pool, 0);
    } else {
        return pool_new_bool(m->pool, 1);
    }
}

static value* prim_number_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, arg != NULL && arg->type == VALUE_NUMBER);
}

static value* prim_symbol_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, arg != NULL && arg->type == VALUE_SYMBOL);
}

static value* prim_string_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, arg != NULL && arg->type == VALUE_STRING);
}

static value* prim_bool_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, arg != NULL && arg->type == VALUE_BOOL);
}

static value* prim_pair_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, arg != NULL && arg->type == VALUE_PAIR);
}

static value* prim_list_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;
    while (arg != NULL) {
        if (arg->type != VALUE_PAIR) {
            return pool_new_bool(m->pool, 0);
        }
        arg = arg->cdr;
    }

    return pool_new_bool(m->pool, 1);
}

static value* prim_null_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, arg == NULL);
}

static value* prim_true_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, is_true(m->pool, arg));
}

static value* prim_false_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, is_false(m->pool, arg));
}

static value* prim_equal_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 2);

    value* v1 = args->car;
    value* v2 = args->cdr->car;

    return pool_new_bool(m->pool, value_equal(v1, v2));
}

static value* prim_eq_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 2);

    value* v1 = args->car;
    value* v2 = args->cdr->car;

    return pool_new_bool(m->pool, v1 == v2);
}

static value* prim_even_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;

    if ((long)value != value) {
        return pool_new_bool(m->pool, 0);
    } else {
        return pool_new_bool(m->pool, (long)value % 2 == 0);
    }
}

static value* prim_odd_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;

    if ((long)value != value) {
        return pool_new_bool(m->pool, 0);
    } else {
        return pool_new_bool(m->pool, (long)value % 2 != 0);
    }
}

static value* prim_error(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_STRING);

    value* error_message = args->car;
    value* error_args = args->cdr;

    static char buffer[BUFFER_SIZE];
    format_args(error_message, error_args, buffer);

    return pool_new_error(m->pool, buffer);
}

static value* prim_info(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_STRING);

    value* info_message = args->car;
    value* info_args = args->cdr;

    static char buffer[BUFFER_SIZE];
    format_args(info_message, info_args, buffer);

    return pool_new_info(m->pool, buffer);
}

static value* prim_display(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);

    while (args != NULL) {
        if (args->car->type == VALUE_SYMBOL || args->car->type == VALUE_STRING) {
            printf("%s", args->car->symbol);
        } else {
            static char buffer[BUFFER_SIZE];
            value_to_str(args->car, buffer);
            printf("%s", buffer);
        }

        if (args->cdr != NULL) {
            printf(" ");
        }

        args = args->cdr;
    }

    return NULL;
}

static value* prim_newline(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 0);

    printf("\n");

    return NULL;
}

static value* prim_collect(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 0);

    size_t values_before = m->pool->size;

    pool_collect_garbage(m->pool);

    size_t values_after = m->pool->size;
    size_t collected = values_before - values_after;
    double percentage = (values_before > 0 ? (double)collected / values_before : 0) * 100;

    return pool_new_info(
        m->pool, "%zu (%.2f%%) from %zu collected",
        collected, percentage, values_before);
}

static value* prim_srand(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_NUMBER);

    srand((int)args->car->number);

    return pool_new_info(m->pool, "RNG was seeded");
}

static value* prim_random(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_NUMBER);

    int upper = (int)args->car->number;
    int result = rand() % upper;

    return pool_new_number(m->pool, result);
}

static value* prim_time(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 0);

    return pool_new_number(m->pool, time(NULL));
}

static value* prim_pretty(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 2);
    ASSERT_ARG_TYPE(m->pool, args, 1, VALUE_NUMBER);

    value* exp = args->car;
    int line_len = (int)args->cdr->car->number;

    static char buffer[BUFFER_SIZE];
    value_to_pretty_str(exp, buffer, line_len);

    return value_new_symbol(buffer);
}

static value* prim_code(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);

    value* exp = args->car;

    return compile(m->pool, exp, "val", "return");
}

static value* prim_compile(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* exp = args->car;

    // compile the expression into machine code
    // target: place the result in the val register
    // lingage: (goto (reg continue)) in the end
    value* code = compile(m->pool, exp, "val", "return");

    if (code->type == VALUE_ERROR) {
        // error in compilation -> return
        return code;
    }

    // add the compiled code to the machine
    value* start = machine_append_code(m, code);
    // and set the position to the code's start
    machine_set_code_position(m, start);

    // the return value here doesn't matter
    // as the machine will continue from the head
    return NULL;
}

static void add_primitive(eval* e, value* env, char* name, builtin fn) {
    pool* p = e->machine->pool;
    env_add_value(env, name, pool_new_builtin(p, fn, name), p);
}

static value* make_global_environment(eval* e) {
    pool* p = e->machine->pool;
    value* env = pool_new_env(e->machine->pool);

    // constants
    env_add_value(env, "#t", pool_new_bool(p, 1), p);
    env_add_value(env, "#f", pool_new_bool(p, 0), p);
    env_add_value(env, "PI", pool_new_number(p, 3.1415926536), p);
    env_add_value(env, "E", pool_new_number(p, 2.7182818285), p);

    // structural
    add_primitive(e, env, "car", prim_car);
    add_primitive(e, env, "cdr", prim_cdr);
    add_primitive(e, env, "cons", prim_cons);
    add_primitive(e, env, "list", prim_list);
    add_primitive(e, env, "set-car!", prim_set_car);
    add_primitive(e, env, "set-cdr!", prim_set_cdr);

    // arithmetic
    add_primitive(e, env, "+", prim_add);
    add_primitive(e, env, "-", prim_sub);
    add_primitive(e, env, "*", prim_mul);
    add_primitive(e, env, "/", prim_div);
    add_primitive(e, env, "remainder", prim_remainder);
    add_primitive(e, env, "expt", prim_expt);
    add_primitive(e, env, "min", prim_min);
    add_primitive(e, env, "max", prim_max);

    // math
    add_primitive(e, env, "abs", prim_abs);
    add_primitive(e, env, "exp", prim_exp);
    add_primitive(e, env, "log", prim_log);
    add_primitive(e, env, "sin", prim_sin);
    add_primitive(e, env, "cos", prim_cos);
    add_primitive(e, env, "tan", prim_tan);
    add_primitive(e, env, "atan", prim_atan);
    add_primitive(e, env, "atan2", prim_atan2);
    add_primitive(e, env, "round", prim_round);
    add_primitive(e, env, "floor", prim_floor);
    add_primitive(e, env, "ceiling", prim_ceiling);

    // relational
    add_primitive(e, env, "=", prim_eq);
    add_primitive(e, env, "<", prim_lt);
    add_primitive(e, env, "<=", prim_lte);
    add_primitive(e, env, ">", prim_gt);
    add_primitive(e, env, ">=", prim_gte);
    add_primitive(e, env, "not", prim_not);

    // predicates
    add_primitive(e, env, "number?", prim_number_q);
    add_primitive(e, env, "symbol?", prim_symbol_q);
    add_primitive(e, env, "string?", prim_string_q);
    add_primitive(e, env, "bool?", prim_bool_q);
    add_primitive(e, env, "pair?", prim_pair_q);
    add_primitive(e, env, "list?", prim_list_q);
    add_primitive(e, env, "null?", prim_null_q);
    add_primitive(e, env, "true?", prim_true_q);
    add_primitive(e, env, "false?", prim_false_q);
    add_primitive(e, env, "equal?", prim_equal_q);
    add_primitive(e, env, "eq?", prim_eq_q);
    add_primitive(e, env, "even?", prim_even_q);
    add_primitive(e, env, "odd?", prim_odd_q);

    // other
    add_primitive(e, env, "error", prim_error);
    add_primitive(e, env, "info", prim_info);
    add_primitive(e, env, "display", prim_display);
    add_primitive(e, env, "newline", prim_newline);
    add_primitive(e, env, "collect", prim_collect);
    add_primitive(e, env, "srand", prim_srand);
    add_primitive(e, env, "random", prim_random);
    add_primitive(e, env, "time", prim_time);
    add_primitive(e, env, "pretty", prim_pretty);

    // compilation
    add_primitive(e, env, "code", prim_code);
    add_primitive(e, env, "compile", prim_compile);

    return env;
}

eval* eval_new(char* path_to_code) {
    eval* e = malloc(sizeof(eval));

    value* code = parse_from_file(path_to_code);
    assert(code->type != VALUE_ERROR);
    e->machine = machine_new(code, "val");
    value_dispose(code);

    bind_machine_ops(e);

    e->env = make_global_environment(e);

    return e;
}

void eval_dispose(eval* e) {
    machine_dispose(e->machine);

    free(e);
}

value* eval_evaluate(eval* e, value* v) {
    machine_get_register(e->machine, "env")->car = e->env;  // set the env
    machine_copy_to_register(e->machine, "exp", v);         // set the input
    machine_run(e->machine);                                // compute the output

    return machine_export_output(e->machine);
}

void eval_reset_env(eval* e) {
    e->env = make_global_environment(e);
}
