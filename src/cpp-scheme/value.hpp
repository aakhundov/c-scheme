#ifndef VALUE_HPP_
#define VALUE_HPP_

#include <cstdarg>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

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
    value() {}           // default constructor
    virtual ~value() {}  // virtual destructor

    virtual std::ostream& write(std::ostream& os) const {
        // the default
        return (os << "value");
    };

    value_t type() {
        return _type;
    }

   protected:
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

    std::ostream& write(std::ostream& os) const override {
        // the symbol as is
        return (os << _symbol);
    }

   private:
    // can't instantiate a singleton
    value_symbol(const std::string symbol) : _symbol(symbol) {
        _type = value_t::symbol;
    }

    std::string _symbol;
};

class value_string : public value {
   public:
    value_string() : _string("") {
        _type = value_t::string;
    }
    value_string(std::string string) : _string(string) {
        _type = value_t::string;
    }

    // getter only
    const std::string& string() const { return _string; }

    std::ostream& write(std::ostream& os) const override {
        // the string in quotes
        return (os << "\"" << _string << "\"");
    }

   protected:
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

    std::ostream& write(std::ostream& os) const override {
        // the red/white and bold error text
        return (os << "\x1B[1;31merror:\x1B[1;37m " << _string << "\x1B[0m");
    }
};

class value_info : public value_format {
   public:
    value_info(const char* format, va_list args) : value_format(format, args) {
        _type = value_t::info;
    }

    std::ostream& write(std::ostream& os) const override {
        // the green info text
        return (os << "\x1B[32m" << _string << "\x1B[0m");
    }
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

    std::ostream& write(std::ostream& os) const override {
        // the corresponding bool literal
        return (os << (_truth ? "true" : "false"));
    }

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

    std::ostream& write(std::ostream& os) const override {
        // the empty list notation for the nil
        return (os << "()");
    };

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

        // stores a value_pair* internally
        value_iterator(value_pair* ptr) : _ptr(ptr) {}

        reference operator*() const {
            // shared ptr to the car
            return (_at_cdr ? _ptr->_cdr : _ptr->_car);
        }

        pointer operator->() {
            // reference to a shared ptr to the car
            return &(_at_cdr ? _ptr->_cdr : _ptr->_car);
        }

        value_iterator& operator++() {
            _advance();

            return *this;
        }

        value_iterator operator++(int) {
            value_iterator tmp = *this;

            _advance();

            return tmp;
        }

        friend bool operator==(const value_iterator& a, const value_iterator b) {
            return a._ptr == b._ptr && a._at_cdr == b._at_cdr;
        };

        friend bool operator!=(const value_iterator& a, const value_iterator b) {
            return a._ptr != b._ptr || a._at_cdr != b._at_cdr;
        };

       private:
        // move the iterator
        // if the terminal cdr is not a pair,
        // return it before terminating
        void _advance();

        value_pair* _ptr;
        bool _at_cdr{false};
    };

    // car and cdr shared ptrs are passed by copying
    value_pair(std::shared_ptr<value> car, std::shared_ptr<value> cdr) : _car(car), _cdr(cdr) {
        _type = value_t::pair;
    }

    value_iterator begin() {
        return value_iterator(this);
    }

    value_iterator end() {
        return value_iterator(nullptr);
    }

    // getters and setters
    const std::shared_ptr<value>& car() const { return _car; }
    void car(std::shared_ptr<value>& car) { _car = car; }
    const std::shared_ptr<value>& cdr() const { return _cdr; }
    void cdr(std::shared_ptr<value>& cdr) { _cdr = cdr; }

    std::ostream& write(std::ostream& os) const override;

    bool is_list();
    size_t length();

   private:
    std::shared_ptr<value> _car;
    std::shared_ptr<value> _cdr;
};

// factory functions

template <typename T,
          typename std::enable_if<
              std::is_arithmetic<T>::value,
              bool>::type = true>  // poor man's concept
inline std::shared_ptr<value_number> make_value(T number) {
    // from the number
    return std::make_shared<value_number>(number);
}

inline const std::shared_ptr<value_symbol> make_value(const char* symbol) {
    // from the singleton table
    return value_symbol::get(symbol);
}

inline std::shared_ptr<value_string> make_value(const std::string string) {
    // from the string
    return std::make_shared<value_string>(string);
}

inline const std::shared_ptr<value_bool>& make_value(bool truth) {
    // from the singletons
    return (truth ? true_ : false_);
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
inline std::shared_ptr<value_pair> make_value_pair(T1&& car, T2&& cdr) {
    // from two separate universal references: car and cdr
    return std::make_shared<value_pair>(make_value(car), make_value(cdr));
}

template <typename T, typename... Tail>
inline std::shared_ptr<value_pair> make_value_list(T head, Tail... tail) {
    if constexpr (sizeof...(tail) > 0) {
        return make_value_pair(
            make_value(head),           // the head
            make_value_list(tail...));  // the rest
    } else {
        return make_value_pair(
            make_value(head),  // the tail
            nil);              // terminating nil
    }
}

inline std::shared_ptr<value_error> make_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    auto result = std::make_shared<value_error>(format, args);
    va_end(args);

    return result;
}

inline std::shared_ptr<value_info> make_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    auto result = std::make_shared<value_info>(format, args);
    va_end(args);

    return result;
}

// helper function

inline std::ostream& operator<<(std::ostream& os, const value& v) {
    return v.write(os);
}

#endif  // VALUE_HPP_
