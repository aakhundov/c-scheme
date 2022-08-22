#include "syntax.h"

#include <string.h>

#include "pool.h"
#include "value.h"

int is_tagged_list(value* v, char* tag) {
    // (tag ...)
    return (
        v != NULL &&
        v->type == VALUE_PAIR &&
        v->car != NULL &&
        v->car->type == VALUE_SYMBOL &&
        strcmp(v->car->symbol, tag) == 0);
}

value* is_self_evaluating(pool* p, value* exp) {
    return pool_new_bool(
        p,
        (exp == NULL ||
         exp->type == VALUE_NUMBER ||  // 10
         exp->type == VALUE_STRING ||  // "abc"
         exp->type == VALUE_BOOL));    // true or false
}

value* is_variable(pool* p, value* exp) {
    // x, y, etc.
    return pool_new_bool(
        p,
        (exp != NULL && exp->type == VALUE_SYMBOL));
}

value* is_quoted(pool* p, value* exp) {
    // (quote ...)
    return pool_new_bool(
        p,
        is_tagged_list(exp, "quote"));
}

value* get_text_of_quotation(pool* p, value* exp) {
    // x from (quote x)
    return exp->cdr->car;
}

value* is_assignment(pool* p, value* exp) {
    // (!set ...)
    return pool_new_bool(
        p,
        is_tagged_list(exp, "set!"));
}

value* get_assignment_variable(pool* p, value* exp) {
    // x from (set! x 10)
    return exp->cdr->car;
}

value* get_assignment_value(pool* p, value* exp) {
    // 10 from (set! x 10)
    return exp->cdr->cdr->car;
}

value* is_definition(pool* p, value* exp) {
    // (define ...)
    return pool_new_bool(
        p,
        is_tagged_list(exp, "define"));
}

value* get_definition_variable(pool* p, value* exp) {
    if (exp->cdr->car->type == VALUE_SYMBOL) {
        // f from (define f 10)"
        return exp->cdr->car;
    } else {
        // f from (define (f x y) (+ x y) x)
        return exp->cdr->car->car;
    }
}

value* get_definition_value(pool* p, value* exp) {
    if (exp->cdr->car->type == VALUE_SYMBOL) {
        // 10 from (define f 10)
        return exp->cdr->cdr->car;
    } else {
        // (lambda (x y) (+ x y) x) from (define (f x y) (+ x y) x)
        return make_lambda(p, exp->cdr->car->cdr, exp->cdr->cdr);
    }
}

value* is_if(pool* p, value* exp) {
    // (if ...)
    return pool_new_bool(
        p,
        is_tagged_list(exp, "if"));
}

value* get_if_predicate(pool* p, value* exp) {
    // x from (if x y z) or (if x y)
    return exp->cdr->car;
}

value* get_if_consequent(pool* p, value* exp) {
    // y from (if x y z) or (if x y)
    return exp->cdr->cdr->car;
}

value* get_if_alternative(pool* p, value* exp) {
    if (exp->cdr->cdr->cdr != NULL) {
        // z from (if x y z) or
        return exp->cdr->cdr->cdr->car;
    } else {
        // false from (if x y)
        return pool_new_bool(p, 0);
    }
}

value* is_lambda(pool* p, value* exp) {
    // (lambda ...)
    return pool_new_bool(
        p,
        is_tagged_list(exp, "lambda"));
}

value* get_lambda_parameters(pool* p, value* exp) {
    // (x y) from (lambda (x y) (+ x y) x)
    return exp->cdr->car;
}

value* get_lambda_body(pool* p, value* exp) {
    // ((+ x y) x) from (lambda (x y) (+ x y) x)
    return exp->cdr->cdr;
}

value* make_lambda(pool* p, value* params, value* body) {
    // (lambda (x y) (+ x y) x) from
    // params = (x y), body = ((+ x y) x)
    return pool_new_pair(
        p,
        pool_new_symbol(p, "lambda"),
        pool_new_pair(p, params, body));
}

value* is_begin(pool* p, value* exp) {
    // (begin ...)
    return pool_new_bool(
        p,
        is_tagged_list(exp, "begin"));
}

value* get_begin_actions(pool* p, value* exp) {
    // (x y z) from (begin x y z)
    return exp->cdr;
}

value* is_application(pool* p, value* exp) {
    // (f ...) with any f
    return pool_new_bool(
        p,
        (exp != NULL && exp->type == VALUE_PAIR));
}

value* is_true(pool* p, value* exp) {
    // anything resolving to the true bool is true
    return pool_new_bool(p, value_is_true(exp));
}

value* is_last_exp(pool* p, value* seq) {
    // (x)
    return pool_new_bool(p, seq->cdr == NULL);
}

value* get_first_exp(pool* p, value* seq) {
    // x from (x y z)
    return seq->car;
}

value* get_rest_exps(pool* p, value* seq) {
    // (y z) from (x y z)
    return seq->cdr;
}

value* get_operator(pool* p, value* compound) {
    // f from (f x y z)
    return compound->car;
}

value* get_operands(pool* p, value* compound) {
    // (x y z) from (f x y z)
    return compound->cdr;
}

value* is_no_operands(pool* p, value* operands) {
    // ()
    return pool_new_bool(p, operands == NULL);
}

value* is_last_operand(pool* p, value* operands) {
    // (x)
    return pool_new_bool(p, operands->cdr == NULL);
}

value* get_first_operand(pool* p, value* operands) {
    // x from (x y z)
    return operands->car;
}

value* get_rest_operands(pool* p, value* operands) {
    // (y z) from (x y z)
    return operands->cdr;
}

value* make_empty_arglist(pool* p) {
    // ()
    return NULL;
}

value* adjoin_arg(pool* p, value* arg, value* arg_list) {
    // (x y z) from arg_list = (x y) and new_arg = z
    value* new_arg = pool_new_pair(p, arg, NULL);

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

value* is_primitive_procedure(pool* p, value* proc) {
    return pool_new_bool(
        p,
        (proc != NULL && proc->type == VALUE_BUILTIN));
}

value* is_compound_procedure(pool* p, value* proc) {
    return pool_new_bool(
        p,
        (proc != NULL && proc->type == VALUE_LAMBDA));
}

value* get_procedure_parameters(pool* p, value* proc) {
    return proc->car->car;
}

value* get_procedure_body(pool* p, value* proc) {
    return proc->car->cdr;
}

value* get_procedure_environment(pool* p, value* proc) {
    return proc->cdr;
}

value* make_compound_procedure(pool* p, value* params, value* body, value* env) {
    return pool_new_lambda(
        p,
        pool_new_pair(
            p,
            params,
            body),
        env);
}

value* make_error(pool* p, value* message, value* args) {
    static const size_t max_args = 5;
    static char bufs[max_args][16384];
    char* fmt = message->symbol;

    // convert available args to string buffers,
    // up to the max_args args / buffers
    size_t i = 0;
    while (args != NULL) {
        if (i == max_args) {
            break;
        }
        value_to_str(args->car, bufs[i]);
        args = args->cdr;
        i += 1;
    }
    if (i < max_args) {
        // indicate the end
        bufs[i][0] = '\0';
    }

    // ugly, but seems like the only way to
    // "dynamically" invoke a function with ... args
    // (without a low-level messing with the stack)
    if (bufs[0][0] != '\0') {
        if (bufs[1][0] != '\0') {
            if (bufs[2][0] != '\0') {
                if (bufs[3][0] != '\0') {
                    if (bufs[4][0] != '\0') {
                        return pool_new_error(p, fmt, bufs[0], bufs[1], bufs[2], bufs[3], bufs[4]);
                    } else {
                        return pool_new_error(p, fmt, bufs[0], bufs[1], bufs[2], bufs[3]);
                    }
                } else {
                    return pool_new_error(p, fmt, bufs[0], bufs[1], bufs[2]);
                }
            } else {
                return pool_new_error(p, fmt, bufs[0], bufs[1]);
            }
        } else {
            return pool_new_error(p, fmt, bufs[0]);
        }
    } else {
        return pool_new_error(p, fmt);
    }
}
