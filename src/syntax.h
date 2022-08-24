#ifndef SYNTAX_H_
#define SYNTAX_H_

#include "pool.h"
#include "value.h"

int is_tagged_list(value* v, const char* tag);

value* is_self_evaluating(pool* p, value* exp);

value* is_variable(pool* p, value* exp);

value* is_quoted(pool* p, value* exp);
value* get_text_of_quotation(pool* p, value* exp);

value* is_assignment(pool* p, value* exp);
value* get_assignment_variable(pool* p, value* exp);
value* get_assignment_value(pool* p, value* exp);

value* is_definition(pool* p, value* exp);
value* get_definition_variable(pool* p, value* exp);
value* get_definition_value(pool* p, value* exp);

value* is_if(pool* p, value* exp);
value* get_if_predicate(pool* p, value* exp);
value* get_if_consequent(pool* p, value* exp);
value* get_if_alternative(pool* p, value* exp);

value* is_lambda(pool* p, value* exp);
value* get_lambda_parameters(pool* p, value* exp);
value* get_lambda_body(pool* p, value* exp);
value* make_lambda(pool* p, value* params, value* body);

value* is_begin(pool* p, value* exp);
value* get_begin_actions(pool* p, value* exp);

value* is_eval(pool* p, value* exp);
value* get_eval_expression(pool* p, value* exp);

value* is_application(pool* p, value* exp);
value* is_true(pool* p, value* exp);

value* is_last_exp(pool* p, value* seq);
value* get_first_exp(pool* p, value* seq);
value* get_rest_exps(pool* p, value* seq);

value* get_operator(pool* p, value* compound);
value* get_operands(pool* p, value* compound);
value* is_no_operands(pool* p, value* operands);
value* is_last_operand(pool* p, value* operands);
value* get_first_operand(pool* p, value* operands);
value* get_rest_operands(pool* p, value* operands);

value* make_empty_arglist(pool* p);
value* adjoin_arg(pool* p, value* arg, value* arg_list);

value* is_primitive_procedure(pool* p, value* proc);
value* is_compound_procedure(pool* p, value* proc);
value* get_procedure_parameters(pool* p, value* proc);
value* get_procedure_body(pool* p, value* proc);
value* get_procedure_environment(pool* p, value* proc);
value* make_compound_procedure(pool* p, value* params, value* body, value* env);

int format_args(value* message, value* args, char* buffer);

#endif  // SYNTAX_H_
