#ifndef EVALUATOR_H_
#define EVALUATOR_H_

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "machine.hpp"
#include "primitives.hpp"
#include "value.hpp"

using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::filesystem::path;

class evaluator {
   public:
    evaluator(path path_to_code)
        : _global(_make_global()),
          _machine(_make_machine(path_to_code)) {
        _machine.write_to("env", _global);
    }

    // can't copy, can move
    evaluator(const evaluator&) = delete;
    void operator=(evaluator const&) = delete;
    evaluator(evaluator&&) = default;
    evaluator& operator=(evaluator&&) = default;

    shared_ptr<value> evaluate(const shared_ptr<value>& expression) {
        return _machine.run({{"exp", expression}, {"env", _global}}, "val");
    }

    const unordered_map<string, shared_ptr<value>>& global() const {
        return _global->values();
    }

    void reset() {
        _global = _make_global();
    }

    void trace(machine_trace trace) {
        _machine.trace(trace);
    }

   private:
    class value_environment : public value {
       public:
        value_environment() : value(value_t::environment) {}
        value_environment(shared_ptr<value_environment> base)
            : value(value_t::environment), _base(base) {}

        ostream& write(ostream& os) const override {
            if (!_base) {
                return (os << "<global>");
            } else {
                os << "<env";
                for (const auto& [name, val] : _values) {
                    os << " " << name << "=" << *val;
                }
                os << ">";
                return os;
            }
        }

        shared_ptr<value> lookup(const string& name, bool recursive = true) const {
            auto iter = _values.find(name);
            if (iter != _values.end()) {
                return iter->second;
            } else if (recursive && _base) {
                return _base->lookup(name, recursive);  // TCO
            } else {
                return nullptr;
            }
        }

        bool update(const string& name, const shared_ptr<value>& val, bool recursive = true) {
            auto iter = _values.find(name);
            if (iter != _values.end()) {
                _values[name] = val;
                return true;
            } else if (recursive && _base) {
                return _base->update(name, val, recursive);  // TCO
            } else {
                return false;
            }
        }

        void add(const string& name, const shared_ptr<value>& val) {
            _values[name] = val;
        }

        const unordered_map<string, shared_ptr<value>>& values() {
            return _values;
        }

       private:
        unordered_map<string, shared_ptr<value>> _values;
        shared_ptr<value_environment> _base{nullptr};
    };

    class value_primitive_op : public value {
       public:
        value_primitive_op(string name, primitive_op op)
            : value(value_t::primitive_op), _name(name), _op(op) {}

        ostream& write(ostream& os) const override {
            return (os << "<primitive '" << _name << "'>");
        };

        const string& name() const { return _name; }
        primitive_op op() const { return _op; }

       private:
        string _name;
        primitive_op _op;
    };

    class value_compound_op : public value {
       public:
        value_compound_op(
            const shared_ptr<value>& params,
            const shared_ptr<value>& body,
            const shared_ptr<value_environment>& env)
            : value(value_t::compound_op), _params(params), _body(body), _env(env) {}

        ostream& write(ostream& os) const override {
            string body = _body->str();
            body = body.substr(1, body.size() - 2);  // drop outer braces
            return (os << "(lambda " << *_params << " " << body << ")");
        };

        const shared_ptr<value>& params() const { return _params; }
        const shared_ptr<value>& body() const { return _body; }
        const shared_ptr<value_environment>& env() const { return _env; }

       private:
        shared_ptr<value> _params;
        shared_ptr<value> _body;
        shared_ptr<value_environment> _env;
    };

    static shared_ptr<value> op_check_quoted(const vector<value_pair*>& args);
    static shared_ptr<value> op_text_of_quotation(const vector<value_pair*>& args);
    static shared_ptr<value> op_check_assignment(const vector<value_pair*>& args);
    static shared_ptr<value> op_assignment_variable(const vector<value_pair*>& args);
    static shared_ptr<value> op_assignment_value(const vector<value_pair*>& args);
    static shared_ptr<value> op_check_definition(const vector<value_pair*>& args);
    static shared_ptr<value> op_definition_variable(const vector<value_pair*>& args);
    static shared_ptr<value> op_definition_value(const vector<value_pair*>& args);
    static shared_ptr<value> op_check_if(const vector<value_pair*>& args);
    static shared_ptr<value> op_if_predicate(const vector<value_pair*>& args);
    static shared_ptr<value> op_if_consequent(const vector<value_pair*>& args);
    static shared_ptr<value> op_if_alternative(const vector<value_pair*>& args);
    static shared_ptr<value> op_check_lambda(const vector<value_pair*>& args);
    static shared_ptr<value> op_lambda_parameters(const vector<value_pair*>& args);
    static shared_ptr<value> op_lambda_body(const vector<value_pair*>& args);
    static shared_ptr<value> op_check_let(const vector<value_pair*>& args);
    static shared_ptr<value> op_transform_let(const vector<value_pair*>& args);
    static shared_ptr<value> op_check_begin(const vector<value_pair*>& args);
    static shared_ptr<value> op_begin_actions(const vector<value_pair*>& args);
    static shared_ptr<value> op_check_cond(const vector<value_pair*>& args);
    static shared_ptr<value> op_transform_cond(const vector<value_pair*>& args);
    static shared_ptr<value> op_check_and(const vector<value_pair*>& args);
    static shared_ptr<value> op_and_expressions(const vector<value_pair*>& args);
    static shared_ptr<value> op_check_or(const vector<value_pair*>& args);
    static shared_ptr<value> op_or_expressions(const vector<value_pair*>& args);
    static shared_ptr<value> op_check_eval(const vector<value_pair*>& args);
    static shared_ptr<value> op_eval_expression(const vector<value_pair*>& args);
    static shared_ptr<value> op_check_apply(const vector<value_pair*>& args);
    static shared_ptr<value> op_apply_operator(const vector<value_pair*>& args);
    static shared_ptr<value> op_apply_arguments(const vector<value_pair*>& args);
    static shared_ptr<value> op_check_apply_args(const vector<value_pair*>& args);
    static shared_ptr<value> op_check_application(const vector<value_pair*>& args);
    static shared_ptr<value> op_no_exps_q(const vector<value_pair*>& args);
    static shared_ptr<value> op_last_exp_q(const vector<value_pair*>& args);
    static shared_ptr<value> op_first_exp(const vector<value_pair*>& args);
    static shared_ptr<value> op_rest_exps(const vector<value_pair*>& args);
    static shared_ptr<value> op_operator(const vector<value_pair*>& args);
    static shared_ptr<value> op_operands(const vector<value_pair*>& args);
    static shared_ptr<value> op_no_operands_q(const vector<value_pair*>& args);
    static shared_ptr<value> op_last_operand_q(const vector<value_pair*>& args);
    static shared_ptr<value> op_first_operand(const vector<value_pair*>& args);
    static shared_ptr<value> op_rest_operands(const vector<value_pair*>& args);
    static shared_ptr<value> op_make_empty_arglist(const vector<value_pair*>& args);
    static shared_ptr<value> op_adjoin_arg(const vector<value_pair*>& args);
    static shared_ptr<value> op_true_q(const vector<value_pair*>& args);
    static shared_ptr<value> op_false_q(const vector<value_pair*>& args);
    static shared_ptr<value> op_make_true(const vector<value_pair*>& args);
    static shared_ptr<value> op_make_false(const vector<value_pair*>& args);
    static shared_ptr<value> op_primitive_procedure_q(const vector<value_pair*>& args);
    static shared_ptr<value> op_compound_procedure_q(const vector<value_pair*>& args);
    static shared_ptr<value> op_compiled_procedure_q(const vector<value_pair*>& args);
    static shared_ptr<value> op_compound_parameters(const vector<value_pair*>& args);
    static shared_ptr<value> op_compound_body(const vector<value_pair*>& args);
    static shared_ptr<value> op_compound_environment(const vector<value_pair*>& args);
    static shared_ptr<value> op_make_compound_procedure(const vector<value_pair*>& args);
    static shared_ptr<value> op_signal_error(const vector<value_pair*>& args);
    static shared_ptr<value> op_apply_primitive_procedure(const vector<value_pair*>& args);
    static shared_ptr<value> op_lookup_variable_value(const vector<value_pair*>& args);
    static shared_ptr<value> op_set_variable_value(const vector<value_pair*>& args);
    static shared_ptr<value> op_define_variable(const vector<value_pair*>& args);
    static shared_ptr<value> op_extend_environment(const vector<value_pair*>& args);
    static shared_ptr<value> op_dispatch_table_ready_q(const vector<value_pair*>& args);
    static shared_ptr<value> op_make_dispatch_table(const vector<value_pair*>& args);
    static shared_ptr<value> op_add_dispatch_record(const vector<value_pair*>& args);
    static shared_ptr<value> op_dispatch_on_type(const vector<value_pair*>& args);

    void _bind_machine_ops(machine& m);
    machine _make_machine(path path_to_code);
    shared_ptr<value_environment> _make_global();

    shared_ptr<value_environment> _global;
    machine _machine;
};

#endif  // EVALUATOR_H_
