#ifndef SYNTAX_H_
#define SYNTAX_H_

#include "pool.h"
#include "value.h"

int is_self_evaluating(const value* exp);
int is_variable(const value* exp);
int is_quoted(const value* exp);
int is_assignment(const value* exp);
int is_definition(const value* exp);
int is_if(const value* exp);
int is_lambda(const value* exp);
int is_let(const value* exp);
int is_begin(const value* exp);
int is_cond(const value* exp);
int is_and(const value* exp);
int is_or(const value* exp);
int is_eval(const value* exp);
int is_apply(const value* exp);

int starts_with_symbol(const value* exp);

value* check_quoted(pool* p, value* exp);
value* get_text_of_quotation(pool* p, const value* exp);

value* check_assignment(pool* p, value* exp);
value* get_assignment_variable(pool* p, const value* exp);
value* get_assignment_value(pool* p, const value* exp);

value* check_definition(pool* p, value* exp);
value* get_definition_variable(pool* p, const value* exp);
value* get_definition_value(pool* p, const value* exp);

value* check_if(pool* p, value* exp);
value* get_if_predicate(pool* p, const value* exp);
value* get_if_consequent(pool* p, const value* exp);
value* get_if_alternative(pool* p, const value* exp);
value* make_if(pool* p, value* predicate, value* consequent, value* alternative);

value* check_lambda(pool* p, value* exp);
value* get_lambda_parameters(pool* p, const value* exp);
value* get_lambda_body(pool* p, const value* exp);
value* make_lambda(pool* p, value* params, value* body);

value* check_let(pool* p, value* exp);
value* transform_let(pool* p, const value* exp);

value* check_begin(pool* p, value* exp);
value* get_begin_actions(pool* p, const value* exp);
value* transform_sequence(pool* p, value* seq);

value* check_cond(pool* p, value* exp);
value* transform_cond(pool* p, const value* exp);

value* check_and(pool* p, value* exp);
value* get_and_expressions(pool* p, const value* exp);

value* check_or(pool* p, value* exp);
value* get_or_expressions(pool* p, const value* exp);

value* check_eval(pool* p, value* exp);
value* get_eval_expression(pool* p, const value* exp);

value* check_apply(pool* p, value* exp);
value* get_apply_operator(pool* p, const value* exp);
value* get_apply_arguments(pool* p, const value* exp);
value* check_apply_arguments(pool* p, value* args);

value* check_application(pool* p, value* exp);

int is_true(pool* p, const value* exp);
int is_false(pool* p, const value* exp);

int has_no_exps(pool* p, const value* seq);
int is_last_exp(pool* p, const value* seq);
value* get_first_exp(pool* p, const value* seq);
value* get_rest_exps(pool* p, const value* seq);

value* get_operator(pool* p, const value* compound);
value* get_operands(pool* p, const value* compound);
int has_no_operands(pool* p, const value* operands);
int is_last_operand(pool* p, const value* operands);
value* get_first_operand(pool* p, const value* operands);
value* get_rest_operands(pool* p, const value* operands);

value* make_empty_arglist(pool* p);
value* adjoin_arg(pool* p, value* arg, value* arg_list);

int is_primitive_procedure(pool* p, const value* proc);
int is_compound_procedure(pool* p, const value* proc);
int is_compiled_procedure(pool* p, const value* proc);

value* get_compound_parameters(pool* p, const value* proc);
value* get_compound_body(pool* p, const value* proc);
value* get_compound_environment(pool* p, const value* proc);
value* make_compound_procedure(pool* p, value* params, value* body, value* env);

value* get_compiled_entry(pool* p, const value* proc);
value* get_compiled_environment(pool* p, const value* proc);
value* make_compiled_procedure(pool* p, value* entry, value* env);

int format_args(const value* message, const value* args, char* buffer);

#endif  // SYNTAX_H_
