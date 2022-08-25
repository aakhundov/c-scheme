#include "syntax.h"

#include <stdio.h>
#include <string.h>

#include "pool.h"
#include "value.h"

#define MAX_ERROR_ARGS 5

#define MAKE_ERROR(p, text, tag, exp)                \
    {                                                \
        static char buffer[16384];                   \
        value_to_str(exp, buffer);                   \
        return pool_new_error(p, text, tag, buffer); \
    }

int is_tagged_list(value* v, const char* tag) {
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
         exp->type == VALUE_NUMBER ||   // 10
         exp->type == VALUE_STRING ||   // "abc"
         exp->type == VALUE_BOOL ||     // true or false
         exp->type == VALUE_BUILTIN));  // true or false
}

value* is_variable(pool* p, value* exp) {
    // x, y, etc.
    return pool_new_bool(
        p,
        (exp != NULL && exp->type == VALUE_SYMBOL));
}

value* is_quoted(pool* p, value* exp) {
    // (quote text)
    static const char* tag = "quote";
    if (is_tagged_list(exp, tag)) {
        if (exp->cdr == NULL) {
            MAKE_ERROR(p, "%s: no item in %s", tag, exp);
        } else if (exp->cdr->cdr != NULL) {
            MAKE_ERROR(p, "%s: more than one item in %s", tag, exp);
        }

        return pool_new_bool(p, 1);
    } else {
        return pool_new_bool(p, 0);
    }
}

value* get_text_of_quotation(pool* p, value* exp) {
    // x from (quote x)
    return exp->cdr->car;
}

value* is_assignment(pool* p, value* exp) {
    // (!set variable value)
    static const char* tag = "set!";
    if (is_tagged_list(exp, tag)) {
        if (exp->cdr == NULL) {
            MAKE_ERROR(p, "%s: no variable in %s", tag, exp);
        } else if (exp->cdr->car == NULL || exp->cdr->car->type != VALUE_SYMBOL) {
            MAKE_ERROR(p, "%s: variable is not a symbol in %s", tag, exp);
        } else if (exp->cdr->cdr == NULL) {
            MAKE_ERROR(p, "%s: no value in %s", tag, exp);
        } else if (exp->cdr->cdr->cdr != NULL) {
            MAKE_ERROR(p, "%s: more than two items in %s", tag, exp);
        }

        return pool_new_bool(p, 1);
    } else {
        return pool_new_bool(p, 0);
    }
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
    // (define variable value)
    // (define (function p1 p2 ...) body)
    static const char* tag = "define";
    if (is_tagged_list(exp, tag)) {
        if (exp->cdr == NULL) {
            MAKE_ERROR(p, "%s: no variable in %s", tag, exp);
        } else if (exp->cdr->car == NULL) {
            MAKE_ERROR(p, "%s: can't define () in %s", tag, exp);
        } else if (exp->cdr->car->type == VALUE_PAIR) {
            if (exp->cdr->cdr == NULL) {
                MAKE_ERROR(p, "%s: no body in %s", tag, exp);
            } else if (exp->cdr->car->car == NULL || exp->cdr->car->car->type != VALUE_SYMBOL) {
                MAKE_ERROR(
                    p, "%s: the function name is not a symbol in %s",
                    tag, exp);
            }
        } else if (exp->cdr->car->type == VALUE_SYMBOL) {
            if (exp->cdr->cdr == NULL) {
                MAKE_ERROR(p, "%s: no value in %s", tag, exp);
            } else if (exp->cdr->cdr->cdr != NULL) {
                MAKE_ERROR(
                    p, "%s: the value can't be more than one item in %s",
                    tag, exp);
            }
        } else {
            MAKE_ERROR(
                p, "%s: either variable or function must be defined in %s",
                tag, exp);
        }

        return pool_new_bool(p, 1);
    } else {
        return pool_new_bool(p, 0);
    }
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
    // (if pred cons)
    // (if pred cons alt)
    static const char* tag = "if";
    if (is_tagged_list(exp, tag)) {
        if (exp->cdr == NULL) {
            MAKE_ERROR(p, "%s: no predicate in %s", tag, exp);
        } else if (exp->cdr->cdr == NULL) {
            MAKE_ERROR(p, "%s: no consequent in %s", tag, exp);
        } else if (exp->cdr->cdr->cdr != NULL && exp->cdr->cdr->cdr->cdr != NULL) {
            MAKE_ERROR(p, "%s: too many items in %s", tag, exp);
        }

        return pool_new_bool(p, 1);
    } else {
        return pool_new_bool(p, 0);
    }
}

value* get_if_predicate(pool* p, value* exp) {
    // pred from (if pred cons alt) or (if pred cons)
    return exp->cdr->car;
}

value* get_if_consequent(pool* p, value* exp) {
    // cons from (if pred cons alt) or (if pred cons)
    return exp->cdr->cdr->car;
}

value* get_if_alternative(pool* p, value* exp) {
    if (exp->cdr->cdr->cdr != NULL) {
        // alt from (if pred cons alt)
        return exp->cdr->cdr->cdr->car;
    } else {
        // no consequent -> false
        // false from (if pred cons)
        return pool_new_bool(p, 0);
    }
}

value* make_if(pool* p, value* predicate, value* consequent, value* alternative) {
    // pred, cons, alt -> (if pred cons alt)
    return pool_new_pair(
        p,
        pool_new_symbol(p, "if"),
        pool_new_pair(
            p,
            predicate,
            pool_new_pair(
                p,
                consequent,
                pool_new_pair(
                    p,
                    alternative,
                    NULL))));
}

value* is_lambda(pool* p, value* exp) {
    // (lambda (p1 p2 ...) e1 e2 ...)
    static const char* tag = "lambda";
    if (is_tagged_list(exp, tag)) {
        if (exp->cdr == NULL) {
            MAKE_ERROR(p, "%s: no parameters in %s", tag, exp);
        } else if (exp->cdr->cdr == NULL) {
            MAKE_ERROR(p, "%s: no body in %s", tag, exp);
        } else {
            value* p1;
            value* p2;

            p1 = exp->cdr->car;
            while (p1 != NULL) {
                if (p1->type == VALUE_SYMBOL) {
                    break;
                }
                if (p1->type != VALUE_PAIR ||
                    p1->car == NULL ||
                    p1->car->type != VALUE_SYMBOL) {
                    MAKE_ERROR(
                        p, "%s: some parameters are not symbols in %s",
                        tag, exp);
                }
                p1 = p1->cdr;
            }

            p1 = exp->cdr->car;
            while (p1 != NULL) {
                if (p1->type == VALUE_SYMBOL) {
                    break;
                }
                p2 = p1->cdr;
                while (p2 != NULL) {
                    if (p2->type == VALUE_SYMBOL) {
                        if (strcmp(p1->car->symbol, p2->symbol) == 0) {
                            MAKE_ERROR(
                                p, "%s: duplicate parameter names in %s",
                                tag, exp);
                        }
                        break;
                    }
                    if (strcmp(p1->car->symbol, p2->car->symbol) == 0) {
                        MAKE_ERROR(
                            p, "%s: duplicate parameter names in %s",
                            tag, exp);
                    }
                    p2 = p2->cdr;
                }
                p1 = p1->cdr;
            }
        }

        return pool_new_bool(p, 1);
    } else {
        return pool_new_bool(p, 0);
    }
}

value* get_lambda_parameters(pool* p, value* exp) {
    // (p1 p2 ...) from (lambda (p1 p2 ...) e1 e2 ...)
    return exp->cdr->car;
}

value* get_lambda_body(pool* p, value* exp) {
    // (e1 e2 ...) from (lambda (p1 p2 ...) e1 e2 ...)
    return exp->cdr->cdr;
}

value* make_lambda(pool* p, value* params, value* body) {
    // (p1 p2 ...), (e1 e2 ...) ->
    // (lambda (p1 p2 ...) e1 e2 ...)
    return pool_new_pair(
        p,
        pool_new_symbol(p, "lambda"),
        pool_new_pair(p, params, body));
}

value* is_begin(pool* p, value* exp) {
    // (begin a1 a2 ...)
    static const char* tag = "begin";
    if (is_tagged_list(exp, tag)) {
        if (exp->cdr == NULL) {
            MAKE_ERROR(p, "%s: no items in %s", tag, exp);
        }

        return pool_new_bool(p, 1);
    } else {
        return pool_new_bool(p, 0);
    }
}

value* get_begin_actions(pool* p, value* exp) {
    // (a1 a2 ...) from (begin a1 a2 ...)
    return exp->cdr;
}

value* transform_sequence(pool* p, value* seq) {
    if (seq == NULL) {
        // () -> ()
        return NULL;
    } else if (seq->cdr == NULL) {
        // (a) -> a
        return seq->car;
    } else {
        // (a1 a2 ...) -> (begin a1 a2 ...)
        return pool_new_pair(
            p,
            pool_new_symbol(p, "begin"),
            seq);
    }
}

value* is_cond(pool* p, value* exp) {
    // (cond (p1 a11 a12 ...) (p2 a21 a22 ...) ...)
    // (cond (p1 a11 a12 ...) (p2 a21 a22 ...) ... (else ae1 ae2 ...))
    static const char* tag = "cond";
    if (is_tagged_list(exp, tag)) {
        if (exp->cdr == NULL) {
            MAKE_ERROR(p, "%s: no clauses in %s", tag, exp);
        } else {
            value* running = exp->cdr;
            while (running != NULL) {
                value* clause = running->car;
                if (clause == NULL) {
                    MAKE_ERROR(p, "%s: empty clause in %s", tag, exp);
                } else {
                    value* item = clause;
                    while (item != NULL) {
                        if (item->type != VALUE_PAIR) {
                            MAKE_ERROR(p, "%s: non-list clause in %s", tag, exp);
                        }
                        item = item->cdr;
                    }

                    value* predicate = clause->car;
                    value* actions = clause->cdr;
                    if (predicate != NULL &&
                        predicate->type == VALUE_SYMBOL &&
                        strcmp(predicate->symbol, "else") == 0 &&
                        running->cdr != NULL) {
                        MAKE_ERROR(p, "%s: else clause must be the last in %s", tag, exp);
                    } else if (actions == NULL) {
                        MAKE_ERROR(p, "%s: actionless clause in %s", tag, exp);
                    }
                }
                running = running->cdr;
            }
        }

        return pool_new_bool(p, 1);
    } else {
        return pool_new_bool(p, 0);
    }
}

static value* transform_cond_rec(pool* p, value* clauses) {
    // (cond (p1 a1) (p2 a21 a22) (else ae) ->
    // (if p1 a1 (if p2 (begin a21 a22) ae))
    if (clauses == NULL) {
        // if no terminal clause -> false
        return pool_new_bool(p, 0);
    } else {
        value* first = clauses->car;
        value* rest = clauses->cdr;
        value* predicate = first->car;
        value* actions = first->cdr;

        if (predicate != NULL &&
            predicate->type == VALUE_SYMBOL &&
            strcmp(predicate->symbol, "else") == 0) {
            // terminal clause: actions
            return transform_sequence(p, actions);
        } else {
            // non-terminal clause:
            // (if predicate actions <transform rest>)
            return make_if(
                p,
                predicate,
                transform_sequence(p, actions),
                transform_cond_rec(p, rest));
        }
    }
}

value* transform_cond(pool* p, value* exp) {
    // transform clauses recursively
    return transform_cond_rec(p, exp->cdr);
}

value* is_let(pool* p, value* exp) {
    static const char* tag = "let";
    if (is_tagged_list(exp, tag)) {
        if (exp->cdr == NULL) {
            MAKE_ERROR(p, "%s: no variables in %s", tag, exp);
        } else if (exp->cdr->cdr == NULL) {
            MAKE_ERROR(p, "%s: no body in %s", tag, exp);
        } else {
            value* running = exp->cdr->car;
            while (running != NULL) {
                if (running->type != VALUE_PAIR) {
                    MAKE_ERROR(p, "%s: non-list variables in %s", tag, exp);
                }

                value* pair = running->car;
                if (pair == NULL) {
                    MAKE_ERROR(p, "%s: no variable name in %s", tag, exp);
                } else if (pair->type != VALUE_PAIR) {
                    MAKE_ERROR(p, "%s: non-list variable pair in %s", tag, exp);
                } else if (pair->car == NULL || pair->car->type != VALUE_SYMBOL) {
                    MAKE_ERROR(p, "%s: variable name must be a symbol in %s", tag, exp);
                } else if (pair->cdr == NULL) {
                    MAKE_ERROR(p, "%s: no variable value in %s", tag, exp);
                } else if (pair->cdr->type != VALUE_PAIR) {
                    MAKE_ERROR(p, "%s: non-list variable pair in %s", tag, exp);
                } else if (pair->cdr->cdr != NULL) {
                    MAKE_ERROR(p, "%s: too many items in a variable pair in %s", tag, exp);
                }

                running = running->cdr;
            }
        }

        return pool_new_bool(p, 1);
    } else {
        return pool_new_bool(p, 0);
    }
}

value* transform_let(pool* p, value* exp) {
    value* params = NULL;
    value* args = NULL;
    value* body = exp->cdr->cdr;

    value* params_tail = NULL;
    value* args_tail = NULL;
    value* running = exp->cdr->car;
    while (running != NULL) {
        value* pair = running->car;
        value* name = pair->car;
        value* val = pair->cdr->car;

        value* next_param = pool_new_pair(p, name, NULL);
        value* next_arg = pool_new_pair(p, val, NULL);
        if (params == NULL) {
            params = next_param;
            args = next_arg;
        } else {
            params_tail->cdr = next_param;
            args_tail->cdr = next_arg;
        }
        params_tail = next_param;
        args_tail = next_arg;

        running = running->cdr;
    }

    return pool_new_pair(
        p,
        make_lambda(
            p,
            params,
            body),
        args);
}

value* is_eval(pool* p, value* exp) {
    // (eval exp)
    static const char* tag = "eval";
    if (is_tagged_list(exp, tag)) {
        if (exp->cdr == NULL) {
            MAKE_ERROR(p, "%s: no expression in %s", tag, exp);
        } else if (exp->cdr->cdr != NULL) {
            MAKE_ERROR(p, "%s: too many items in %s", tag, exp);
        }

        return pool_new_bool(p, 1);
    } else {
        return pool_new_bool(p, 0);
    }
}

value* get_eval_expression(pool* p, value* exp) {
    // exp from (eval exp)
    return exp->cdr->car;
}

value* is_apply(pool* p, value* exp) {
    // (apply f (a1 a2 ...))
    static const char* tag = "apply";
    if (is_tagged_list(exp, tag)) {
        if (exp->cdr == NULL) {
            MAKE_ERROR(p, "%s: no operator in %s", tag, exp);
        } else if (exp->cdr->cdr == NULL) {
            MAKE_ERROR(p, "%s: no arguments in %s", tag, exp);
        } else if (exp->cdr->cdr->cdr != NULL) {
            MAKE_ERROR(p, "%s: too many items in in %s", tag, exp);
        }

        return pool_new_bool(p, 1);
    } else {
        return pool_new_bool(p, 0);
    }
}

value* get_apply_operator(pool* p, value* exp) {
    // f from (apply f (a1 a2 ...))
    return exp->cdr->car;
}

value* get_apply_arguments(pool* p, value* exp) {
    // (a1 a2 ...) from (apply f (a1 a2 ...))
    return exp->cdr->cdr->car;
}

value* verify_apply_arguments(pool* p, value* args) {
    // make sure (a1 a2 ...) is a NULL-terminated list
    value* running = args;
    while (running != NULL) {
        if (running->type != VALUE_PAIR) {
            MAKE_ERROR(p, "%s: can't apply to %s", "apply", args);
        }
        running = running->cdr;
    }

    return pool_new_bool(p, 1);
}

value* is_application(pool* p, value* exp) {
    // (f ...) with any f
    if (exp != NULL && exp->type == VALUE_PAIR) {
        value* running = exp;
        while (running != NULL) {
            if (running->type != VALUE_PAIR) {
                MAKE_ERROR(p, "%s: can't apply to %s", "apply", exp->cdr);
            }
            running = running->cdr;
        }

        return pool_new_bool(p, 1);
    } else {
        return pool_new_bool(p, 0);
    }
}

value* is_true(pool* p, value* exp) {
    // exp != false
    if (exp != NULL && exp->type == VALUE_BOOL && !value_is_true(exp)) {
        return pool_new_bool(p, 0);
    } else {
        return pool_new_bool(p, 1);
    }
}

value* is_false(pool* p, value* exp) {
    // exp == false
    if (exp != NULL && exp->type == VALUE_BOOL && !value_is_true(exp)) {
        return pool_new_bool(p, 1);
    } else {
        return pool_new_bool(p, 0);
    }
}

value* is_last_exp(pool* p, value* seq) {
    // (e)
    return pool_new_bool(p, seq->cdr == NULL);
}

value* get_first_exp(pool* p, value* seq) {
    // e1 from (e1 e2 ...)
    return seq->car;
}

value* get_rest_exps(pool* p, value* seq) {
    // (e2 ...) from (e1 e2 ...)
    return seq->cdr;
}

value* get_operator(pool* p, value* compound) {
    // op from (op p1 p2 ...)
    return compound->car;
}

value* get_operands(pool* p, value* compound) {
    // (p1 p2 ...) from (op p1 p2 ...)
    return compound->cdr;
}

value* is_no_operands(pool* p, value* operands) {
    // ()
    return pool_new_bool(p, operands == NULL);
}

value* is_last_operand(pool* p, value* operands) {
    // (p)
    return pool_new_bool(p, operands->cdr == NULL);
}

value* get_first_operand(pool* p, value* operands) {
    // p1 from (p1 p2 ...)
    return operands->car;
}

value* get_rest_operands(pool* p, value* operands) {
    // (p2 ...) from (p1 p2 ...)
    return operands->cdr;
}

value* make_empty_arglist(pool* p) {
    // ()
    return NULL;
}

value* adjoin_arg(pool* p, value* arg, value* arg_list) {
    // p, () -> (p)
    // p, (p1 p2 ... pn) -> (p1 p2 ... pn p)
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

int format_args(value* message, value* args, char* buffer) {
    static char bufs[MAX_ERROR_ARGS][16384];
    char* fmt = message->symbol;

    // convert available args to string buffers,
    // up to the MAX_ERROR_ARGS args / buffers
    size_t i = 0;
    while (args != NULL) {
        if (i == MAX_ERROR_ARGS) {
            break;
        }
        value_to_str(args->car, bufs[i]);
        args = args->cdr;
        i += 1;
    }
    if (i < MAX_ERROR_ARGS) {
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
                        return sprintf(buffer, fmt, bufs[0], bufs[1], bufs[2], bufs[3], bufs[4]);
                    } else {
                        return sprintf(buffer, fmt, bufs[0], bufs[1], bufs[2], bufs[3]);
                    }
                } else {
                    return sprintf(buffer, fmt, bufs[0], bufs[1], bufs[2]);
                }
            } else {
                return sprintf(buffer, fmt, bufs[0], bufs[1]);
            }
        } else {
            return sprintf(buffer, fmt, bufs[0]);
        }
    } else {
        return sprintf(buffer, "%s", fmt);
    }
}
