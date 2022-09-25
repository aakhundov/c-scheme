#ifndef CODE_HPP_
#define CODE_HPP_

#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "value.hpp"

using std::get;
using std::ostream;
using std::ostringstream;
using std::reinterpret_pointer_cast;
using std::shared_ptr;
using std::string;
using std::variant;
using std::vector;

// token

enum class token_t {
    op,
    reg,
    label,
    const_,
};

class token {
   public:
    token(token_t type, const string& name) : _type(type), _content(name), _str(to_value()->str()) {
        assert(type == token_t::op || type == token_t::reg || type == token_t::label);
    }
    token(token_t type, const shared_ptr<value>& val) : _type(type), _content(val), _str(to_value()->str()) {
        assert(type == token_t::const_);
    }
    token(const shared_ptr<value>& v);

    token_t type() const { return _type; }

    const string& name() const {
        assert(_type == token_t::op || _type == token_t::reg || _type == token_t::label);
        return get<string>(_content);
    }

    const shared_ptr<value>& val() const {
        assert(_type == token_t::const_);
        return get<shared_ptr<value>>(_content);
    }

    shared_ptr<value> to_value() const;

    ostream& write(ostream& os) const {
        return (os << _str);
    };

    string str() const {
        ostringstream s;
        write(s);

        return s.str();
    }

    // for temporarily undefined token
    friend class code_assign_copy;
    friend class code_goto;

   private:
    // for friends
    token() {}

    token_t _type;
    variant<string, shared_ptr<value>> _content;
    string _str;  // string representation
};

// code hierarchy

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

class code {
   public:
    virtual ~code() {}  // virtual destructor

    code_t type() const { return _type; }

    virtual shared_ptr<value> to_value() const = 0;

    ostream& write(ostream& os) const {
        return (os << *to_value());
    };

    string str() const {
        ostringstream s;
        write(s);

        return s.str();
    }

   protected:
    code(code_t type) : _type(type) {}

    code_t _type;
};

class code_label : public code {
   public:
    code_label(const string& label)
        : code(code_t::label), _label{label} {}
    code_label(const value_symbol* v);

    const string& label() const { return _label; }

    shared_ptr<value> to_value() const override;

   private:
    string _label;
};

class code_assign_call : public code {
   public:
    code_assign_call(
        const string& reg,
        const string& op,
        const vector<token>& args)
        : code(code_t::assign_call), _reg{reg}, _op{op}, _args{args} {}
    code_assign_call(const value_pair* v);

    const string& reg() const { return _reg; }
    const string& op() const { return _op; }
    vector<token>& args() { return _args; }

    shared_ptr<value> to_value() const override;

   private:
    string _reg;
    string _op;
    vector<token> _args;
};

class code_assign_copy : public code {
   public:
    code_assign_copy(const string& reg, token src)
        : code(code_t::assign_copy), _reg{reg}, _src{src} {}
    code_assign_copy(const value_pair* v);

    const string& reg() const { return _reg; }
    const token& src() const { return _src; }

    shared_ptr<value> to_value() const override;

   private:
    string _reg;
    token _src;
};

class code_perform : public code {
   public:
    code_perform(
        const string& op,
        const vector<token>& args)
        : code(code_t::assign_call), _op{op}, _args{args} {}
    code_perform(const value_pair* v);

    const string& op() const { return _op; }
    vector<token>& args() { return _args; }

    shared_ptr<value> to_value() const override;

   private:
    string _op;
    vector<token> _args;
};

class code_branch : public code {
   public:
    code_branch(
        const string& label,
        const string& op,
        const vector<token>& args)
        : code(code_t::branch), _label{label}, _op{op}, _args{args} {}
    code_branch(const value_pair* v);

    const string& label() const { return _label; }
    const string& op() const { return _op; }
    vector<token>& args() { return _args; }

    shared_ptr<value> to_value() const override;

   private:
    string _label;
    string _op;
    vector<token> _args;
};

class code_goto : public code {
   public:
    code_goto(token target)
        : code(code_t::goto_), _target{target} {}
    code_goto(const value_pair* v);

    const token& target() const { return _target; }

    shared_ptr<value> to_value() const override;

   private:
    token _target;
};

class code_save : public code {
   public:
    code_save(const string& reg)
        : code(code_t::save), _reg{reg} {}
    code_save(const value_pair* v);

    const string& reg() const { return _reg; }

    shared_ptr<value> to_value() const override;

   private:
    string _reg;
};

class code_restore : public code {
   public:
    code_restore(const string& reg)
        : code(code_t::restore), _reg{reg} {}
    code_restore(const value_pair* v);

    const string& reg() const { return _reg; }

    shared_ptr<value> to_value() const override;

   private:
    string _reg;
};

// helper functions

vector<shared_ptr<code>> translate_to_code(const shared_ptr<value>& source);

inline ostream& operator<<(ostream& os, const token& t) {
    return t.write(os);
}

inline ostream& operator<<(ostream& os, const code& c) {
    return c.write(os);
}

template <typename T,
          typename enable_if<
              is_base_of<code, T>::value,
              bool>::type = true>  // poor man's concept
inline shared_ptr<T> to_sptr(const shared_ptr<code>& v) {
    return reinterpret_pointer_cast<T>(v);
}

// exceptions

class code_error : public format_exception {
   public:
    using format_exception::format_exception;
};

#endif  // CODE_HPP_
