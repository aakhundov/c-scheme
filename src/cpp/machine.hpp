#ifndef MACHINE_HPP_
#define MACHINE_HPP_

#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "code.hpp"
#include "constants.hpp"
#include "value.hpp"

using std::pair;
using std::setfill;
using std::setw;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::vector;

// exceptions

class machine_error : public format_exception {
   public:
    using format_exception::format_exception;
};

// machine

using machine_op = shared_ptr<value> (*)(const vector<shared_ptr<value_pair>>&);

enum class machine_trace {
    off,
    code,
};

class machine {
   public:
    machine(const vector<shared_ptr<code>>& code) {
        _append_code(code);
    }

    ~machine();

    void bind_op(const string& name, machine_op const op) {
        _get_op(name)->op(op);
    }

    shared_ptr<value> read_from_register(const string& name) {
        return _get_register(name)->car();
    }

    void write_to_register(const string& name, const shared_ptr<value>& v) {
        _get_register(name)->car(v);
    }

    void append_and_jump(const vector<shared_ptr<code>>& code) {
        auto code_head = _append_code(code);  // append
        _pc = code_head;                      // jump
    }

    void set_trace(machine_trace trace) {
        _trace = trace;
    }

    shared_ptr<value> run(
        const vector<pair<string, shared_ptr<value>>>& inputs,
        const string& output_register);

   private:
    // value wrapper for machine_ops
    class value_machine_op : public value {
       public:
        // create with a name, add op later
        value_machine_op(string name)
            : value(value_t::machine_op), _name{name} {}

        ostream& write(ostream& os) const override {
            return (os << "<machine op '" << _name << "'>");
        };

        const string& name() const { return _name; }
        const machine_op op() const { return _op; }
        void op(const machine_op op) { _op = op; }

       private:
        const string _name;
        machine_op _op{nullptr};
    };

    // abstact base class for the instructions
    class value_instruction : public value {
       public:
        value_instruction(machine& machine)
            : value(value_t::instruction), _machine(machine) {}

        virtual void execute() const = 0;

        ostream& write(ostream& os) const override {
            report_before(os);
            return os;
        }

        // for tracing of the instructions
        virtual void report_before(ostream& os) const = 0;
        virtual void report_after(ostream& os) const = 0;

       protected:
        machine& _machine;
    };

    // concrete instruction classes
    class instruction_assign_call;
    class instruction_assign_copy;
    class instruction_perform;
    class instruction_branch;
    class instruction_goto;
    class instruction_save;
    class instruction_restore;

    void _advance_pc() {
        // move the pc forward
        _pc = _pc->pcdr();
    }

    void _set_output(const shared_ptr<value>& v) {
        // write v to the current output register
        _output->car(v);
    }

    shared_ptr<value> _call_op(
        const shared_ptr<value_machine_op>& v,
        const vector<shared_ptr<value_pair>>& _args) {
        if (auto op = v->op()) {
            return op(_args);  // call the op
        } else {
            throw machine_error("%s is unbound", v->str().c_str());
        }
    }

    void _push_to_stack(shared_ptr<value> v) {
        // push v as a new head
        _stack = make_vpair(v, _stack);
    }

    shared_ptr<value> _pop_from_stack() {
        if (_stack != nil) {
            auto v = _stack->car();   // pop v
            _stack = _stack->pcdr();  // move the head
            return v;
        } else {
            throw machine_error("can't pop from empty stack");
        }
    }

    void _report_before(ostream& os, const shared_ptr<value_instruction>& instruction) {
        os << BLUE(<< setfill('0') << setw(5) << ++_counter <<) " ";
        instruction->report_before(os);
    }

    void _report_after(ostream& os, const shared_ptr<value_instruction>& instruction) {
        instruction->report_after(os);
        os << '\n';
    }

    shared_ptr<value_pair> _get_constant(shared_ptr<value> val);
    shared_ptr<value_pair> _get_register(const string& name);
    shared_ptr<value_pair> _get_label(const string& name);
    shared_ptr<value_machine_op> _get_op(const string& name);

    const shared_ptr<value_pair> _token_to_arg(const token& t);
    const vector<shared_ptr<value_pair>> _tokens_to_args(const vector<token>& tokens);

    shared_ptr<value_pair> _append_code(const vector<shared_ptr<code>>& code);

    shared_ptr<value_pair> _constants{nil};  // chain of constants
    shared_ptr<value_pair> _registers{nil};  // chain of registers
    shared_ptr<value_pair> _labels{nil};     // chain of labels
    shared_ptr<value_pair> _ops{nil};        // chain of ops

    unordered_map<string, shared_ptr<value_pair>> _register_map;  // name to register
    unordered_map<string, shared_ptr<value_pair>> _label_map;     // name to label
    unordered_map<string, shared_ptr<value_machine_op>> _op_map;  // name to op

    shared_ptr<value_pair> _code_head{nil};  // head pair of the code
    shared_ptr<value_pair> _code_tail{nil};  // tail pair of the code
    shared_ptr<value_pair> _stack{nil};      // stack as a chain of pairs
    shared_ptr<value_pair> _pc;              // curent code position
    shared_ptr<value_pair> _output;          // output register

    machine_trace _trace{machine_trace::off};  // machine tracing flag
    size_t _counter{0};                        // instruction counter
};

#endif  // MACHINE_HPP_
