#ifndef CODE_HPP_
#define CODE_HPP_

#include <cassert>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "value.hpp"

enum class token_t {
    op,
    reg,
    label,
    const_,
};

enum class code_t {
    label,
    assign_call,
    assign_copy,
    perform,
    branch,
    goto_,
    save,
    restore,
};

// token

class token {
   public:
    token(token_t type, const std::string& name) : _type(type), _name(name) {
        assert(type == token_t::op || type == token_t::reg || type == token_t::label);
    }
    token(token_t type, const std::shared_ptr<value>& val) : _type(type), _val(val) {
        assert(type == token_t::const_);
    }
    token(const std::shared_ptr<value>& v);

    token_t type() const { return _type; }

    const std::string& name() const {
        assert(_type == token_t::op || _type == token_t::reg || _type == token_t::label);
        return _name;
    }

    const std::shared_ptr<value>& val() const {
        assert(_type == token_t::const_);
        return _val;
    }

    std::shared_ptr<value> to_value() const;

    // for temporarily undefined token
    friend class code_assign_copy;
    friend class code_goto;

   private:
    // for friends
    token() {}

    token_t _type;
    std::string _name;
    std::shared_ptr<value> _val{nullptr};
};

// code hierarchy

class code {
   public:
    virtual ~code() {}  // virtual destructor

    code_t type() const { return _type; }

    virtual std::shared_ptr<value> to_value() const = 0;

    virtual std::ostream& write(std::ostream& os) const {
        return (os << "    " << *to_value());
    };

    std::string str() const {
        std::ostringstream s;
        write(s);

        return s.str();
    }

   protected:
    code(code_t type) : _type(type) {}

    code_t _type;
};

class code_label : public code {
   public:
    code_label(const std::string& label)
        : code(code_t::label), _label{label} {}
    code_label(const std::shared_ptr<value_symbol>& v);

    const std::string& label() const { return _label; }

    virtual std::shared_ptr<value> to_value() const override;

    virtual std::ostream& write(std::ostream& os) const override {
        return (os << _label);
    };

   private:
    std::string _label;
};

class code_assign_call : public code {
   public:
    code_assign_call(
        const std::string& reg,
        const std::string& op,
        std::initializer_list<token> args)
        : code(code_t::assign_call), _reg{reg}, _op{op}, _args{args} {}
    code_assign_call(const std::shared_ptr<value_pair>& v);

    const std::string& reg() const { return _reg; }
    const std::string& op() const { return _op; }
    std::vector<token>& args() { return _args; }

    virtual std::shared_ptr<value> to_value() const override;

   private:
    std::string _reg;
    std::string _op;
    std::vector<token> _args;
};

class code_assign_copy : public code {
   public:
    code_assign_copy(const std::string& reg, token src)
        : code(code_t::assign_copy), _reg{reg}, _src{src} {}
    code_assign_copy(const std::shared_ptr<value_pair>& v);

    const std::string& reg() const { return _reg; }
    const token& src() const { return _src; }

    virtual std::shared_ptr<value> to_value() const override;

   private:
    std::string _reg;
    token _src;
};

class code_perform : public code {
   public:
    code_perform(const std::string& op, std::initializer_list<token> args)
        : code(code_t::assign_call), _op{op}, _args{args} {}
    code_perform(const std::shared_ptr<value_pair>& v);

    const std::string& op() const { return _op; }
    std::vector<token>& args() { return _args; }

    virtual std::shared_ptr<value> to_value() const override;

   private:
    std::string _op;
    std::vector<token> _args;
};

class code_branch : public code {
   public:
    code_branch(
        const std::string& label,
        const std::string& op,
        std::initializer_list<token> args)
        : code(code_t::branch), _label{label}, _op{op}, _args{args} {}
    code_branch(const std::shared_ptr<value_pair>& v);

    const std::string& label() const { return _label; }
    const std::string& op() const { return _op; }
    std::vector<token>& args() { return _args; }

    virtual std::shared_ptr<value> to_value() const override;

   private:
    std::string _label;
    std::string _op;
    std::vector<token> _args;
};

class code_goto : public code {
   public:
    code_goto(token target)
        : code(code_t::goto_), _target{target} {}
    code_goto(const std::shared_ptr<value_pair>& v);

    const token& target() const { return _target; }

    virtual std::shared_ptr<value> to_value() const override;

   private:
    token _target;
};

class code_save : public code {
   public:
    code_save(const std::string& reg)
        : code(code_t::save), _reg{reg} {}

    code_save(const std::shared_ptr<value_pair>& v);

    const std::string& reg() const { return _reg; }

    virtual std::shared_ptr<value> to_value() const override;

   private:
    std::string _reg;
};

class code_restore : public code {
   public:
    code_restore(const std::string& reg)
        : code(code_t::restore), _reg{reg} {}

    code_restore(const std::shared_ptr<value_pair>& v);

    const std::string& reg() const { return _reg; }

    virtual std::shared_ptr<value> to_value() const override;

   private:
    std::string _reg;
};

// helper functions

std::vector<std::unique_ptr<code>> translate_to_code(const std::shared_ptr<value>& source);

inline std::ostream& operator<<(std::ostream& os, const code& c) {
    return c.write(os);
}

// exceptions

class code_error : public format_exception {
   public:
    using format_exception::format_exception;
};

#endif  // CODE_HPP_
