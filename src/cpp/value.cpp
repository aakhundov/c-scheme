#include "value.hpp"

#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>

using std::defaultfloat;
using std::ostream;
using std::ostringstream;
using std::setprecision;
using std::shared_ptr;
using std::unordered_map;
using std::string;

// value_number

ostream& value_number::write(ostream& os) const {
    // write with a 12-digit precision, then restore the default
    return (os << setprecision(12) << _number << defaultfloat);
}

bool value_number::equals(const value& other) const {
    if (other.type() == value_t::number) {
        return (reinterpret_cast<const value_number*>(&other)->_number == _number);
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
        return (reinterpret_cast<const value_string*>(&other)->_string == _string);
    } else {
        return false;
    }
}

// value_error

ostream& value_error::write(ostream& os) const {
    // the red/white and bold error text
    return (os << BOLD(RED("error:") " " WHITE(<< _string <<)));
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

// value_nil

const shared_ptr<value_nil>& value_nil::get() {
    // static singleton: nil
    static const shared_ptr<value_nil> nil = shared_ptr<value_nil>(new value_nil);

    return nil;
}

ostream& value_nil::write(ostream& os) const {
    // the empty list notation for the nil
    return (os << "()");
};

// value_pair

void value_pair::iterator::_advance() {
    if (_ptr->cdr()->compound()) {
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

ostream& value_pair::write(ostream& os) const {
    if (car()->type() == value_t::symbol &&              // first item is a symbol
        to<value_symbol>(car())->symbol() == "quote" &&  // first item is a quote symbol
        cdr()->type() == value_t::pair &&                // there is a second item
        pcdr()->cdr() == nil) {                          // there is no third item
        // (quote x) -> 'x
        // (quote (x y z)) -> '(x y z)
        os << '\'' << pcdr()->car()->str();

        return os;
    }

    os << "(";
    car()->write(os);  // write the first car
    shared_ptr<value> running{cdr()};
    while (running != nil) {
        if (running->compound()) {
            auto pair = to<value_pair>(running);
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
    if (other.type() == this->type()) {
        const value* running1 = this;
        const value* running2 = &other;
        while (running1->compound() && running1->type() == running2->type()) {
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
    shared_ptr<value> running{cdr()};
    while (running != nil) {
        if (running->compound()) {
            // the cdr is a pair: go to the next cdr
            running = to<value_pair>(running)->cdr();
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

    shared_ptr<value> running{cdr()};
    while (running != nil) {
        result += 1;  // increment the length
        if (running->compound()) {
            // the cdr is a pair: go to the next cdr
            running = to<value_pair>(running)->cdr();
        } else {
            // the (terminating) cdr is not a pair
            break;  // stop the counting
        }
    }

    return result;
}

void value_pair::_throw_on_cycle_from(const shared_ptr<value>& other) {
    if (other->compound()) {
        shared_ptr<value> running{other};
        while (running != nil) {
            if (running->compound()) {
                const value_pair* pair = reinterpret_cast<value_pair*>(running.get());

                if (pair == this) {
                    throw cycle_error(this);
                }

                _throw_on_cycle_from(pair->car());
                running = pair->cdr();
            } else {
                break;
            }
        }
    }
}

// helper functions

ostream& operator<<(ostream& os, const value_t& t) {
    switch (t) {
        case value_t::number:
            os << "number";
            break;
        case value_t::symbol:
            os << "symbol";
            break;
        case value_t::string:
            os << "string";
            break;
        case value_t::format:
            os << "format";
            break;
        case value_t::error:
            os << "error";
            break;
        case value_t::info:
            os << "info";
            break;
        case value_t::bool_:
            os << "bool";
            break;
        case value_t::nil:
            os << "nil";
            break;
        case value_t::pair:
            os << "pair";
            break;
        default:
            os << "unknown";
    }

    return os;
}

// exceptions

string cycle_error::_make_message(const value_pair* from) {
    ostringstream s;
    s << "cycle from " << *from << " (" << from << ")";
    return s.str();
}
