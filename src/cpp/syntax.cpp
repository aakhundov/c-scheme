#include "syntax.hpp"

#include <unordered_set>

#include "value.hpp"

using std::string;
using std::unordered_set;

namespace {

const value_pair* to_list(const shared_ptr<value>& v) {
    if (v->type() == value_t::pair) {
        auto pair = to_ptr<value_pair>(v);
        return (pair->is_list() ? pair : nullptr);
    } else {
        return nullptr;
    }
}

const value_pair* to_list(const value& v) {
    if (v.type() == value_t::pair) {
        auto pair = reinterpret_cast<const value_pair*>(&v);
        return (pair->is_list() ? pair : nullptr);
    } else {
        return nullptr;
    }
}

bool is_tagged_pair(const shared_ptr<value>& v, const string& tag) {
    if (v->type() == value_t::pair) {
        auto car = to_ptr<value_pair>(v)->car();
        return (car->type() == value_t::symbol &&
                to_ptr<value_symbol>(car)->symbol() == tag);
    } else {
        return false;
    }
}

bool is_else_clause(const value_pair* clause) {
    return (clause->car()->type() == value_t::symbol &&
            to_ptr<value_symbol>(clause->car())->symbol() == "else");
}

shared_ptr<value> transform_cond_rec(const value_pair* clauses) {
    if (clauses == nilptr) {
        return false_;  // false if no else clause
    }
    auto first = clauses->pcar();
    if (is_else_clause(first)) {
        return transform_sequence(first->cdr());  // actions
    } else {
        return make_if(
            first->car(),                        // predicate
            transform_sequence(first->cdr()),    // actions
            transform_cond_rec(clauses->pcdr())  // rest
        );
    }
}

}  // namespace

bool is_self_evaluating(const shared_ptr<value>& v) {
    static unordered_set<value_t> self_evaluating_types = {
        value_t::nil,
        value_t::number,
        value_t::string,
        value_t::bool_,
        value_t::primitive_op,
    };

    return self_evaluating_types.count(v->type());
}

bool is_variable(const shared_ptr<value>& v) {
    return v->type() == value_t::symbol;
}

bool is_quoted(const shared_ptr<value>& v) {
    return is_tagged_pair(v, "quote");
}

bool is_assignment(const shared_ptr<value>& v) {
    return is_tagged_pair(v, "set!");
}

bool is_definition(const shared_ptr<value>& v) {
    return is_tagged_pair(v, "define");
}

bool is_if(const shared_ptr<value>& v) {
    return is_tagged_pair(v, "if");
}

bool is_lambda(const shared_ptr<value>& v) {
    return is_tagged_pair(v, "lambda");
}

bool is_let(const shared_ptr<value>& v) {
    return is_tagged_pair(v, "let");
}

bool is_begin(const shared_ptr<value>& v) {
    return is_tagged_pair(v, "begin");
}

bool is_cond(const shared_ptr<value>& v) {
    return is_tagged_pair(v, "cond");
}

bool is_and(const shared_ptr<value>& v) {
    return is_tagged_pair(v, "and");
}

bool is_or(const shared_ptr<value>& v) {
    return is_tagged_pair(v, "or");
}

bool is_eval(const shared_ptr<value>& v) {
    return is_tagged_pair(v, "eval");
}

bool is_apply(const shared_ptr<value>& v) {
    return is_tagged_pair(v, "apply");
}

void check_quoted(const shared_ptr<value>& exp) {
    // (quote x)
    auto quoted = to_list(exp);
    if (!quoted) {
        throw syntax_error("quote: non-list structure in %s", exp->str().c_str());
    } else if (quoted->cdr() == nil) {
        throw syntax_error("quote: no expression in %s", exp->str().c_str());
    } else if (quoted->pcdr()->cdr() != nil) {
        throw syntax_error("quote: more than one item in %s", exp->str().c_str());
    }
}

shared_ptr<value> get_text_of_quotation(const shared_ptr<value>& quoted) {
    // x from (quote x)
    return to_ptr<value_pair>(quoted)->pcdr()->car();
}

void check_assignment(const shared_ptr<value>& exp) {
    // (!set variable value)
    auto assignment = to_list(exp);
    if (!assignment) {
        throw syntax_error("set!: non-list structure in %s", exp->str().c_str());
    } else if (assignment->cdr() == nil) {
        throw syntax_error("set!: no variable in %s", exp->str().c_str());
    } else if (assignment->pcdr()->car()->type() != value_t::symbol) {
        throw syntax_error("set!: variable is not a symbol in %s", exp->str().c_str());
    } else if (assignment->pcdr()->cdr() == nil) {
        throw syntax_error("set!: no value in %s", exp->str().c_str());
    } else if (assignment->pcdr()->pcdr()->cdr() != nil) {
        throw syntax_error("set!: more than two items in %s", exp->str().c_str());
    }
}

shared_ptr<value> get_assignment_variable(const shared_ptr<value>& assignment) {
    // x from (set! x 10)
    return to_ptr<value_pair>(assignment)->pcdr()->car();
}

shared_ptr<value> get_assignment_value(const shared_ptr<value>& assignment) {
    // 10 from (set! x 10)
    return to_ptr<value_pair>(assignment)->pcdr()->pcdr()->car();
}

void check_definition(const shared_ptr<value>& exp) {
    // (define x 10)
    // (define (f x y) (+ x y) x)
    auto definition = to_list(exp);
    if (!definition) {
        throw syntax_error("define: non-list structure in %s", exp->str().c_str());
    } else if (definition->cdr() == nil) {
        throw syntax_error("define: no variable in %s", exp->str().c_str());
    } else if (definition->pcdr()->car()->type() == value_t::pair) {
        if (definition->pcdr()->cdr() == nil) {
            throw syntax_error("define: no body in %s", exp->str().c_str());
        } else if (definition->pcdr()->pcar()->car()->type() != value_t::symbol) {
            throw syntax_error(
                "define: the function name is not a symbol in %s",
                exp->str().c_str());
        }
    } else if (definition->pcdr()->car()->type() == value_t::symbol) {
        if (definition->pcdr()->cdr() == nil) {
            throw syntax_error("define: no value in %s", exp->str().c_str());
        } else if (definition->pcdr()->pcdr()->cdr() != nil) {
            throw syntax_error(
                "define: the value can't be more than one item in %s",
                exp->str().c_str());
        }
    } else {
        throw syntax_error(
            "define: either variable or function must be defined in %s",
            exp->str().c_str());
    }
}

shared_ptr<value> get_definition_variable(const shared_ptr<value>& definition) {
    auto def_list = to_ptr<value_pair>(definition);
    if (def_list->pcdr()->car()->type() == value_t::symbol) {
        // x from (define x 10)
        return def_list->pcdr()->car();
    } else {
        // f from (define (f x y) (+ x y) x)
        return def_list->pcdr()->pcar()->car();
    }
}

shared_ptr<value> get_definition_value(const shared_ptr<value>& definition) {
    auto def_list = to_ptr<value_pair>(definition);
    if (def_list->pcdr()->car()->type() == value_t::symbol) {
        // 10 from (define x 10)
        return def_list->pcdr()->pcdr()->car();
    } else {
        // (lambda (x y) (+ x y) x) from (define (f x y) (+ x y) x)
        return make_lambda(
            def_list->pcdr()->pcar()->cdr(),  // params
            def_list->pcdr()->cdr());         // body
    }
}

void check_if(const shared_ptr<value>& exp) {
    // (if x 1)
    // (if x 1 2)
    auto if_ = to_list(exp);
    if (!if_) {
        throw syntax_error("if: non-list structure in %s", exp->str().c_str());
    } else if (if_->cdr() == nil) {
        throw syntax_error("if: no predicate in %s", exp->str().c_str());
    } else if (if_->pcdr()->cdr() == nil) {
        throw syntax_error("if: no consequent in %s", exp->str().c_str());
    } else if (if_->pcdr()->pcdr()->cdr() != nil &&
               if_->pcdr()->pcdr()->pcdr()->cdr() != nil) {
        throw syntax_error("if: too many items in %s", exp->str().c_str());
    }
}

shared_ptr<value> get_if_predicate(const shared_ptr<value>& if_) {
    // x from (if x 1 2) or (if x 1)
    return to_ptr<value_pair>(if_)->pcdr()->car();
}

shared_ptr<value> get_if_consequent(const shared_ptr<value>& if_) {
    // 1 from (if x 1 2) or (if x 1)
    return to_ptr<value_pair>(if_)->pcdr()->pcdr()->car();
}

shared_ptr<value> get_if_alternative(const shared_ptr<value>& if_) {
    auto if_list = to_ptr<value_pair>(if_);
    if (if_list->pcdr()->pcdr()->cdr() != nil) {
        // 2 from (if x 1 2)
        return if_list->pcdr()->pcdr()->pcdr()->car();
    } else {
        // no consequent -> false
        // false from (if x 1)
        return false_;
    }
}

shared_ptr<value> make_if(
    const shared_ptr<value>& predicate,
    const shared_ptr<value>& consequent,
    const shared_ptr<value>& alternative) {
    // x, 1, 2 -> (if x 1 2)
    return make_list(
        "if",
        predicate,
        consequent,
        alternative);
}

void check_lambda(const shared_ptr<value>& exp) {
    // (lambda (p1 p2 ...) e1 e2 ...)
    auto lambda = to_list(exp);
    if (!lambda) {
        throw syntax_error("lambda: non-list structure in %s", exp->str().c_str());
    } else if (lambda->cdr() == nil) {
        throw syntax_error("lambda: no parameters in %s", exp->str().c_str());
    } else if (lambda->pcdr()->cdr() == nil) {
        throw syntax_error("lambda: no body in %s", exp->str().c_str());
    } else if (lambda->pcdr()->car()->type() == value_t::symbol) {
        // parameters are symbol: stop here
        return;
    } else if (lambda->pcdr()->car()->type() == value_t::pair) {
        // parameters are a (possibly non-nil-terminated) list
        unordered_set<string> seen;
        for (const auto& param : *lambda->pcdr()->pcar()) {
            if (param.type() != value_t::symbol) {
                throw syntax_error(
                    "lambda: some parameters are not symbols in %s",
                    exp->str().c_str());
            }
            auto name = reinterpret_cast<const value_symbol*>(&param)->symbol();
            if (seen.count(name) > 0) {
                throw syntax_error(
                    "lambda: duplicate parameter names in %s",
                    exp->str().c_str());
            }
            seen.insert(name);
        }
    } else {
        throw syntax_error(
            "lambda: some parameters are not symbols in %s",
            exp->str().c_str());
    }
}

shared_ptr<value> get_lambda_parameters(const shared_ptr<value>& lambda) {
    // (p1 p2 ...) from (lambda (p1 p2 ...) e1 e2 ...)
    return to_ptr<value_pair>(lambda)->pcdr()->car();
}

shared_ptr<value> get_lambda_body(const shared_ptr<value>& lambda) {
    // (e1 e2 ...) from (lambda (p1 p2 ...) e1 e2 ...)
    return to_ptr<value_pair>(lambda)->pcdr()->cdr();
}

shared_ptr<value> make_lambda(
    const shared_ptr<value>& params,
    const shared_ptr<value>& body) {
    // (p1 p2 ...), (e1 e2 ...) -> (lambda (p1 p2 ...) e1 e2 ...)
    return make_vpair(
        "lambda",
        make_vpair(
            params,
            body));
}

void check_let(const shared_ptr<value>& exp) {
    // (let ((x 1) (y 2)) (+ x y) x)
    auto let = to_list(exp);
    if (!let) {
        throw syntax_error("let: non-list structure in %s", exp->str().c_str());
    } else if (let->cdr() == nil) {
        throw syntax_error("let: no variables in %s", exp->str().c_str());
    } else if (let->pcdr()->cdr() == nil) {
        throw syntax_error("let: no body in %s", exp->str().c_str());
    }
    auto variables = to_list(let->pcdr()->car());
    if (!variables) {
        throw syntax_error("let: non-list variables in %s", exp->str().c_str());
    } else if (variables == nilptr) {
        throw syntax_error("let: no variable name in %s", exp->str().c_str());
    }
    for (const auto& variable : *variables) {
        auto pair = to_list(variable);
        if (!pair) {
            throw syntax_error("let: non-list variable pair in %s", exp->str().c_str());
        } else if (pair->car()->type() != value_t::symbol) {
            throw syntax_error("let: variable name must be a symbol in %s", exp->str().c_str());
        } else if (pair->cdr() == nil) {
            throw syntax_error("let: no variable value in %s", exp->str().c_str());
        } else if (pair->pcdr()->cdr() != nil) {
            throw syntax_error("let: too many items in a variable pair in %s", exp->str().c_str());
        }
    }
}

shared_ptr<value> transform_let(const shared_ptr<value>& let) {
    // (let ((x 1) (y 2)) (+ x y) x) -> ((lambda (x y) (+ x y) x) 1 2)
    shared_ptr<value_pair> params;
    shared_ptr<value_pair> params_tail;
    shared_ptr<value_pair> args;
    shared_ptr<value_pair> args_tail;

    auto let_list = to_ptr<value_pair>(let);
    for (const auto& variable : *let_list->pcdr()->pcar()) {
        auto param_and_arg = reinterpret_cast<const value_pair*>(&variable);

        // append to params
        auto next_param = make_vpair(param_and_arg->car(), nil);
        if (!params_tail) {
            params = next_param;
        } else {
            params_tail->cdr(next_param);
        }
        params_tail = next_param;

        // append to args
        auto next_arg = make_vpair(param_and_arg->pcdr()->car(), nil);
        if (!args_tail) {
            args = next_arg;
        } else {
            args_tail->cdr(next_arg);
        }
        args_tail = next_arg;
    }

    return make_vpair(
        make_lambda(
            params,
            let_list->pcdr()->cdr()),  // body
        args);
}

void check_begin(const shared_ptr<value>& exp) {
    // (begin e1 e2 ...)
    auto begin = to_list(exp);
    if (!begin) {
        throw syntax_error("begin: non-list structure in %s", exp->str().c_str());
    } else if (begin->cdr() == nil) {
        throw syntax_error("begin: no expressions in %s", exp->str().c_str());
    }
}

shared_ptr<value> get_begin_actions(const shared_ptr<value>& begin) {
    // (e1 e2 ...) from (begin e1 e2 ...)
    return to_ptr<value_pair>(begin)->cdr();
}

shared_ptr<value> transform_sequence(const shared_ptr<value>& seq) {
    if (seq == nil) {
        // () -> ()
        return seq;
    }
    auto seq_list = to_ptr<value_pair>(seq);
    if (seq_list->cdr() == nil) {
        // (e) -> e
        return seq_list->car();
    } else {
        // (e1 e2 ...) -> (begin e1 e2 ...)
        return make_vpair("begin", seq);
    }
}

void check_cond(const shared_ptr<value>& exp) {
    // (cond (p1 e11 e12 ...) (p2 e21 e22 ...) ...)
    // (cond (p1 e11 e12 ...) (p2 e21 e22 ...) ... (else ee1 ee2 ...))
    auto cond = to_list(exp);
    if (!cond) {
        throw syntax_error("cond: non-list structure in %s", exp->str().c_str());
    } else if (cond->cdr() == nil) {
        throw syntax_error("cond: no clauses in %s", exp->str().c_str());
    }
    bool else_clause_seen = false;
    for (const auto& clause : *cond->pcdr()) {
        if (else_clause_seen) {
            throw syntax_error("cond: else clause must be the last in %s", exp->str().c_str());
        } else if (clause.type() == value_t::nil) {
            throw syntax_error("cond: empty clause in %s", exp->str().c_str());
        }
        auto clause_list = to_list(clause);
        if (!clause_list) {
            throw syntax_error("cond: non-list clause in %s", exp->str().c_str());
        } else if (clause_list->cdr() == nil) {
            throw syntax_error("cond: clause without consequent in %s", exp->str().c_str());
        } else if (is_else_clause(clause_list)) {
            else_clause_seen = true;  // else clause must be in the end
        }
    }
}

shared_ptr<value> transform_cond(const shared_ptr<value>& cond) {
    // (cond (p1 e1) (p2 e21 e22) (else ee1) -> (if p1 e1 (if p2 (begin e21 e22) ee1))
    // (cond (p1 e1) (p2 e21 e22) -> (if p1 e1 (if p2 (begin e21 e22) false))
    return transform_cond_rec(to_ptr<value_pair>(cond)->pcdr());
}

void check_and(const shared_ptr<value>& exp) {
    // (and ...)
    if (!to_list(exp)) {
        throw syntax_error("and: non-list structure in %s", exp->str().c_str());
    }
}

shared_ptr<value> get_and_expressions(const shared_ptr<value>& and_) {
    // (...) from (and ...)
    return to_ptr<value_pair>(and_)->cdr();
}

void check_or(const shared_ptr<value>& exp) {
    // (or ...)
    if (!to_list(exp)) {
        throw syntax_error("or: non-list structure in %s", exp->str().c_str());
    }
}

shared_ptr<value> get_or_expressions(const shared_ptr<value>& or_) {
    // (...) from (or ...)
    return to_ptr<value_pair>(or_)->cdr();
}

void check_eval(const shared_ptr<value>& exp) {
    // (eval e)
    auto eval = to_list(exp);
    if (!eval) {
        throw syntax_error("eval: non-list structure in %s", exp->str().c_str());
    } else if (eval->cdr() == nil) {
        throw syntax_error("eval: no expression in %s", exp->str().c_str());
    } else if (eval->pcdr()->cdr() != nil) {
        throw syntax_error("eval: too many items in %s", exp->str().c_str());
    }
}

shared_ptr<value> get_eval_expression(const shared_ptr<value>& eval) {
    // e from (eval e)
    return to_ptr<value_pair>(eval)->pcdr()->car();
}

void check_apply(const shared_ptr<value>& exp) {
    // (apply f (a1 a2 ...))
    auto apply = to_list(exp);
    if (!apply) {
        throw syntax_error("apply: non-list structure in %s", exp->str().c_str());
    } else if (apply->cdr() == nil) {
        throw syntax_error("apply: no operator in %s", exp->str().c_str());
    } else if (apply->pcdr()->cdr() == nil) {
        throw syntax_error("apply: no arguments in %s", exp->str().c_str());
    } else if (apply->pcdr()->pcdr()->cdr() != nil) {
        throw syntax_error("apply: too many items in %s", exp->str().c_str());
    }
}

shared_ptr<value> get_apply_operator(const shared_ptr<value>& apply) {
    // f from (apply f (a1 a2 ...))
    return to_ptr<value_pair>(apply)->pcdr()->car();
}

shared_ptr<value> get_apply_arguments(const shared_ptr<value>& apply) {
    // (a1 a2 ...) from (apply f (a1 a2 ...))
    return to_ptr<value_pair>(apply)->pcdr()->pcdr()->car();
}

void check_apply_arguments(const shared_ptr<value>& args) {
    // (...)
    if (!to_list(args)) {
        throw syntax_error("apply: can't apply to %s", args->str().c_str());
    }
}

void check_application(const shared_ptr<value>& exp) {
    // (f ...) with any f
    if (exp == nil) {
        throw syntax_error("bad application %s", exp->str().c_str());
    } else if (!to_list(exp)) {
        throw syntax_error("can't apply to %s", exp->str().c_str());
    }
}

bool has_no_exps(const shared_ptr<value>& seq) {
    // ()
    return seq == nil;
}

bool is_last_exp(const shared_ptr<value>& seq) {
    // (e)
    return to_ptr<value_pair>(seq)->cdr() == nil;
}

shared_ptr<value> get_first_exp(const shared_ptr<value>& seq) {
    // e1 from (e1 e2 ...)
    return to_ptr<value_pair>(seq)->car();
}

shared_ptr<value> get_rest_exps(const shared_ptr<value>& seq) {
    // (e2 ...) from (e1 e2 ...)
    return to_ptr<value_pair>(seq)->cdr();
}

shared_ptr<value> get_operator(const shared_ptr<value>& compound) {
    // f from (f p1 p2 ...)
    return to_ptr<value_pair>(compound)->car();
}

shared_ptr<value> get_operands(const shared_ptr<value>& compound) {
    // (p1 p2 ...) from (f p1 p2 ...)
    return to_ptr<value_pair>(compound)->car();
}

bool has_no_operands(const shared_ptr<value>& operands) {
    // ()
    return operands == nil;
}

bool is_last_operand(const shared_ptr<value>& operands) {
    // (o)
    return to_ptr<value_pair>(operands)->cdr() == nil;
}

shared_ptr<value> get_first_operand(const shared_ptr<value>& operands) {
    // o1 from (o1 o2 ...)
    return to_ptr<value_pair>(operands)->car();
}

shared_ptr<value> get_rest_operands(const shared_ptr<value>& operands) {
    // (o2 ...) from (o1 o2 ...)
    return to_ptr<value_pair>(operands)->cdr();
}

shared_ptr<value> make_empty_arglist() {
    // ()
    return nil;
}

shared_ptr<value> adjoin_arg(
    const shared_ptr<value>& arg,
    const shared_ptr<value>& arg_list) {
    // a, () -> (a)
    // a, (a1 a2 .. an) -> (a1 a2 ... an a)
    auto new_arg = make_vpair(arg, nil);
    if (arg_list == nil) {
        // one-argument list
        return new_arg;
    } else {
        // append to the list
        auto last_arg = to_ptr<value_pair>(arg_list);
        while (last_arg->cdr() != nil) {
            last_arg = to_ptr<value_pair>(last_arg->cdr());
        }
        last_arg->cdr(new_arg);
        return arg_list;
    }
}
