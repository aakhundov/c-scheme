#include "evaluator.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "code.hpp"
#include "parsing.hpp"
#include "primitives.hpp"
#include "syntax.hpp"
#include "value.hpp"

using std::make_shared;
using std::shared_ptr;
using std::snprintf;
using std::strcpy;
using std::string;
using std::unordered_map;
using std::vector;
using std::filesystem::path;

shared_ptr<value> evaluator::op_check_quoted(const vector<value_pair*>& args) {
    check_quoted(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_text_of_quotation(const vector<value_pair*>& args) {
    return get_text_of_quotation(args[0]->car());
}

shared_ptr<value> evaluator::op_check_assignment(const vector<value_pair*>& args) {
    check_assignment(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_assignment_variable(const vector<value_pair*>& args) {
    return get_assignment_variable(args[0]->car());
}

shared_ptr<value> evaluator::op_assignment_value(const vector<value_pair*>& args) {
    return get_assignment_value(args[0]->car());
}

shared_ptr<value> evaluator::op_check_definition(const vector<value_pair*>& args) {
    check_definition(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_definition_variable(const vector<value_pair*>& args) {
    return get_definition_variable(args[0]->car());
}

shared_ptr<value> evaluator::op_definition_value(const vector<value_pair*>& args) {
    return get_definition_value(args[0]->car());
}

shared_ptr<value> evaluator::op_check_if(const vector<value_pair*>& args) {
    check_if(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_if_predicate(const vector<value_pair*>& args) {
    return get_if_predicate(args[0]->car());
}

shared_ptr<value> evaluator::op_if_consequent(const vector<value_pair*>& args) {
    return get_if_consequent(args[0]->car());
}

shared_ptr<value> evaluator::op_if_alternative(const vector<value_pair*>& args) {
    return get_if_alternative(args[0]->car());
}

shared_ptr<value> evaluator::op_check_lambda(const vector<value_pair*>& args) {
    check_lambda(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_lambda_parameters(const vector<value_pair*>& args) {
    return get_lambda_parameters(args[0]->car());
}

shared_ptr<value> evaluator::op_lambda_body(const vector<value_pair*>& args) {
    return get_lambda_body(args[0]->car());
}

shared_ptr<value> evaluator::op_check_let(const vector<value_pair*>& args) {
    check_let(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_transform_let(const vector<value_pair*>& args) {
    return transform_let(args[0]->car());
}

shared_ptr<value> evaluator::op_check_begin(const vector<value_pair*>& args) {
    check_begin(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_begin_actions(const vector<value_pair*>& args) {
    return get_begin_actions(args[0]->car());
}

shared_ptr<value> evaluator::op_check_cond(const vector<value_pair*>& args) {
    check_cond(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_transform_cond(const vector<value_pair*>& args) {
    return transform_cond(args[0]->car());
}

shared_ptr<value> evaluator::op_check_and(const vector<value_pair*>& args) {
    check_and(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_and_expressions(const vector<value_pair*>& args) {
    return get_and_expressions(args[0]->car());
}

shared_ptr<value> evaluator::op_check_or(const vector<value_pair*>& args) {
    check_or(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_or_expressions(const vector<value_pair*>& args) {
    return get_or_expressions(args[0]->car());
}

shared_ptr<value> evaluator::op_check_eval(const vector<value_pair*>& args) {
    check_eval(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_eval_expression(const vector<value_pair*>& args) {
    return get_eval_expression(args[0]->car());
}

shared_ptr<value> evaluator::op_check_apply(const vector<value_pair*>& args) {
    check_apply(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_apply_operator(const vector<value_pair*>& args) {
    return get_apply_operator(args[0]->car());
}

shared_ptr<value> evaluator::op_apply_arguments(const vector<value_pair*>& args) {
    return get_apply_arguments(args[0]->car());
}

shared_ptr<value> evaluator::op_check_apply_args(const vector<value_pair*>& args) {
    check_apply_arguments(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_check_application(const vector<value_pair*>& args) {
    check_application(args[0]->car());
    return nil;
}

shared_ptr<value> evaluator::op_no_exps_q(const vector<value_pair*>& args) {
    return has_no_exps(args[0]->car()) ? true_ : false_;
}

shared_ptr<value> evaluator::op_last_exp_q(const vector<value_pair*>& args) {
    return is_last_exp(args[0]->car()) ? true_ : false_;
}

shared_ptr<value> evaluator::op_first_exp(const vector<value_pair*>& args) {
    return get_first_exp(args[0]->car());
}

shared_ptr<value> evaluator::op_rest_exps(const vector<value_pair*>& args) {
    return get_rest_exps(args[0]->car());
}

shared_ptr<value> evaluator::op_operator(const vector<value_pair*>& args) {
    return get_operands(args[0]->car());
}

shared_ptr<value> evaluator::op_operands(const vector<value_pair*>& args) {
    return get_operands(args[0]->car());
}

shared_ptr<value> evaluator::op_no_operands_q(const vector<value_pair*>& args) {
    return has_no_operands(args[0]->car()) ? true_ : false_;
}

shared_ptr<value> evaluator::op_last_operand_q(const vector<value_pair*>& args) {
    return is_last_operand(args[0]->car()) ? true_ : false_;
}

shared_ptr<value> evaluator::op_first_operand(const vector<value_pair*>& args) {
    return get_first_operand(args[0]->car());
}

shared_ptr<value> evaluator::op_rest_operands(const vector<value_pair*>& args) {
    return get_rest_operands(args[0]->car());
}

shared_ptr<value> evaluator::op_make_empty_arglist(const vector<value_pair*>& args) {
    return make_empty_arglist();
}

shared_ptr<value> evaluator::op_adjoin_arg(const vector<value_pair*>& args) {
    auto arg = args[0]->car();
    auto arg_list = args[1]->car();
    return adjoin_arg(arg, arg_list);
}

shared_ptr<value> evaluator::op_true_q(const vector<value_pair*>& args) {
    return (*args[0]->car() ? true_ : false_);
}

shared_ptr<value> evaluator::op_false_q(const vector<value_pair*>& args) {
    return (*args[0]->car() ? false_ : true_);
}

shared_ptr<value> evaluator::op_make_true(const vector<value_pair*>& args) {
    return true_;
}

shared_ptr<value> evaluator::op_make_false(const vector<value_pair*>& args) {
    return false_;
}

shared_ptr<value> evaluator::op_primitive_procedure_q(const vector<value_pair*>& args) {
    return (args[0]->car()->type() == value_t::primitive_op ? true_ : false_);
}

shared_ptr<value> evaluator::op_compound_procedure_q(const vector<value_pair*>& args) {
    return (args[0]->car()->type() == value_t::compound_op ? true_ : false_);
}

shared_ptr<value> evaluator::op_compound_parameters(const vector<value_pair*>& args) {
    return to_ptr<value_compound_op>(args[0]->car())->params();
}

shared_ptr<value> evaluator::op_compound_body(const vector<value_pair*>& args) {
    return to_ptr<value_compound_op>(args[0]->car())->body();
}

shared_ptr<value> evaluator::op_compound_environment(const vector<value_pair*>& args) {
    return to_ptr<value_compound_op>(args[0]->car())->env();
}

shared_ptr<value> evaluator::op_make_compound_procedure(const vector<value_pair*>& args) {
    auto params = args[0]->car();
    auto body = args[1]->car();
    auto env = to_sptr<value_environment>(args[2]->car());

    return make_shared<value_compound_op>(params, body, env);
}

shared_ptr<value> evaluator::op_signal_error(const vector<value_pair*>& args) {
    static char buffer[65536];

    string format_str = to_ptr<value_string>(args[0]->car())->string_();
    const char* format = format_str.c_str();

    if (args.size() == 2) {
        // 1 parameter
        string v1 = args[1]->car()->str();
        snprintf(buffer, sizeof(buffer), format, v1.c_str());
    } else if (args.size() == 3) {
        // 2 parameters
        string v1 = args[1]->car()->str();
        string v2 = args[2]->car()->str();
        snprintf(buffer, sizeof(buffer), format, v1.c_str(), v2.c_str());
    } else if (args.size() == 4) {
        // 3 parameters
        string v1 = args[1]->car()->str();
        string v2 = args[2]->car()->str();
        string v3 = args[3]->car()->str();
        snprintf(buffer, sizeof(buffer), format, v1.c_str(), v2.c_str(), v3.c_str());
    } else {
        // 0 or 3+ parameters: use the format only
        snprintf(buffer, sizeof(buffer), "%s", format);
    }

    return make_error(buffer);
}

shared_ptr<value> evaluator::op_apply_primitive_procedure(const vector<value_pair*>& args) {
    auto procedure = to_ptr<value_primitive_op>(args[0]->car());
    auto arguments = to_ptr<const value_pair>(args[0]->car());

    auto result = procedure->op()(arguments);
    if (result->type() == value_t::error) {
        auto error = to_ptr<value_error>(result);
        error->topic("error applying " + procedure->name());
    }
    return result;
}

shared_ptr<value> evaluator::op_lookup_variable_value(const vector<value_pair*>& args) {
    auto name = to_ptr<value_symbol>(args[0]->car());
    auto env = to_ptr<value_environment>(args[1]->car());

    if (auto val = env->lookup(name->symbol())) {
        return val;
    } else {
        return make_error("%s is unbound", name->symbol().c_str());
    }
}

shared_ptr<value> evaluator::op_set_variable_value(const vector<value_pair*>& args) {
    auto name = to_ptr<value_symbol>(args[0]->car());
    auto val = args[1]->car();
    auto env = to_ptr<value_environment>(args[2]->car());

    if (env->update(name->symbol(), val)) {
        return nil;
    } else {
        return make_error("%s is unbound", name->symbol().c_str());
    }
}

shared_ptr<value> evaluator::op_define_variable(const vector<value_pair*>& args) {
    auto name = to_ptr<value_symbol>(args[0]->car());
    auto val = args[1]->car();
    auto env = to_ptr<value_environment>(args[2]->car());

    if (env->update(name->symbol(), val, false)) {
        return make_info("%s is updated", name->symbol().c_str());
    } else {
        env->add(name->symbol(), val);
        return make_error("%s is defined", name->symbol().c_str());
    }
}

// shared_ptr<value> evaluator::op_extend_environment(const vector<value_pair*>& args) {
// }

// shared_ptr<value> evaluator::op_dispatch_table_ready_q(const vector<value_pair*>& args) {
// }

// shared_ptr<value> evaluator::op_make_dispatch_table(const vector<value_pair*>& args) {
// }

// shared_ptr<value> evaluator::op_add_dispatch_record(const vector<value_pair*>& args) {
// }

// shared_ptr<value> evaluator::op_dispatch_on_type(const vector<value_pair*>& args) {
// }

// evaluator

void evaluator::_bind_machine_ops(machine& m) {
    m.bind_op("check-quoted", evaluator::op_check_quoted);
    m.bind_op("text-of-quotation", evaluator::op_text_of_quotation);

    m.bind_op("check-assignment", evaluator::op_check_assignment);
    m.bind_op("assignment-variable", evaluator::op_assignment_variable);
    m.bind_op("assignment-value", evaluator::op_assignment_value);

    m.bind_op("check-definition", evaluator::op_check_definition);
    m.bind_op("definition-variable", evaluator::op_definition_variable);
    m.bind_op("definition-value", evaluator::op_definition_value);

    m.bind_op("check-if", evaluator::op_check_if);
    m.bind_op("if-predicate", evaluator::op_if_predicate);
    m.bind_op("if-consequent", evaluator::op_if_consequent);
    m.bind_op("if-alternative", evaluator::op_if_alternative);

    m.bind_op("check-lambda", evaluator::op_check_lambda);
    m.bind_op("lambda-parameters", evaluator::op_lambda_parameters);
    m.bind_op("lambda-body", evaluator::op_lambda_body);

    m.bind_op("check-let", evaluator::op_check_let);
    m.bind_op("transform-let", evaluator::op_transform_let);

    m.bind_op("check-begin", evaluator::op_check_begin);
    m.bind_op("begin-actions", evaluator::op_begin_actions);

    m.bind_op("check-cond", evaluator::op_check_cond);
    m.bind_op("transform-cond", evaluator::op_transform_cond);

    m.bind_op("check-and", evaluator::op_check_and);
    m.bind_op("and-expressions", evaluator::op_and_expressions);

    m.bind_op("check-or", evaluator::op_check_or);
    m.bind_op("or-expressions", evaluator::op_or_expressions);

    m.bind_op("check-eval", evaluator::op_check_eval);
    m.bind_op("eval-expression", evaluator::op_eval_expression);

    m.bind_op("check-apply", evaluator::op_check_apply);
    m.bind_op("apply-operator", evaluator::op_apply_operator);
    m.bind_op("apply-arguments", evaluator::op_apply_arguments);
    m.bind_op("check-apply-args", evaluator::op_check_apply_args);

    m.bind_op("check-application", evaluator::op_check_application);

    m.bind_op("no-exps?", evaluator::op_no_exps_q);
    m.bind_op("last-exp?", evaluator::op_last_exp_q);
    m.bind_op("first-exp", evaluator::op_first_exp);
    m.bind_op("rest-exps", evaluator::op_rest_exps);

    m.bind_op("operator", evaluator::op_operator);
    m.bind_op("operands", evaluator::op_operands);
    m.bind_op("no-operands?", evaluator::op_no_operands_q);
    m.bind_op("last-operand?", evaluator::op_last_operand_q);
    m.bind_op("first-operand", evaluator::op_first_operand);
    m.bind_op("rest-operands", evaluator::op_rest_operands);

    m.bind_op("make-empty-arglist", evaluator::op_make_empty_arglist);
    m.bind_op("adjoin-arg", evaluator::op_adjoin_arg);

    m.bind_op("true?", evaluator::op_true_q);
    m.bind_op("false?", evaluator::op_false_q);
    m.bind_op("make-true", evaluator::op_make_true);
    m.bind_op("make-false", evaluator::op_make_false);

    m.bind_op("primitive-procedure?", evaluator::op_primitive_procedure_q);
    m.bind_op("compound-procedure?", evaluator::op_compound_procedure_q);

    m.bind_op("compound-parameters", evaluator::op_compound_parameters);
    m.bind_op("compound-body", evaluator::op_compound_body);
    m.bind_op("compound-environment", evaluator::op_compound_environment);
    m.bind_op("make-compound-procedure", evaluator::op_make_compound_procedure);

    m.bind_op("signal-error", evaluator::op_signal_error);

    m.bind_op("apply-primitive-procedure", evaluator::op_apply_primitive_procedure);

    m.bind_op("lookup-variable-value", evaluator::op_lookup_variable_value);
    m.bind_op("set-variable-value!", evaluator::op_set_variable_value);
    m.bind_op("define-variable!", evaluator::op_define_variable);
    // m.bind_op("extend-environment", evaluator::op_extend_environment);

    // m.bind_op("dispatch-table-ready?", evaluator::op_dispatch_table_ready_q);
    // m.bind_op("make-dispatch-table", evaluator::op_make_dispatch_table);
    // m.bind_op("add-dispatch-record", evaluator::op_add_dispatch_record);
    // m.bind_op("dispatch-on-type", evaluator::op_dispatch_on_type);
}

machine evaluator::_make_machine(path path_to_code) {
    auto source = parse_values_from(path_to_code);
    auto code = translate_to_code(source);
    machine m{code};

    _bind_machine_ops(m);

    return m;
}

shared_ptr<evaluator::value_environment> evaluator::_make_global() {
    shared_ptr<value_environment> env = make_shared<value_environment>();

    for (const auto& prim : get_primitives()) {
        auto val = make_shared<value_primitive_op>(prim.first, prim.second);
        env->add(prim.first, val);
    }

    return env;
}
