#include "machine.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "code.hpp"
#include "constants.hpp"
#include "value.hpp"

using std::cout;
using std::ios_base;
using std::make_shared;
using std::move;
using std::pair;
using std::shared_ptr;
using std::string;
using std::vector;

// instruction hierarchy

namespace {

void annotate_register(ostream& os, const value_pair* reg) {
    os << BLUE("[");
    if (reg->car()->type() == value_t::pair &&
        reg->pcar()->car()->type() == value_t::instruction) {
        // the register points to code
        os << "<code>";
    } else {
        os << *reg->car();
    }
    os << BLUE("]");
}

}  // namespace

class machine::instruction_assign_call : public value_instruction {
   public:
    instruction_assign_call(machine& machine, const shared_ptr<code_assign_call>& code)
        : value_instruction(machine),
          _reg(machine._get_register(code->reg())),
          _op(machine._get_op(code->op())),
          _args(machine._tokens_to_args(code->args())),
          _code(code) {}

    void execute() const override {
        auto result = _machine._call_op(_op, _args);

        if (result->type() == value_t::error) {
            // halt the program
            _machine._output->car(result, false);
            _machine._move_pc_to_end();
        } else {
            // assign the result
            _reg->car(result, false);
            _machine._advance_pc();
        }
    }

    void trace_before(ostream& os) const override {
        os << "(" << BLUE("assign");
        os << " " << _code->reg();            // register
        os << " (op " << _code->op() << ")";  // op

        size_t i = 0;
        for (auto& t : _code->args()) {
            os << " " << t;
            if (t.type() == token_t::reg) {
                annotate_register(os, _args[i]);  // argument
            }
            ++i;
        }

        os << ")";
    }

    void trace_after(ostream& os) const override {
        os << BLUE(" == ");
        if (_machine._output->car()->type() == value_t::error) {
            // error occured
            os << *_machine._output->car();
        } else if (_reg) {
            if (_reg->car()->type() == value_t::pair &&
                _reg->pcar()->car()->type() == value_t::instruction) {
                // code returned
                os << "<code>";
            } else {
                os << *_reg->car();
            }
        }
    }

   private:
    value_pair* _reg;
    const value_machine_op* _op;
    const vector<value_pair*> _args;
    const shared_ptr<code_assign_call> _code;
};

class machine::instruction_assign_copy : public value_instruction {
   public:
    instruction_assign_copy(machine& machine, const shared_ptr<code_assign_copy>& code)
        : value_instruction(machine),
          _reg(machine._get_register(code->reg())),
          _src(machine._token_to_arg(code->src())),
          _code(code) {}

    void execute() const override {
        // assign from source
        _reg->car(_src->car(), false);
        _machine._advance_pc();
    }

    void trace_before(ostream& os) const override {
        os << "(" << BLUE("assign");
        os << " " << _code->reg();  // register

        os << " " << _code->src();
        if (_code->src().type() == token_t::reg) {
            annotate_register(os, _src);  // source
        }

        os << ")";
    }

    void trace_after(ostream& os) const override {
    }

   private:
    value_pair* _reg;
    const value_pair* _src;
    const shared_ptr<code_assign_copy> _code;
};

class machine::instruction_perform : public value_instruction {
   public:
    instruction_perform(machine& machine, const shared_ptr<code_perform>& code)
        : value_instruction(machine),
          _op(machine._get_op(code->op())),
          _args(machine._tokens_to_args(code->args())),
          _code(code) {}

    void execute() const override {
        auto result = _machine._call_op(_op, _args);

        if (result->type() == value_t::error) {
            // halt the program
            _machine._output->car(result, false);
            _machine._move_pc_to_end();
        } else {
            // just move the pc
            _machine._advance_pc();
        }
    }

    void trace_before(ostream& os) const override {
        os << "(" << BLUE("perform");
        os << " (op " << _code->op() << ")";  // op

        size_t i = 0;
        for (auto& t : _code->args()) {
            os << " " << t;
            if (t.type() == token_t::reg) {
                annotate_register(os, _args[i]);  // argument
            }
            ++i;
        }

        os << ")";
    }

    void trace_after(ostream& os) const override {
        if (_machine._output->car()->type() == value_t::error) {
            // error occured
            os << BLUE(" == ") << *_machine._output->car();
        }
    }

   private:
    const value_machine_op* _op;
    const vector<value_pair*> _args;
    const shared_ptr<code_perform> _code;
};

class machine::instruction_branch : public value_instruction {
   public:
    instruction_branch(machine& machine, const shared_ptr<code_branch>& code)
        : value_instruction(machine),
          _label(machine._get_label(code->label())),
          _op(machine._get_op(code->op())),
          _args(machine._tokens_to_args(code->args())),
          _code(code) {}

    void execute() const override {
        auto result = _machine._call_op(_op, _args);

        if (result->type() == value_t::error) {
            // halt the program
            _machine._output->car(result, false);
            _machine._move_pc_to_end();
        } else if (*result) {
            // jump to the label
            _machine._move_pc(_label->car());
        } else {
            // move the pc to the next
            _machine._advance_pc();
        }
    }

    void trace_before(ostream& os) const override {
        os << "(" << BLUE("branch");
        os << " (label " << _code->label() << ")";  // label
        os << " (op " << _code->op() << ")";        // op

        size_t i = 0;
        for (auto& t : _code->args()) {
            os << " " << t;
            if (t.type() == token_t::reg) {
                annotate_register(os, _args[i]);  // argument
            }
            ++i;
        }

        os << ")";
    }

    void trace_after(ostream& os) const override {
        os << BLUE(" -> ");
        if (_machine._output->car()->type() == value_t::error) {
            // error ocurred
            os << *_machine._output->car();
        } else if (_machine._pc == _label->car().get()) {
            // test has passed
            os << GREEN("yes");
        } else {
            // test has failed
            os << RED("no");
        }
    }

   private:
    const value_pair* _label;
    const value_machine_op* _op;
    const vector<value_pair*> _args;
    const shared_ptr<code_branch> _code;
};

class machine::instruction_goto : public value_instruction {
   public:
    instruction_goto(machine& machine, const shared_ptr<code_goto>& code)
        : value_instruction(machine),
          _target(machine._token_to_arg(code->target())),
          _code(code) {}

    void execute() const override {
        // jump to the target: label or register
        _machine._move_pc(_target->car());
    }

    void trace_before(ostream& os) const override {
        os << "(" << BLUE("goto") " ";
        os << _code->target();  // target
        os << ")";
    }

    void trace_after(ostream& os) const override {
    }

   private:
    const value_pair* _target;
    const shared_ptr<code_goto> _code;
};

class machine::instruction_save : public value_instruction {
   public:
    instruction_save(machine& machine, const shared_ptr<code_save>& code)
        : value_instruction(machine),
          _reg(machine._get_register(code->reg())),
          _code(code) {}

    void execute() const override {
        // save the register's content
        _machine._push_to_stack(_reg->car());
        _machine._advance_pc();
    }

    void trace_before(ostream& os) const override {
        os << "(" << BLUE("save") " ";
        os << _code->reg();  // register
        os << ")";
    }

    void trace_after(ostream& os) const override {
        os << BLUE(" >> ");
        if (_reg->car()->type() == value_t::pair &&
            _reg->pcar()->car()->type() == value_t::instruction) {
            // code saved
            os << "<code>";
        } else {
            os << *_reg->car();
        }
    }

   private:
    const value_pair* _reg;
    const shared_ptr<code_save> _code;
};

class machine::instruction_restore : public value_instruction {
   public:
    instruction_restore(machine& machine, const shared_ptr<code_restore>& code)
        : value_instruction(machine),
          _reg(machine._get_register(code->reg())),
          _code(code) {}

    void execute() const override {
        // restore the register's content
        _reg->car(_machine._pop_from_stack(), false);
        _machine._advance_pc();
    }

    void trace_before(ostream& os) const override {
        os << "(" << BLUE("restore") " ";
        os << _code->reg();  // register
        os << ")";
    }

    void trace_after(ostream& os) const override {
        os << BLUE(" << ");
        if (_reg->car()->type() == value_t::pair &&
            _reg->pcar()->car()->type() == value_t::instruction) {
            // code restored
            os << "<code>";
        } else {
            os << *_reg->car();
        }
    }

   private:
    value_pair* _reg;
    const shared_ptr<code_restore> _code;
};

// machine

value_pair* machine::_get_constant(const shared_ptr<value>& val) {
    // create and return a new constant
    _constants = make_vpair(val, _constants);
    return _constants.get();
}

value_pair* machine::_get_register(const string& name) {
    auto iter = _register_map.find(name);
    if (iter != _register_map.end()) {
        // return existing register
        return iter->second;
    } else {
        // create a new register with a nil
        _registers = make_vpair(nil, _registers);
        _register_map[name] = _registers.get();
        return _registers.get();
    }
}

value_pair* machine::_get_label(const string& name) {
    auto iter = _label_map.find(name);
    if (iter != _label_map.end()) {
        // return existing label
        return iter->second;
    } else {
        // create a new label pointing to nil
        _labels = make_vpair(nil, _labels);
        _label_map[name] = _labels.get();
        return _labels.get();
    }
}

machine::value_machine_op* machine::_get_op(const string& name) {
    auto iter = _op_map.find(name);
    if (iter != _op_map.end()) {
        // return existing op
        return iter->second;
    } else {
        // create a new unbound op
        auto op = make_shared<value_machine_op>(name);
        _ops = make_vpair(op, _ops);
        _op_map[name] = op.get();
        return op.get();
    }
}

value_pair* machine::_token_to_arg(const token& t) {
    // convert code token to a machine arg
    switch (t.type()) {
        case token_t::reg:
            return _get_register(t.name());
        case token_t::label:
            return _get_label(t.name());
        case token_t::const_:
            return _get_constant(t.val());
        default:
            throw machine_error("illegal token: %s", t.str().c_str());
    }
}

const vector<value_pair*> machine::_tokens_to_args(const vector<token>& tokens) {
    vector<value_pair*> result;

    for (auto& t : tokens) {
        // create an arg for every token
        result.push_back(_token_to_arg(t));
    }

    return result;
}

shared_ptr<machine::value_instruction> machine::_make_instruction(const shared_ptr<code>& line) {
    switch (line->type()) {
        case code_t::assign_call:
            return make_shared<instruction_assign_call>(*this, to_sptr<code_assign_call>(line));
        case code_t::assign_copy:
            return make_shared<instruction_assign_copy>(*this, to_sptr<code_assign_copy>(line));
        case code_t::perform:
            return make_shared<instruction_perform>(*this, to_sptr<code_perform>(line));
        case code_t::branch:
            return make_shared<instruction_branch>(*this, to_sptr<code_branch>(line));
        case code_t::goto_:
            return make_shared<instruction_goto>(*this, to_sptr<code_goto>(line));
        case code_t::save:
            return make_shared<instruction_save>(*this, to_sptr<code_save>(line));
        case code_t::restore:
            return make_shared<instruction_restore>(*this, to_sptr<code_restore>(line));
        default:
            throw machine_error(
                "can't create an instruction from '%s'",
                line->str().c_str());
    }
}

value_pair* machine::_append_code(const vector<shared_ptr<code>>& code) {
    shared_ptr<value_pair> head{nullptr};
    shared_ptr<value_pair> tail{nullptr};

    vector<string> label_queue;
    for (const auto& line : code) {
        if (line->type() == code_t::label) {
            // label: add to the queue and go the the next line
            label_queue.push_back(to_sptr<code_label>(line)->label());
            continue;
        }

        // make an instruction and add it to a new pair (to be appended to the code)
        shared_ptr<value_pair> new_pair = make_vpair(_make_instruction(line), nil);

        if (!head) {
            head = new_pair;  // first (non-label) line of the code
        } else {
            tail->cdr(new_pair);  // append to the tail
        }
        tail = new_pair;  // set the new tail

        if (!label_queue.empty()) {
            // point the labels in the queued to
            // the following instruction (new_pair)
            // and clear the queue
            for (const auto& label_str : label_queue) {
                _get_label(label_str)->car(new_pair);
            }
            label_queue.clear();
        }
    }

    if (!head) {
        // there was no (non-label) code to init the head
        throw machine_error("can't append empty code");
    }

    if (!label_queue.empty()) {
        // if there are still queued labels,
        // point them to nil (end of the program)
        for (const auto& label_str : label_queue) {
            _get_label(label_str)->car(nil);
        }
    }

    if (_code_head == nil) {
        _code_head = head;  // first code of the machine
    } else {
        _code_tail->cdr(head);  // append to the code tail
    }
    _code_tail = tail;  // set the new code tail

    return head.get();
}

machine::~machine() {
    // cleanup registers, labels, etc. before implicit destruction
    for (auto p = _registers; p != nil; p = to_sptr<value_pair>(p->cdr())) p->car(nil);
    for (auto p = _labels; p != nil; p = to_sptr<value_pair>(p->cdr())) p->car(nil);
    for (auto p = _constants; p != nil; p = to_sptr<value_pair>(p->cdr())) p->car(nil);
    for (auto p = _ops; p != nil; p = to_sptr<value_pair>(p->cdr())) p->car(nil);
    for (auto p = _code_head; p != nil; p = to_sptr<value_pair>(p->cdr())) p->car(nil);

    _stack.clear();
}

shared_ptr<value> machine::run(const vector<pair<string, shared_ptr<value>>>& inputs, const string& output_register) {
    // write the inputs one by one
    // to the designated registers
    for (const auto& [input_register, v] : inputs) {
        write_to(input_register, v);
    }

    _stack.clear();                            // clear the stack
    _move_pc_to_beginning();                   // set the pc to program start
    _output = _get_register(output_register);  // define the output register
    _output->car(nil);                         // reset the output register
    _counter = 0;                              // reset the instruction counter

    if (_trace != machine_trace::code) {
        // without code tracing
        while (_pc != nilptr) {
            // execution of the instruction moves the pc
            (to_ptr<const value_instruction>(_pc->car()))->execute();
        }
    } else {
        // with code tracing
        ios_base::sync_with_stdio(false);
        while (_pc != nilptr) {
            auto instruction = to_ptr<const value_instruction>(_pc->car());

            _trace_before(cout, instruction);
            instruction->execute();
            _trace_after(cout, instruction);
        }
        ios_base::sync_with_stdio(true);
    }

    // read and return the output
    // from the designated register
    return read_from(output_register);
}
