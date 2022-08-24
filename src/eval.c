#include "eval.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "env.h"
#include "machine.h"
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
            static char buffer[16384];                   \
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
                    static char buffer[16384];                 \
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

    static char buffer[16384];
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

    value* record = lookup_in_env(env, name->symbol, 1);

    if (record == NULL) {
        return pool_new_error(m->pool, "%s is unbound", name->symbol);
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
        return pool_new_error(m->pool, "%s is unbound", name->symbol);
    } else {
        record->cdr = val;
        return pool_new_info(m->pool, "%s is updated", name->symbol);
    }
}

static value* op_define_variable(machine* m, value* args) {
    value* name = args->car->car;
    value* val = args->cdr->car->car;
    value* env = args->cdr->cdr->car->car;

    value* record = lookup_in_env(env, name->symbol, 0);

    if (record == NULL) {
        add_to_env(env, name->symbol, val, m->pool);
        return pool_new_info(m->pool, "%s is defined", name->symbol);
    } else {
        record->cdr = val;
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
        static char names_buffer[16384];
        static char values_buffer[16384];

        value_to_str(names, names_buffer);
        value_to_str(values, values_buffer);

        return pool_new_error(
            m->pool, "the arguments %s don't match the parameters %s",
            values_buffer, names_buffer);
    } else {
        value* env = pool_new_env(m->pool);

        while (names != NULL) {
            if (names->type == VALUE_SYMBOL) {
                // rest of the values are bound to y in (x . y)
                add_to_env(env, names->symbol, values, m->pool);
                break;
            } else {
                add_to_env(env, names->car->symbol, values->car, m->pool);
            }
            names = names->cdr;
            values = values->cdr;
        }

        return extend_env(env, parent_env);
    }
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

static value* prim_mod(machine* m, value* args) {
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

static value* prim_pow(machine* m, value* args) {
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

    return pool_new_bool(
        m->pool,
        (value_is_true(args->car) ? 0 : 1));
}

static value* prim_null_q(machine* m, value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, arg == NULL);
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

static value* prim_error(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_STRING);

    value* error_message = args->car;
    value* error_args = args->cdr;

    static char buffer[16384];
    format_args(error_message, error_args, buffer);

    return pool_new_error(m->pool, buffer);
}

static value* prim_info(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_STRING);

    value* info_message = args->car;
    value* info_args = args->cdr;

    static char buffer[16384];
    format_args(info_message, info_args, buffer);

    return pool_new_info(m->pool, buffer);
}

static value* prim_display(machine* m, value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_STRING);

    value* display_message = args->car;
    value* display_args = args->cdr;

    static char buffer[16384];
    format_args(display_message, display_args, buffer);
    printf("%s", buffer);

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
        m->pool, "collected %zu (%.2f%%) from %zu values",
        collected, percentage, values_before);
}

static void add_primitive(eval* e, value* env, char* name, builtin fn) {
    pool* p = e->machine->pool;
    add_to_env(env, name, pool_new_builtin(p, fn, name), p);
}

static value* make_global_environment(eval* e) {
    pool* p = e->machine->pool;
    value* env = pool_new_env(e->machine->pool);

    // constants
    add_to_env(env, "PI", pool_new_number(p, 3.14159265358979323846264338327950288), p);
    add_to_env(env, "E", pool_new_number(p, 2.71828182845904523536028747135266250), p);

    // structural
    add_primitive(e, env, "car", prim_car);
    add_primitive(e, env, "cdr", prim_cdr);
    add_primitive(e, env, "cons", prim_cons);

    // arithmetic
    add_primitive(e, env, "+", prim_add);
    add_primitive(e, env, "-", prim_sub);
    add_primitive(e, env, "*", prim_mul);
    add_primitive(e, env, "/", prim_div);
    add_primitive(e, env, "%", prim_mod);
    add_primitive(e, env, "^", prim_pow);
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
    add_primitive(e, env, "equal?", prim_equal_q);
    add_primitive(e, env, "eq?", prim_eq_q);

    // other
    add_primitive(e, env, "error", prim_error);
    add_primitive(e, env, "info", prim_info);
    add_primitive(e, env, "display", prim_display);
    add_primitive(e, env, "newline", prim_newline);
    add_primitive(e, env, "collect", prim_collect);

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
