#ifndef SYNTAX_H_
#define SYNTAX_H_

#include "pool.h"
#include "value.h"

int is_self_evaluating(value* exp);
int is_variable(value* exp);
int is_quoted(value* exp);
int is_assignment(value* exp);
int is_definition(value* exp);
int is_if(value* exp);
int is_lambda(value* exp);
int is_let(value* exp);
int is_begin(value* exp);
int is_cond(value* exp);
int is_and(value* exp);
int is_or(value* exp);
int is_eval(value* exp);
int is_apply(value* exp);

int starts_with_symbol(value* exp);

value* check_quoted(pool* p, value* exp);
value* get_text_of_quotation(pool* p, value* exp);

value* check_assignment(pool* p, value* exp);
value* get_assignment_variable(pool* p, value* exp);
value* get_assignment_value(pool* p, value* exp);

value* check_definition(pool* p, value* exp);
value* get_definition_variable(pool* p, value* exp);
value* get_definition_value(pool* p, value* exp);

value* check_if(pool* p, value* exp);
value* get_if_predicate(pool* p, value* exp);
value* get_if_consequent(pool* p, value* exp);
value* get_if_alternative(pool* p, value* exp);
value* make_if(pool* p, value* predicate, value* consequent, value* alternative);

value* check_lambda(pool* p, value* exp);
value* get_lambda_parameters(pool* p, value* exp);
value* get_lambda_body(pool* p, value* exp);
value* make_lambda(pool* p, value* params, value* body);

value* check_let(pool* p, value* exp);
value* transform_let(pool* p, value* exp);

value* check_begin(pool* p, value* exp);
value* get_begin_actions(pool* p, value* exp);
value* transform_sequence(pool* p, value* seq);

value* check_cond(pool* p, value* exp);
value* transform_cond(pool* p, value* exp);

value* check_and(pool* p, value* exp);
value* get_and_expressions(pool* p, value* exp);

value* check_or(pool* p, value* exp);
value* get_or_expressions(pool* p, value* exp);

value* check_eval(pool* p, value* exp);
value* get_eval_expression(pool* p, value* exp);

value* check_apply(pool* p, value* exp);
value* get_apply_operator(pool* p, value* exp);
value* get_apply_arguments(pool* p, value* exp);
value* check_apply_arguments(pool* p, value* args);

value* check_application(pool* p, value* exp);

int is_true(pool* p, value* exp);
int is_false(pool* p, value* exp);

int has_no_exps(pool* p, value* seq);
int is_last_exp(pool* p, value* seq);
value* get_first_exp(pool* p, value* seq);
value* get_rest_exps(pool* p, value* seq);

value* get_operator(pool* p, value* compound);
value* get_operands(pool* p, value* compound);
int has_no_operands(pool* p, value* operands);
int is_last_operand(pool* p, value* operands);
value* get_first_operand(pool* p, value* operands);
value* get_rest_operands(pool* p, value* operands);

value* make_empty_arglist(pool* p);
value* adjoin_arg(pool* p, value* arg, value* arg_list);

int is_primitive_procedure(pool* p, value* proc);
int is_compound_procedure(pool* p, value* proc);
value* get_procedure_parameters(pool* p, value* proc);
value* get_procedure_body(pool* p, value* proc);
value* get_procedure_environment(pool* p, value* proc);
value* make_compound_procedure(pool* p, value* params, value* body, value* env);

int format_args(value* message, value* args, char* buffer);

#endif  // SYNTAX_H_
