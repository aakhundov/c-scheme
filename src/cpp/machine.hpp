#ifndef MACHINE_HPP_
#define MACHINE_HPP_

#include <iomanip>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "code.hpp"
#include "constants.hpp"
#include "error.hpp"
#include "value.hpp"

using std::pair;
using std::setfill;
using std::setw;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::vector;

// types

using machine_op = shared_ptr<value> (*)(const vector<value_pair*>&);

// exceptions

class machine_error : public scheme_error {
   public:
    template <typename... Args>
    machine_error(const char* format, Args&&... args)
        : scheme_error("machine error", format, forward<Args>(args)...) {}
};

// machine

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

    // can't copy, can move
    machine(const machine&) = delete;
    void operator=(machine const&) = delete;
    machine(machine&&) = default;
    machine& operator=(machine&&) = default;

    void bind_op(const string& name, const machine_op& op) {
        _get_op(name)->op(op);
    }

    shared_ptr<value> read_from(const string& register_name) {
        return _get_register(register_name)->car();
    }

    void write_to(const string& register_name, const shared_ptr<value>& v) {
        _get_register(register_name)->car(v);
    }

    void append_and_jump(const vector<shared_ptr<code>>& code) {
        _pc = _append_code(code);
    }

    void trace(machine_trace trace) {
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
        value_machine_op(const string& name)
            : value(value_t::machine_op), _name{name} {}

        ostream& write(ostream& os) const override {
            return (os << "<machine op '" << _name << "'>");
        };

        const string& name() const { return _name; }
        const machine_op op() const { return _op; }
        void op(machine_op op) { _op = op; }

       private:
        const string _name;
        machine_op _op{nullptr};
    };

    // abstact base class for the instructions
    class value_instruction : public value {
       public:
        value_instruction(machine& machine)
            : value(value_t::instruction), _machine(machine) {}

        ostream& write(ostream& os) const override {
            trace_before(os);
            return os;
        }

        // execute the instruction
        virtual void execute() const = 0;

        // for tracing before and after execution
        virtual void trace_before(ostream& os) const = 0;
        virtual void trace_after(ostream& os) const = 0;

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

    void _move_pc(const shared_ptr<value>& position) {
        // set the pc to a given position
        _pc = to_ptr<value_pair>(position);
    }

    void _move_pc_to_beginning() {
        // move the pc to the end of the program
        _pc = _code_head.get();
    }

    void _move_pc_to_end() {
        // move the pc to the end of the program
        _pc = nilptr;
    }

    void _set_output(const shared_ptr<value>& v) {
        // write v to the current output register
        _output->car(v);
    }

    shared_ptr<value> _call_op(const value_machine_op* v, const vector<value_pair*>& _args) {
        if (auto op = v->op()) {
            return op(_args);  // call the op
        } else {
            throw machine_error("%s is unbound", v->str().c_str());
        }
    }

    void _push_to_stack(const shared_ptr<value>& v) {
        _stack.push_back(v);
    }

    shared_ptr<value> _pop_from_stack() {
        if (_stack.empty()) {
            throw machine_error("can't pop from empty stack");
        } else {
            auto v = _stack.back();
            _stack.pop_back();
            return v;
        }
    }

    void _trace_before(ostream& os, const value_instruction* instruction) {
        os << BLUE(<< setfill('0') << setw(5) << ++_counter <<) " ";
        instruction->trace_before(os);
        os.flush();
    }

    void _trace_after(ostream& os, const value_instruction* instruction) {
        instruction->trace_after(os);
        os << '\n';
    }

    value_pair* _get_constant(const shared_ptr<value>& val);
    value_pair* _get_register(const string& name);
    value_pair* _get_label(const string& name);
    value_machine_op* _get_op(const string& name);

    value_pair* _token_to_arg(const token& t);
    const vector<value_pair*> _tokens_to_args(const vector<token>& tokens);

    shared_ptr<value_instruction> _make_instruction(const shared_ptr<code>& line);
    value_pair* _append_code(const vector<shared_ptr<code>>& code);

    shared_ptr<value_pair> _constants{nil};  // chain of constants
    shared_ptr<value_pair> _registers{nil};  // chain of registers
    shared_ptr<value_pair> _labels{nil};     // chain of labels
    shared_ptr<value_pair> _ops{nil};        // chain of ops

    unordered_map<string, value_pair*> _register_map;  // name to register
    unordered_map<string, value_pair*> _label_map;     // name to label
    unordered_map<string, value_machine_op*> _op_map;  // name to op

    shared_ptr<value_pair> _code_head{nil};  // head pair of the code
    shared_ptr<value_pair> _code_tail{nil};  // tail pair of the code

    vector<shared_ptr<value>> _stack;  // stack of values

    const value_pair* _pc;  // curent code position
    value_pair* _output;    // output register

    machine_trace _trace{machine_trace::off};  // machine tracing flag
    size_t _counter{0};                        // instruction counter
};

#endif  // MACHINE_HPP_
