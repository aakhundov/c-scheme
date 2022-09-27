#ifndef SYNTAX_HPP_
#define SYNTAX_HPP_

#include "error.hpp"
#include "value.hpp"

// exceptions

class syntax_error : public scheme_error {
   public:
    template <typename... Args>
    syntax_error(const char* format, Args&&... args)
        : scheme_error("syntax error", format, forward<Args>(args)...) {}
};

// helper functions

bool is_self_evaluating(const shared_ptr<value>& exp);
bool is_variable(const shared_ptr<value>& exp);
bool is_quoted(const shared_ptr<value>& exp);
bool is_assignment(const shared_ptr<value>& exp);
bool is_definition(const shared_ptr<value>& exp);
bool is_if(const shared_ptr<value>& exp);
bool is_lambda(const shared_ptr<value>& exp);
bool is_let(const shared_ptr<value>& exp);
bool is_begin(const shared_ptr<value>& exp);
bool is_cond(const shared_ptr<value>& exp);
bool is_and(const shared_ptr<value>& exp);
bool is_or(const shared_ptr<value>& exp);
bool is_eval(const shared_ptr<value>& exp);
bool is_apply(const shared_ptr<value>& exp);

void check_quoted(const shared_ptr<value>& exp);
shared_ptr<value> get_text_of_quotation(const shared_ptr<value>& quoted);

void check_assignment(const shared_ptr<value>& exp);
shared_ptr<value> get_assignment_variable(const shared_ptr<value>& assignment);
shared_ptr<value> get_assignment_value(const shared_ptr<value>& assignment);

void check_definition(const shared_ptr<value>& exp);
shared_ptr<value> get_definition_variable(const shared_ptr<value>& definition);
shared_ptr<value> get_definition_value(const shared_ptr<value>& definition);

void check_if(const shared_ptr<value>& exp);
shared_ptr<value> get_if_predicate(const shared_ptr<value>& if_);
shared_ptr<value> get_if_consequent(const shared_ptr<value>& if_);
shared_ptr<value> get_if_alternative(const shared_ptr<value>& if_);
shared_ptr<value> make_if(
    const shared_ptr<value>& predicate,
    const shared_ptr<value>& consequent,
    const shared_ptr<value>& alternative);

void check_lambda(const shared_ptr<value>& exp);
shared_ptr<value> get_lambda_parameters(const shared_ptr<value>& lambda);
shared_ptr<value> get_lambda_body(const shared_ptr<value>& lambda);
shared_ptr<value> make_lambda(
    const shared_ptr<value>& params,
    const shared_ptr<value>& body);

void check_let(const shared_ptr<value>& exp);
shared_ptr<value> transform_let(const shared_ptr<value>& let);

void check_begin(const shared_ptr<value>& exp);
shared_ptr<value> get_begin_actions(const shared_ptr<value>& begin);
shared_ptr<value> transform_sequence(const shared_ptr<value>& seq);

void check_cond(const shared_ptr<value>& exp);
shared_ptr<value> transform_cond(const shared_ptr<value>& cond);

void check_and(const shared_ptr<value>& exp);
shared_ptr<value> get_and_expressions(const shared_ptr<value>& and_);

void check_or(const shared_ptr<value>& exp);
shared_ptr<value> get_or_expressions(const shared_ptr<value>& or_);

void check_eval(const shared_ptr<value>& exp);
shared_ptr<value> get_eval_expression(const shared_ptr<value>& eval);

void check_apply(const shared_ptr<value>& exp);
shared_ptr<value> get_apply_operator(const shared_ptr<value>& apply);
shared_ptr<value> get_apply_arguments(const shared_ptr<value>& apply);
void check_apply_arguments(const shared_ptr<value>& args);

void check_application(const shared_ptr<value>& exp);

bool has_no_exps(const shared_ptr<value>& seq);
bool is_last_exp(const shared_ptr<value>& seq);
shared_ptr<value> get_first_exp(const shared_ptr<value>& seq);
shared_ptr<value> get_rest_exps(const shared_ptr<value>& seq);

shared_ptr<value> get_operator(const shared_ptr<value>& compound);
shared_ptr<value> get_operands(const shared_ptr<value>& compound);

bool has_no_operands(const shared_ptr<value>& operands);
bool is_last_operand(const shared_ptr<value>& operands);
shared_ptr<value> get_first_operand(const shared_ptr<value>& operands);
shared_ptr<value> get_rest_operands(const shared_ptr<value>& operands);

shared_ptr<value> make_empty_arglist();
shared_ptr<value> adjoin_arg(
    const shared_ptr<value>& arg,
    const shared_ptr<value>& arg_list);

#endif  // SYNTAX_HPP_
