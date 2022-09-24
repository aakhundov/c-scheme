#include "machine.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "code.hpp"
#include "value.hpp"

using std::make_shared;
using std::pair;
using std::shared_ptr;
using std::string;
using std::vector;

// instruction hierarchy

class machine::instruction_assign_call : public value_instruction {
   public:
    instruction_assign_call(machine& machine, const shared_ptr<code_assign_call>& code)
        : value_instruction(machine, code),
          _reg(machine._get_register(code->reg())),
          _op(machine._get_op(code->op())),
          _args(machine._tokens_to_args(code->args())) {}

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

   private:
    const shared_ptr<value_pair> _reg;
    const shared_ptr<value_machine_op> _op;
    const vector<shared_ptr<value_pair>> _args;
};

class machine::instruction_assign_copy : public value_instruction {
   public:
    instruction_assign_copy(machine& machine, const shared_ptr<code_assign_copy>& code)
        : value_instruction(machine, code),
          _reg(machine._get_register(code->reg())),
          _src(machine._token_to_arg(code->src())) {}

    void execute() const override {
        // assign from source
        _reg->car(_src->car());
        _machine._advance_pc();
    }

   private:
    const shared_ptr<value_pair> _reg;
    const shared_ptr<value_pair> _src;
};

class machine::instruction_perform : public value_instruction {
   public:
    instruction_perform(machine& machine, const shared_ptr<code_perform>& code)
        : value_instruction(machine, code),
          _op(machine._get_op(code->op())),
          _args(machine._tokens_to_args(code->args())) {}

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

   private:
    const shared_ptr<value_machine_op> _op;
    const vector<shared_ptr<value_pair>> _args;
};

class machine::instruction_branch : public value_instruction {
   public:
    instruction_branch(machine& machine, const shared_ptr<code_branch>& code)
        : value_instruction(machine, code),
          _label(machine._get_label(code->label())),
          _op(machine._get_op(code->op())),
          _args(machine._tokens_to_args(code->args())) {}

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

   private:
    const shared_ptr<value_pair> _label;
    const shared_ptr<value_machine_op> _op;
    const vector<shared_ptr<value_pair>> _args;
};

class machine::instruction_goto : public value_instruction {
   public:
    instruction_goto(machine& machine, const shared_ptr<code_goto>& code)
        : value_instruction(machine, code),
          _target{machine._token_to_arg(code->target())} {}

    void execute() const override {
        // jump to the target: label or register
        _machine._pc = _target->pcar();
    }

   private:
    const shared_ptr<value_pair> _target;
};

class machine::instruction_save : public value_instruction {
   public:
    instruction_save(machine& machine, const shared_ptr<code_save>& code)
        : value_instruction(machine, code),
          _reg{machine._get_register(code->reg())} {}

    void execute() const override {
        // save the register's content
        _machine._push_to_stack(_reg->car());
        _machine._advance_pc();
    }

   private:
    const shared_ptr<value_pair> _reg;
};

class machine::instruction_restore : public value_instruction {
   public:
    instruction_restore(machine& machine, const shared_ptr<code_restore>& code)
        : value_instruction(machine, code),
          _reg{machine._get_register(code->reg())} {}

    void execute() const override {
        // restore the register's content
        _reg->car(_machine._pop_from_stack());
        _machine._advance_pc();
    }

   private:
    const shared_ptr<value_pair> _reg;
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
            // point the labels in the queued  o
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

machine::machine(const vector<shared_ptr<code>>& code) {
    _append_code(code);
}

void machine::bind_op(const string& name, machine_op const op) {
    _get_op(name)->op(op);
}

shared_ptr<value> machine::read_from_register(const string& name) {
    return _get_register(name)->car();
}

void machine::write_to_register(const string& name, const shared_ptr<value>& v) {
    _get_register(name)->car(v);
}

void machine::append_and_jump(const vector<shared_ptr<code>>& code) {
    auto code_head = _append_code(code);  // append
    _pc = code_head;                      // jump
}

shared_ptr<value> machine::run(
    const vector<pair<string, shared_ptr<value>>>& inputs,
    const string& output_register) {
    // write the inputs to the designated registers
    for (const auto& [input_register, v] : inputs) {
        write_to_register(input_register, v);
    }

    _pc = _code_head;                          // set the pc to the code head
    _output = _get_register(output_register);  // set the output register

    while (_pc != nil) {
        auto instruction = to<value_instruction>(_pc->car());
        instruction->execute();  // instruction moves the pc
    }

    // read the output from the designated register
    return read_from_register(output_register);
}
