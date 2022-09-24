#ifndef VALUE_HPP_
#define VALUE_HPP_

#include <cassert>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>

#include "constants.hpp"
#include "exception.hpp"

// value hierarchy

enum class value_t {
    number,
    symbol,
    string,
    format,
    error,
    info,
    bool_,
    nil,
    pair,
    custom,
};

using std::enable_if;
using std::forward;
using std::forward_iterator_tag;
using std::is_base_of;
using std::is_convertible;
using std::make_shared;
using std::ostream;
using std::ostringstream;
using std::reinterpret_pointer_cast;
using std::shared_ptr;
using std::string;

class value {
   public:
    virtual ~value() {}  // virtual destructor

    // write the value to a stream (pure virtual)
    virtual ostream& write(ostream& os) const = 0;

    // convert to string
    string str() const {
        ostringstream s;
        write(s);

        return s.str();
    }

    // equals to another value?
    virtual bool equals(const value& other) const {
        return this == &other;
    }

    value_t type() const { return _type; }

    explicit operator bool() const;

   protected:
    value(value_t type) : _type{type} {}

    // for efficient type checking
    value_t _type;
};

class value_number : public value {
   public:
    value_number(double number) : value(value_t::number), _number(number) {}

    double number() const { return _number; }
    void number(double number) { _number = number; }

    ostream& write(ostream& os) const override;
    bool equals(const value& other) const override;

   private:
    double _number;
};

class value_symbol : public value {
   public:
    // singleton instance getter
    static const shared_ptr<value_symbol>& get(const string& symbol);

    // getter only
    const string& symbol() const { return _symbol; }

    // can't copy or move a singleton
    value_symbol(const value_symbol&) = delete;
    void operator=(value_symbol const&) = delete;
    value_symbol(value_symbol&&) = delete;
    void operator=(value_symbol&&) = delete;

    ostream& write(ostream& os) const override;

   private:
    // can't instantiate a singleton
    value_symbol(const string symbol) : value(value_t::symbol), _symbol(symbol) {}

    string _symbol;
};

class value_string : public value {
   public:
    value_string(const string& string) : value(value_t::string), _string(string) {}

    const string& string() const { return _string; }
    void string(const std::string& string) { _string = string; }

    ostream& write(ostream& os) const override;
    bool equals(const value& other) const override;

   protected:
    value_string(value_t type) : value(type) {}

    std::string _string;
};

class value_format : public value_string {
   public:
    // poor man's format
    template <typename... Args>
    value_format(value_t type, const char* format, Args&&... args) : value_string(type) {
        static char buffer[65536];

        if constexpr (sizeof...(args) > 0) {
            snprintf(buffer, sizeof(buffer), format, forward<Args>(args)...);
        } else {
            snprintf(buffer, sizeof(buffer), "%s", format);
        }

        _string = buffer;
    }

    template <typename... Args>
    value_format(const char* format, Args&&... args)
        : value_format(value_t::format, format, forward<Args>(args)...) {}
};

class value_error : public value_format {
   public:
    template <typename... Args>
    value_error(const char* format, Args&&... args)
        : value_format(value_t::error, format, forward<Args>(args)...) {}

    ostream& write(ostream& os) const override;
};

class value_info : public value_format {
   public:
    template <typename... Args>
    value_info(const char* format, Args&&... args)
        : value_format(value_t::info, format, forward<Args>(args)...) {}

    ostream& write(ostream& os) const override;
};

class value_bool : public value {
   public:
    // singleton instance getter
    static const shared_ptr<value_bool>& get(bool truth);

    // getter only
    bool truth() const { return _truth; }

    // can't copy or move a singleton
    value_bool(const value_bool&) = delete;
    void operator=(value_bool const&) = delete;
    value_bool(value_bool&&) = delete;
    void operator=(value_bool&&) = delete;

    ostream& write(ostream& os) const override;

   private:
    // can't instantiate a singleton
    value_bool(bool truth) : value(value_t::bool_), _truth(truth) {}

    bool _truth;
};

// singleton bools
const shared_ptr<value_bool> true_ = value_bool::get(true);
const shared_ptr<value_bool> false_ = value_bool::get(false);

class value_pair : public value {
   public:
    struct iterator {
       public:
        // iterator traits
        using iterator_category = forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using value_type = value;
        using pointer = shared_ptr<value>;
        using reference = value&;

        iterator(value_pair* ptr) : _ptr(ptr) {}

        reference operator*() const {
            return *(_at_cdr ? _ptr->_cdr : _ptr->_car);
        }

        pointer operator->() {
            return _at_cdr ? _ptr->_cdr : _ptr->_car;
        }

        iterator& operator++() {
            _advance();
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            _advance();
            return tmp;
        }

        friend bool operator==(const iterator& a, const iterator& b) {
            return a._ptr == b._ptr && a._at_cdr == b._at_cdr;
        };

        friend bool operator!=(const iterator& a, const iterator& b) {
            return a._ptr != b._ptr || a._at_cdr != b._at_cdr;
        };

        const shared_ptr<value>& ptr() const {
            return _at_cdr ? _ptr->_cdr : _ptr->_car;
        }

        void ptr(const shared_ptr<value>& p) {
            (_at_cdr ? _ptr->_cdr : _ptr->_car) = p;
        }

       private:
        // move the iterator
        // if the terminal cdr is not a pair,
        // return it before terminating
        void _advance();

        value_pair* _ptr;
        bool _at_cdr{false};
    };

    // car and cdr shared ptrs are passed by copying
    value_pair(
        value_t type,
        const shared_ptr<value>& car,
        const shared_ptr<value>& cdr)
        : value(type), _car(car), _cdr(cdr) {}
    value_pair(
        const shared_ptr<value>& car,
        const shared_ptr<value>& cdr)
        : value_pair(value_t::pair, car, cdr) {}

    virtual iterator begin() {
        return iterator(this);
    }

    iterator end() {
        return iterator(nullptr);
    }

    // getters
    const shared_ptr<value>& car() const { return _car; }
    const shared_ptr<value>& cdr() const { return _cdr; }

    // pair getters
    const shared_ptr<value_pair> pcar() const {
        return reinterpret_pointer_cast<value_pair>(_car);
    }
    const shared_ptr<value_pair> pcdr() const {
        return reinterpret_pointer_cast<value_pair>(_cdr);
    }

    // setters
    void car(shared_ptr<value> car) {
        _throw_on_cycle_from(car);
        _car = move(car);
    }
    void cdr(shared_ptr<value> cdr) {
        _throw_on_cycle_from(cdr);
        _cdr = move(cdr);
    }

    ostream& write(ostream& os) const override;
    bool equals(const value& other) const override;

    virtual bool is_list() const;
    virtual size_t length() const;

   private:
    void _throw_on_cycle_from(const shared_ptr<value>& other);

    shared_ptr<value> _car;
    shared_ptr<value> _cdr;
};

class value_nil : public value_pair {
   public:
    // singleton instance getter
    static const shared_ptr<value_nil>& get();

    // can't copy or move a singleton
    value_nil(const value_nil&) = delete;
    void operator=(value_nil const&) = delete;
    value_nil(value_nil&&) = delete;
    void operator=(value_nil&&) = delete;

    iterator begin() override {
        return iterator(nullptr);
    }

    ostream& write(ostream& os) const override {
        // the empty list notation
        return (os << "()");
    };

    bool equals(const value& other) const override {
        // pointer-based comparision
        return this == &other;
    }

    bool is_list() const override {
        // still a list
        return true;
    }

    size_t length() const override {
        // empty list
        return 0;
    }

   private:
    // can't instantiate a singleton
    value_nil() : value_pair(value_t::nil, nullptr, nullptr) {}
};

// singleton nil
const shared_ptr<value_pair> nil = value_nil::get();

// factory functions

template <typename T,
          typename enable_if<
              is_convertible<T, double>::value,
              bool>::type = true>  // poor man's concept
inline shared_ptr<value_number> make_value(T number) {
    // from the number
    return make_shared<value_number>(number);
}

template <typename T,
          typename enable_if<
              is_convertible<T, double>::value,
              bool>::type = true>  // poor man's concept
inline shared_ptr<value_number> make_number(T number) {
    return make_value(number);
}

inline const shared_ptr<value_symbol>& make_value(const char* symbol) {
    // from the singleton table
    return value_symbol::get(symbol);
}

inline const shared_ptr<value_symbol>& make_symbol(const char* symbol) {
    return make_value(symbol);
}

inline const shared_ptr<value_symbol>& make_symbol(const string& symbol) {
    return make_value(symbol.c_str());
}

inline shared_ptr<value_string> make_value(const string& string) {
    // from the string
    return make_shared<value_string>(string);
}

inline shared_ptr<value_string> make_string(const char* string) {
    return make_value(std::string(string));
}

inline shared_ptr<value_string> make_string(const string& string) {
    return make_value(string);
}

inline const shared_ptr<value_bool>& make_value(bool truth) {
    // from the singletons
    return value_bool::get(truth);
}

inline const shared_ptr<value_bool>& make_bool(bool truth) {
    return make_value(truth);
}

inline const shared_ptr<value_nil>& make_nil() {
    // from the singleton
    return value_nil::get();
}

inline shared_ptr<value>& make_value(shared_ptr<value>& val) {
    // from lvalue reference: identity
    return val;
}

inline shared_ptr<value>& make_value(shared_ptr<value>&& val) {
    // from rvalue reference: identity
    return val;
}

template <typename T1, typename T2>
shared_ptr<value_pair> make_vpair(T1&& car, T2&& cdr) {
    // from two separate universal references: car and cdr
    return make_shared<value_pair>(
        make_value(forward<T1>(car)),
        make_value(forward<T2>(cdr)));
}

template <typename Head, typename... Tail>
shared_ptr<value_pair> make_list(Head&& first, Tail&&... rest) {
    if constexpr (sizeof...(rest) > 0) {
        return make_vpair(
            make_value(forward<Head>(first)),    // the head
            make_list(forward<Tail>(rest)...));  // the rest
    } else {
        return make_vpair(
            make_value(forward<Head>(first)),  // the tail
            nil);                              // terminating nil
    }
}

template <typename... Args>
inline shared_ptr<value_error> make_error(const char* format, Args... args) {
    return make_shared<value_error>(format, forward<Args>(args)...);
}

template <typename... Args>
inline shared_ptr<value_info> make_info(const char* format, Args... args) {
    return make_shared<value_info>(format, forward<Args>(args)...);
}

// helper functions

inline ostream& operator<<(ostream& os, const value& v) {
    return v.write(os);
}

inline bool operator==(const value& v1, const value& v2) {
    return (&v1 == &v2 || v1.equals(v2));
}

inline bool operator!=(const value& v1, const value& v2) {
    return (&v1 != &v2 && !v1.equals(v2));
}

ostream& operator<<(ostream& os, const value_t& t);

template <typename T,
          typename enable_if<
              is_base_of<value, T>::value,
              bool>::type = true>  // poor man's concept
inline shared_ptr<T> to(const shared_ptr<value>& v) {
    return reinterpret_pointer_cast<T>(v);
}

// exceptions

class cycle_error : public str_exception {
   public:
    cycle_error(const value_pair* from)
        : str_exception(_make_message(from)) {}

   private:
    string _make_message(const value_pair* from);
};

#endif  // VALUE_HPP_
