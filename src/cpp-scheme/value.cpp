#include "value.hpp"

#include <cstdio>
#include <iomanip>
#include <iostream>
#include <unordered_map>

// value_symbol

const std::shared_ptr<value_symbol>& value_symbol::get(const std::string& symbol) {
    // static table of content-to-value mappings
    static std::unordered_map<std::string, std::shared_ptr<value_symbol>> _odarray;

    std::shared_ptr<value_symbol>& val = _odarray[symbol];

    if (!val) {
        // create a new value
        val.reset(new value_symbol(symbol));
    }

    return val;
}

// value_format

value_format::value_format(const char* format, va_list args) : value_string() {
    static char buffer[65536];
    vsnprintf(buffer, sizeof(buffer), format, args);
    _string = buffer;

    _type = value_t::format;
}

// value_bool

const std::shared_ptr<value_bool>& value_bool::get(bool truth) {
    // static singletons: true and false
    static const std::shared_ptr<value_bool> true_ = std::shared_ptr<value_bool>(new value_bool(true));
    static const std::shared_ptr<value_bool> false_ = std::shared_ptr<value_bool>(new value_bool(false));

    return (truth ? true_ : false_);
}

// value_nil

const std::shared_ptr<value_nil>& value_nil::get() {
    // static singleton: nil
    static const std::shared_ptr<value_nil> nil = std::shared_ptr<value_nil>(new value_nil);

    return nil;
}

// value_pair

void value_pair::value_iterator::_advance() {
    if (_ptr->cdr()->type() == value_t::pair) {
        _ptr = reinterpret_cast<value_pair*>(_ptr->cdr().get());  // cdr is a pair
    } else {
        if (_ptr->cdr() == nil) {
            // the list terminates: stop here
            _ptr = nullptr;
        } else if (!_at_cdr) {
            // the terminal cdr is not a pair: return it next
            _at_cdr = true;
        } else {
            // already returned the cdr: stop here
            _ptr = nullptr;
            // reset _at_cdr to false for equivalence
            // with end() iterator with false by default
            _at_cdr = false;
        }
    }
}

std::ostream& value_pair::write(std::ostream& os) const {
    os << "(";
    car()->write(os);  // write the first car
    std::shared_ptr<value> running{cdr()};
    while (running != nil) {
        if (running->type() == value_t::pair) {
            value_pair* pair = reinterpret_cast<value_pair*>(running.get());
            os << " ";
            pair->car()->write(os);  // write the next car
            running = pair->cdr();   // go to the following cdr
        } else {
            os << " . ";
            running->write(os);  // write the non-pair cdr
            break;               // and stop
        }
    }
    os << ")";

    return os;
};

bool value_pair::equals(const value& other) const {
    if (other.type() == value_t::pair) {
        const value* running1 = this;
        const value* running2 = &other;
        while (running1->type() == value_t::pair && running2->type() == value_t::pair) {
            const value_pair* pair1 = reinterpret_cast<const value_pair*>(running1);
            const value_pair* pair2 = reinterpret_cast<const value_pair*>(running2);

            if (!pair1->car()->equals(*pair2->car())) {
                return false;
            }

            running1 = pair1->cdr().get();
            running2 = pair2->cdr().get();
        }

        return running1->equals(*running2);
    } else {
        return false;
    }
}

bool value_pair::is_list() const {
    std::shared_ptr<value> running{cdr()};
    while (running != nil) {
        if (running->type() == value_t::pair) {
            // the cdr is a pair
            value_pair* pair = reinterpret_cast<value_pair*>(running.get());
            running = pair->cdr();  // go to the next cdr
        } else {
            // the (terminating) cdr is not a pair
            return false;  // this is not a list
        }
    }

    return true;
}

size_t value_pair::length() const {
    // at least one pair
    size_t result = 1;

    std::shared_ptr<value> running{cdr()};
    while (running != nil) {
        result += 1;  // increment the length
        if (running->type() == value_t::pair) {
            // the cdr is a pair
            value_pair* pair = reinterpret_cast<value_pair*>(running.get());
            running = pair->cdr();  // go to the next cdr
        } else {
            // the (terminating) cdr is not a pair
            break;  // stop the counting
        }
    }

    return result;
}
