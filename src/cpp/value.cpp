#include "value.hpp"

#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

using std::defaultfloat;
using std::ostream;
using std::ostringstream;
using std::setprecision;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::vector;

// value

value::operator bool() const {
    return (
        _type != value_t::bool_ ||
        reinterpret_cast<const value_bool*>(this)->truth());
}

// value_number

ostream& value_number::write(ostream& os) const {
    // write with a 12-digit precision, then restore the default
    return (os << setprecision(12) << _number << defaultfloat);
}

bool value_number::equals(const value& other) const {
    if (other.type() == value_t::number) {
        return (reinterpret_cast<const value_number&>(other)._number == _number);
    } else {
        return false;
    }
}

// value_symbol

const shared_ptr<value_symbol>& value_symbol::get(const string& symbol) {
    // static table of content-to-value mappings
    static unordered_map<string, shared_ptr<value_symbol>> _odarray;

    shared_ptr<value_symbol>& val = _odarray[symbol];

    if (!val) {
        // create a new value
        val.reset(new value_symbol(symbol));
    }

    return val;
}

ostream& value_symbol::write(ostream& os) const {
    // the symbol as is
    return (os << _symbol);
}

// value_string

ostream& value_string::write(ostream& os) const {
    // the string in quotes
    return (os << "\"" << _string << "\"");
}

bool value_string::equals(const value& other) const {
    if (other.type() == this->type()) {
        return (reinterpret_cast<const value_string&>(other)._string == _string);
    } else {
        return false;
    }
}

// value_error

ostream& value_error::write(ostream& os) const {
    // the red/white and bold error text
    return (os << BOLD(RED(<< _topic << ":")) " " BOLD(WHITE(<< _string <<)));
}

// value_info

ostream& value_info::write(ostream& os) const {
    // the green info text
    return (os << GREEN(<< _string <<));
}

// value_bool

const shared_ptr<value_bool>& value_bool::get(bool truth) {
    // static singletons: true and false
    static const shared_ptr<value_bool> true_ = shared_ptr<value_bool>(new value_bool(true));
    static const shared_ptr<value_bool> false_ = shared_ptr<value_bool>(new value_bool(false));

    return (truth ? true_ : false_);
}

ostream& value_bool::write(ostream& os) const {
    // the corresponding bool literal
    return (os << (_truth ? "true" : "false"));
}

// value_pair

ostream& value_pair::write(ostream& os) const {
    if (car()->type() == value_t::symbol &&                  // first item is a symbol
        to_ptr<value_symbol>(car())->symbol() == "quote" &&  // first item is a quote symbol
        cdr()->type() == value_t::pair &&                    // there is a second item
        pcdr()->cdr() == nil) {                              // there is no third item
        // (quote x) -> 'x
        // (quote (x y z)) -> '(x y z)
        os << '\'' << pcdr()->car()->str();

        return os;
    }

    os << "(";
    const value_pair* running{this};
    running->car()->write(os);  // write the first car
    while (true) {
        value_t cdr_type = running->cdr()->type();
        if (cdr_type == value_t::pair) {
            // go to the next cdr
            running = running->pcdr();
            // write the next car
            os << " ";
            running->car()->write(os);
        } else {
            if (cdr_type != value_t::nil) {
                // write the non-pair cdr
                os << " . ";
                running->cdr()->write(os);
            }
            break;
        }
    }
    os << ")";

    return os;
};

bool value_pair::equals(const value& other) const {
    if (other.type() == this->type()) {
        const value* running1 = this;
        const value* running2 = &other;
        while (running1->type() == value_t::pair && running1->type() == running2->type()) {
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
    const value_pair* running{this};
    while (true) {
        value_t cdr_type = running->cdr()->type();
        if (cdr_type == value_t::pair) {
            // the cdr is a pair: go to the next cdr
            running = running->pcdr();
        } else {
            // the cdr is not a pair: check if it is nil
            return cdr_type == value_t::nil;
        }
    }
}

size_t value_pair::length() const {
    size_t result = 0;
    const value_pair* running{this};
    while (true) {
        ++result;  // increment the length
        auto cdr_type = running->cdr()->type();
        if (cdr_type == value_t::pair) {
            // the cdr is a pair: go to the next cdr
            running = running->pcdr();
        } else {
            // the cdr is not a pair
            if (cdr_type != value_t::nil) {
                // count the non-nil terminating cdr
                ++result;
            }
            return result;
        }
    }
}

void value_pair::_throw_on_cycle_from(const shared_ptr<value>& other) {
    if (other->type() == value_t::pair) {
        const value_pair* running{to_ptr<value_pair>(other)};
        while (running != nilptr) {
            if (running == this) {
                throw cycle_error("cycle from %s (%p)", this->str().c_str(), this);
            }

            _throw_on_cycle_from(running->car());

            if (running->cdr()->type() == value_t::pair) {
                running = running->pcdr();
            } else {
                break;
            }
        }
    }
}

// value_nil

const shared_ptr<value_nil>& value_nil::get() {
    // static singleton: nil
    static const shared_ptr<value_nil> nil = shared_ptr<value_nil>(new value_nil);

    return nil;
}

// helper functions

const char* get_type_name(value_t t) {
    static string unknown_name = "unknown";
    static unordered_map<value_t, string> value_t_names = {
        {value_t::number, "number"},
        {value_t::symbol, "symbol"},
        {value_t::string, "string"},
        {value_t::format, "format"},
        {value_t::error, "error"},
        {value_t::info, "info"},
        {value_t::bool_, "bool"},
        {value_t::nil, "nil"},
        {value_t::pair, "pair"},
        {value_t::machine_op, "machine op"},
        {value_t::instruction, "instruction"},
        {value_t::environment, "environment"},
        {value_t::primitive_op, "primitive op"},
        {value_t::compound_op, "compound op"},
        {value_t::compiled_op, "compiled op"},
    };

    auto iter = value_t_names.find(t);
    if (iter != value_t_names.end()) {
        return iter->second.c_str();
    } else {
        return unknown_name.c_str();
    }
}
