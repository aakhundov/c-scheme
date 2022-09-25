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
using std::pair;
using std::shared_ptr;
using std::string;
using std::vector;

// instruction hierarchy

namespace {

void annotate_register(ostream& os, const shared_ptr<value_pair>& reg) {
    os << BLUE("[");
    if (reg->car()->type() == value_t::pair &&
        to<value_pair>(reg)->pcar()->car()->type() == value_t::instruction) {
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
            _machine._output->car(result);
            _machine._pc = nil;
        } else {
            // assign the result
            _reg->car(result);
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
            i++;
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
                to<value_pair>(_reg)->pcar()->car()->type() == value_t::instruction) {
                // code returned
                os << "<code>";
            } else {
                os << *_reg->car();
            }
        }
    }

   private:
    const shared_ptr<value_pair> _reg;
    const shared_ptr<value_machine_op> _op;
    const vector<shared_ptr<value_pair>> _args;
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
        _reg->car(_src->car());
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
    const shared_ptr<value_pair> _reg;
    const shared_ptr<value_pair> _src;
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
            _machine._output->car(result);
            _machine._pc = nil;
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
            i++;
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
    const shared_ptr<value_machine_op> _op;
    const vector<shared_ptr<value_pair>> _args;
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
            _machine._output->car(result);
            _machine._pc = nil;
        } else if (*result) {
            // jump to the label
            _machine._pc = _label->pcar();
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
            i++;
        }

        os << ")";
    }

    void trace_after(ostream& os) const override {
        os << BLUE(" -> ");
        if (_machine._output->car()->type() == value_t::error) {
            // error ocurred
            os << *_machine._output->car();
        } else if (_machine._pc == _label->car()) {
            // test has passed
            os << GREEN("yes");
        } else {
            // test has failed
            os << RED("no");
        }
    }

   private:
    const shared_ptr<value_pair> _label;
    const shared_ptr<value_machine_op> _op;
    const vector<shared_ptr<value_pair>> _args;
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
        _machine._pc = _target->pcar();
    }

    void trace_before(ostream& os) const override {
        os << "(" << BLUE("goto") " ";
        os << _code->target();  // target
        os << ")";
    }

    void trace_after(ostream& os) const override {
    }

   private:
    const shared_ptr<value_pair> _target;
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
            to<value_pair>(_reg)->pcar()->car()->type() == value_t::instruction) {
            // code saved
            os << "<code>";
        } else {
            os << *_reg->car();
        }
    }

   private:
    const shared_ptr<value_pair> _reg;
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
        _reg->car(_machine._pop_from_stack());
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
            to<value_pair>(_reg)->pcar()->car()->type() == value_t::instruction) {
            // code restored
            os << "<code>";
        } else {
            os << *_reg->car();
        }
    }

   private:
    const shared_ptr<value_pair> _reg;
    const shared_ptr<code_restore> _code;
};

// machine

shared_ptr<value_pair> machine::_get_constant(shared_ptr<value> val) {
    // create and return a new constant
    _constants = make_vpair(val, _constants);
    return _constants;
}

shared_ptr<value_pair> machine::_get_register(const string& name) {
    if (_register_map.count(name)) {
        // return existing register
        return _register_map[name];
    } else {
        // create a new register with a nil
        _registers = make_vpair(nil, _registers);
        _register_map[name] = _registers;
        return _registers;
    }
}

shared_ptr<value_pair> machine::_get_label(const string& name) {
    if (_label_map.count(name)) {
        // return existing label
        return _label_map[name];
    } else {
        // create a new label pointing to nil
        _labels = make_vpair(nil, _labels);
        _label_map[name] = _labels;
        return _labels;
    }
}

shared_ptr<machine::value_machine_op> machine::_get_op(const string& name) {
    if (_op_map.count(name)) {
        // return existing op
        return _op_map[name];
    } else {
        // create a new unbound op
        auto op = make_shared<value_machine_op>(name);
        _ops = make_vpair(op, _ops);
        _op_map[name] = op;
        return op;
    }
}

const shared_ptr<value_pair> machine::_token_to_arg(const token& t) {
    // convert code token to a machine arg
    switch (t.type()) {
        case token_t::reg:
            return _get_register(t.name());
        case token_t::label:
            return _get_label(t.name());
        case token_t::const_:
            return _get_constant(t.val());
        default:
            throw machine_error(
                "illegal token type for the argument: %d",
                t.type());
    }
}

const vector<shared_ptr<value_pair>> machine::_tokens_to_args(const vector<token>& tokens) {
    vector<shared_ptr<value_pair>> result;

    for (auto& t : tokens) {
        // create an arg for every token
        result.push_back(_token_to_arg(t));
    }

    return result;
}

shared_ptr<value_pair> machine::_append_code(const vector<shared_ptr<code>>& code) {
    shared_ptr<value_pair> head{nullptr};
    shared_ptr<value_pair> tail{nullptr};

    vector<string> label_queue;
    for (const auto& line : code) {
        shared_ptr<value_instruction> instruction;
        switch (line->type()) {
            case code_t::label:
                // label: add to the queue and go the the next line
                label_queue.push_back(to<code_label>(line)->label());
                continue;
            case code_t::assign_call:
                instruction = make_shared<instruction_assign_call>(*this, to<code_assign_call>(line));
                break;
            case code_t::assign_copy:
                instruction = make_shared<instruction_assign_copy>(*this, to<code_assign_copy>(line));
                break;
            case code_t::perform:
                instruction = make_shared<instruction_perform>(*this, to<code_perform>(line));
                break;
            case code_t::branch:
                instruction = make_shared<instruction_branch>(*this, to<code_branch>(line));
                break;
            case code_t::goto_:
                instruction = make_shared<instruction_goto>(*this, to<code_goto>(line));
                break;
            case code_t::save:
                instruction = make_shared<instruction_save>(*this, to<code_save>(line));
                break;
            case code_t::restore:
                instruction = make_shared<instruction_restore>(*this, to<code_restore>(line));
                break;
        }

        // a new code pair with the new instruction to add to the chain
        shared_ptr<value_pair> new_pair = make_vpair(instruction, nil);

        if (!head) {
            head = new_pair;  // first (non-label) line of the code
        } else {
            tail->cdr(new_pair);  // append to the tail
        }
        tail = new_pair;  // set the new tail

        if (label_queue.size() > 0) {
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

    if (label_queue.size() > 0) {
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

    return head;
}

machine::~machine() {
    // cleanup registers, labels, etc. before implicit destruction
    for (auto p = _registers->begin(); p != _registers->end(); p++) p.ptr(nullptr);
    for (auto p = _labels->begin(); p != _labels->end(); p++) p.ptr(nullptr);
    for (auto p = _constants->begin(); p != _constants->end(); p++) p.ptr(nullptr);
    for (auto p = _ops->begin(); p != _ops->end(); p++) p.ptr(nullptr);
    for (auto p = _code_head->begin(); p != _code_head->end(); p++) p.ptr(nullptr);
    for (auto p = _stack->begin(); p != _stack->end(); p++) p.ptr(nullptr);
}

shared_ptr<value> machine::run(const vector<pair<string, shared_ptr<value>>>& inputs, const string& output_register) {
    // write the inputs one by one
    // to the designated registers
    for (const auto& [input_register, v] : inputs) {
        write_to_register(input_register, v);
    }

    _pc = _code_head;                          // set the pc to the code head
    _output = _get_register(output_register);  // set the output register
    _counter = 0;                              // reset the instruction counter

    if (_trace != machine_trace::code) {
        // without code tracing
        while (_pc != nil) {
            auto instruction = to<value_instruction>(_pc->car());
            instruction->execute();  // instruction moves the pc
        }
    } else {
        // with code tracing
        ios_base::sync_with_stdio(false);
        while (_pc != nil) {
            auto instruction = to<value_instruction>(_pc->car());

            _trace_before(cout, instruction);
            instruction->execute();
            _trace_after(cout, instruction);
        }
        ios_base::sync_with_stdio(true);
    }

    // read and return the output
    // from the designated register
    return read_from_register(output_register);
}
