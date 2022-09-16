#ifndef VALUE_HPP_
#define VALUE_HPP_

#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

#include "constants.hpp"
#include "exception.hpp"

enum class value_t {
    undefined,
    number,
    symbol,
    string,
    format,
    error,
    info,
    bool_,
    nil,
    pair,
};

// value hierarchy

class value {
   public:
    virtual ~value() {}  // virtual destructor

    // write the value to a stream (pure virtual)
    virtual std::ostream& write(std::ostream& os) const = 0;

    // equals to another value?
    virtual bool equals(const value& other) const {
        return this == &other;
    }

    // get the value's type
    value_t type() const {
        return _type;
    }

   protected:
    // for efficient type checking
    value_t _type{value_t::undefined};
};

class value_number : public value {
   public:
    value_number(double number) : _number(number) {
        _type = value_t::number;
    }

    // getter only
    double number() const { return _number; }

    std::ostream& write(std::ostream& os) const override;
    bool equals(const value& other) const override;

   private:
    double _number;
};

class value_symbol : public value {
   public:
    // singleton instance getter
    static const std::shared_ptr<value_symbol>& get(const std::string& symbol);

    // getter only
    const std::string& symbol() const { return _symbol; }

    // can't copy or move a singleton
    value_symbol(const value_symbol&) = delete;
    void operator=(value_symbol const&) = delete;
    value_symbol(value_symbol&&) = delete;
    void operator=(value_symbol&&) = delete;

    std::ostream& write(std::ostream& os) const override;

   private:
    // can't instantiate a singleton
    value_symbol(const std::string symbol) : _symbol(symbol) {
        _type = value_t::symbol;
    }

    std::string _symbol;
};

class value_string : public value {
   public:
    value_string(const std::string& string) : _string(string) {
        _type = value_t::string;
    }

    // getter only
    const std::string& string() const { return _string; }

    std::ostream& write(std::ostream& os) const override;
    bool equals(const value& other) const override;

   protected:
    value_string() {
        _type = value_t::string;
    }

    std::string _string;
};

class value_format : public value_string {
   public:
    // poor man's std::format
    value_format(const char* format, va_list args);
};

class value_error : public value_format {
   public:
    value_error(const char* format, va_list args) : value_format(format, args) {
        _type = value_t::error;
    }

    std::ostream& write(std::ostream& os) const override;
};

class value_info : public value_format {
   public:
    value_info(const char* format, va_list args) : value_format(format, args) {
        _type = value_t::info;
    }

    std::ostream& write(std::ostream& os) const override;
};

class value_bool : public value {
   public:
    // singleton instance getter
    static const std::shared_ptr<value_bool>& get(bool truth);

    // getter only
    bool truth() const { return _truth; }

    // can't copy or move a singleton
    value_bool(const value_bool&) = delete;
    void operator=(value_bool const&) = delete;
    value_bool(value_bool&&) = delete;
    void operator=(value_bool&&) = delete;

    std::ostream& write(std::ostream& os) const override;

   private:
    // can't instantiate a singleton
    value_bool(bool truth) : _truth(truth) {
        _type = value_t::bool_;
    }

    bool _truth;
};

class value_nil : public value {
   public:
    // singleton instance getter
    static const std::shared_ptr<value_nil>& get();

    // can't copy or move a singleton
    value_nil(const value_nil&) = delete;
    void operator=(value_nil const&) = delete;
    value_nil(value_nil&&) = delete;
    void operator=(value_nil&&) = delete;

    std::ostream& write(std::ostream& os) const override;

   private:
    // can't instantiate a singleton
    value_nil() {
        _type = value_t::nil;
    }
};

// singleton instances
const std::shared_ptr<value_bool> true_ = value_bool::get(true);
const std::shared_ptr<value_bool> false_ = value_bool::get(false);
const std::shared_ptr<value_nil> nil = value_nil::get();

class value_pair : public value {
   public:
    struct value_iterator {
       public:
        // iterator traits
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::shared_ptr<value>;
        using pointer = std::shared_ptr<value>*;
        using reference = std::shared_ptr<value>&;

        value_iterator(value_pair* ptr) : _ptr(ptr) {}

        reference operator*() const;
        pointer operator->();
        value_iterator& operator++();
        value_iterator operator++(int);

        friend bool operator==(const value_iterator& a, const value_iterator& b);
        friend bool operator!=(const value_iterator& a, const value_iterator& b);

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
        const std::shared_ptr<value>& car,
        const std::shared_ptr<value>& cdr)
        : _car(car), _cdr(cdr) {
        _type = value_t::pair;
    }

    value_iterator begin() {
        return value_iterator(this);
    }

    value_iterator end() {
        return value_iterator(nullptr);
    }

    // getters
    const std::shared_ptr<value>& car() const { return _car; }
    const std::shared_ptr<value>& cdr() const { return _cdr; }

    // pair getters
    const std::shared_ptr<value_pair> pcar() const {
        assert(_car->type() == value_t::pair);
        return std::reinterpret_pointer_cast<value_pair>(_car);
    }
    const std::shared_ptr<value_pair> pcdr() const {
        assert(_cdr->type() == value_t::pair);
        return std::reinterpret_pointer_cast<value_pair>(_cdr);
    }

    // setters
    void car(std::shared_ptr<value> car) {
        _throw_on_cycle_from(car);
        _car = std::move(car);
    }
    void cdr(std::shared_ptr<value> cdr) {
        _throw_on_cycle_from(cdr);
        _cdr = std::move(cdr);
    }

    std::ostream& write(std::ostream& os) const override;
    bool equals(const value& other) const override;

    bool is_list() const;
    size_t length() const;

   private:
    void _throw_on_cycle_from(const std::shared_ptr<value>& other);

    std::shared_ptr<value> _car;
    std::shared_ptr<value> _cdr;
};

// factory functions

template <typename T,
          typename std::enable_if<
              std::is_convertible<T, double>::value,
              bool>::type = true>  // poor man's concept
inline std::shared_ptr<value_number> make_value(T number) {
    // from the number
    return std::make_shared<value_number>(number);
}

template <typename T,
          typename std::enable_if<
              std::is_convertible<T, double>::value,
              bool>::type = true>  // poor man's concept
inline std::shared_ptr<value_number> make_number(T number) {
    return make_value(number);
}

inline const std::shared_ptr<value_symbol>& make_value(const char* symbol) {
    // from the singleton table
    return value_symbol::get(symbol);
}

inline const std::shared_ptr<value_symbol>& make_symbol(const char* symbol) {
    return make_value(symbol);
}

inline const std::shared_ptr<value_symbol>& make_symbol(const std::string& symbol) {
    return make_value(symbol.c_str());
}

inline std::shared_ptr<value_string> make_value(const std::string& string) {
    // from the string
    return std::make_shared<value_string>(string);
}

inline std::shared_ptr<value_string> make_string(const char* string) {
    return make_value(std::string(string));
}

inline std::shared_ptr<value_string> make_string(const std::string& string) {
    return make_value(string);
}

inline const std::shared_ptr<value_bool>& make_value(bool truth) {
    // from the singletons
    return (truth ? true_ : false_);
}

inline const std::shared_ptr<value_bool>& make_bool(bool truth) {
    return make_value(truth);
}

inline const std::shared_ptr<value_nil>& make_nil() {
    // from the singleton
    return nil;
}

inline std::shared_ptr<value>& make_value(std::shared_ptr<value>& val) {
    // from lvalue reference: identity
    return val;
}

inline std::shared_ptr<value>& make_value(std::shared_ptr<value>&& val) {
    // from rvalue reference: identity
    return val;
}

template <typename T1, typename T2>
std::shared_ptr<value_pair> make_value_pair(T1&& car, T2&& cdr) {
    // from two separate universal references: car and cdr
    return std::make_shared<value_pair>(
        make_value(std::forward<T1>(car)),
        make_value(std::forward<T2>(cdr)));
}

template <typename Head, typename... Tail>
std::shared_ptr<value_pair> make_value_list(Head&& first, Tail&&... rest) {
    if constexpr (sizeof...(rest) > 0) {
        return make_value_pair(
            make_value(std::forward<Head>(first)),          // the head
            make_value_list(std::forward<Tail>(rest)...));  // the rest
    } else {
        return make_value_pair(
            make_value(std::forward<Head>(first)),  // the tail
            nil);                                   // terminating nil
    }
}

std::shared_ptr<value_error> make_error(const char* format, ...);
std::shared_ptr<value_info> make_info(const char* format, ...);

// helper functions

inline std::ostream& operator<<(std::ostream& os, const value& v) {
    return v.write(os);
}

inline bool operator==(const value& v1, const value& v2) {
    return (&v1 == &v2 || v1.equals(v2));
}

inline bool operator!=(const value& v1, const value& v2) {
    return (&v1 != &v2 && !v1.equals(v2));
}

std::ostream& operator<<(std::ostream& os, const value_t& t);

// exceptions

class cycle_error : public str_exception {
   public:
    cycle_error(const value_pair* from)
        : str_exception(_make_message(from)) {}

   private:
    std::string _make_message(const value_pair* from);
};

#endif  // VALUE_HPP_
