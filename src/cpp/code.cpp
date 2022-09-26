#include "code.hpp"

#include <memory>
#include <string>
#include <vector>

#include "exception.hpp"
#include "value.hpp"

using std::make_shared;
using std::shared_ptr;
using std::string;
using std::vector;

// token

token::token(const shared_ptr<value>& v) {
    if (v->type() == value_t::pair) {
        // v is a pair
        auto pair = to_ptr<value_pair>(v);
        if (pair->is_list() and pair->length() == 2) {
            // v is a two-item list
            if (pair->car()->type() == value_t::symbol) {
                // the first item is a symbol
                string type_symbol = to_ptr<value_symbol>(pair->car())->symbol();
                if (type_symbol == "op") {
                    _type = token_t::op;
                } else if (type_symbol == "reg") {
                    _type = token_t::reg;
                } else if (type_symbol == "label") {
                    _type = token_t::label;
                } else if (type_symbol == "const") {
                    _type = token_t::const_;
                } else {
                    throw code_error(
                        "unrecognized token type: %s",
                        type_symbol.c_str());
                }

                if (_type == token_t::const_) {
                    // use the second item (value) as is
                    _content = pair->pcdr()->car();
                } else {
                    if (pair->pcdr()->car()->type() == value_t::symbol) {
                        // the second item (name) is a string
                        _content = to_ptr<value_symbol>(pair->pcdr()->car())->symbol();
                    } else {
                        throw code_error(
                            "non-const token's second item must be a string: %s",
                            pair->str().c_str());
                    }
                }
            } else {
                throw code_error(
                    "token's first item (name) must be a string: %s",
                    pair->str().c_str());
            }
        } else {
            throw code_error(
                "token must be a two-item list: %s",
                pair->str().c_str());
        }
    } else {
        throw code_error(
            "token must be a list: %s",
            v->str().c_str());
    }

    _str = v->str();
}

shared_ptr<value> token::to_value() const {
    switch (_type) {
        case token_t::op:
            return make_list("op", make_symbol(name()));
        case token_t::reg:
            return make_list("reg", make_symbol(name()));
        case token_t::label:
            return make_list("label", make_symbol(name()));
        case token_t::const_:
            // copy _val to avoid const conflict
            shared_ptr<value> v = val();
            return make_list("const", v);
    }

    return nil;
}

// code_label

code_label::code_label(const value_symbol* statement) : code(code_t::label) {
    _label = statement->symbol();
}

shared_ptr<value> code_label::to_value() const {
    return make_symbol(_label);
}

// code_assign_call

code_assign_call::code_assign_call(const value_pair* statement) : code(code_t::assign_call) {
    if (statement->length() >= 3) {
        // at least three items
        auto second = statement->pcdr()->car();
        auto third = statement->pcdr()->pcdr()->car();
        auto rest = statement->pcdr()->pcdr()->pcdr();

        if (second->type() == value_t::symbol) {
            // the second item is a symbol
            _reg = to_ptr<value_symbol>(second)->symbol();  // register
            token op_token{third};
            if (op_token.type() == token_t::op) {
                // the third item is an op token
                _op = op_token.name();
                while (rest != nilptr) {
                    // there is another arg
                    token arg_token{rest->car()};
                    if (arg_token.type() != token_t::op) {
                        // the arg is not an op token
                        _args.push_back(arg_token);
                    } else {
                        throw code_error(
                            "assign by call statement's args may not be op tokens: %s",
                            statement->str().c_str());
                    }
                    rest = rest->pcdr();
                }
            } else {
                throw code_error(
                    "assign by call statement's third item (op) must be an op token: %s",
                    statement->str().c_str());
            }
        } else {
            throw code_error(
                "assign statement's second item (register) must be a string: %s",
                statement->str().c_str());
        }
    } else {
        throw code_error(
            "assign by call statement must have at least 3 items: %s",
            statement->str().c_str());
    }
}

shared_ptr<value> code_assign_call::to_value() const {
    auto result = make_list(
        "assign",                            // header
        make_symbol(_reg),                   // register
        make_list("op", make_symbol(_op)));  // op

    auto tail = to_sptr<value_pair>(result->pcdr()->cdr());
    for (const auto& arg : _args) {
        auto arg_pair = make_vpair(arg.to_value(), nil);  // arg
        tail->cdr(arg_pair);
        tail = arg_pair;
    }

    return result;
}

// code_assign_copy

code_assign_copy::code_assign_copy(const value_pair* statement) : code(code_t::assign_copy) {
    if (statement->length() == 3) {
        // exactly three items
        auto second = statement->pcdr()->car();
        auto third = statement->pcdr()->pcdr()->car();

        if (second->type() == value_t::symbol) {
            // the second item is a symbol
            _reg = to_ptr<value_symbol>(second)->symbol();  // register
            token src_token{third};
            if (src_token.type() != token_t::op) {
                // the source is not an op token
                _src = src_token;
            } else {
                throw code_error(
                    "assign by copy statement's source may not be an op token: %s",
                    statement->str().c_str());
            }
        } else {
            throw code_error(
                "assign statement's second item (register) must be a string: %s",
                statement->str().c_str());
        }
    } else {
        throw code_error(
            "assign by copy statement must have exactly 3 items: %s",
            statement->str().c_str());
    }
}

shared_ptr<value> code_assign_copy::to_value() const {
    return make_list(
        "assign",           // header
        make_symbol(_reg),  // register
        _src.to_value());   // source
}

// code_perform

code_perform::code_perform(const value_pair* statement) : code(code_t::perform) {
    if (statement->length() >= 2) {
        // at least two items
        auto second = statement->pcdr()->car();
        auto rest = statement->pcdr()->pcdr();

        token op_token{second};
        if (op_token.type() == token_t::op) {
            // the second item is an op token
            _op = op_token.name();
            while (rest != nilptr) {
                // there is another arg
                token arg_token{rest->car()};
                if (arg_token.type() != token_t::op) {
                    // the arg is not an op token
                    _args.push_back(arg_token);
                } else {
                    throw code_error(
                        "perform statement's args may not be op tokens: %s",
                        statement->str().c_str());
                }
                rest = rest->pcdr();
            }
        } else {
            throw code_error(
                "perform statement's second item must be an op token: %s",
                statement->str().c_str());
        }
    } else {
        throw code_error(
            "perform statement must have at least 2 items: %s",
            statement->str().c_str());
    }
}

shared_ptr<value> code_perform::to_value() const {
    auto result = make_list(
        "perform",                           // header
        make_list("op", make_symbol(_op)));  // op

    auto tail = to_sptr<value_pair>(result->cdr());
    for (const auto& arg : _args) {
        auto arg_pair = make_vpair(arg.to_value(), nil);  // arg
        tail->cdr(arg_pair);
        tail = arg_pair;
    }

    return result;
}

// code_branch

code_branch::code_branch(const value_pair* statement) : code(code_t::branch) {
    if (statement->length() >= 3) {
        // at least three items
        auto second = statement->pcdr()->car();
        auto third = statement->pcdr()->pcdr()->car();
        auto rest = statement->pcdr()->pcdr()->pcdr();

        token label_token{second};
        if (label_token.type() == token_t::label) {
            // the second item is a label token
            _label = label_token.name();
            token op_token{third};
            if (op_token.type() == token_t::op) {
                // the third item is an op token
                _op = op_token.name();
                while (rest != nilptr) {
                    // there is another arg
                    token arg_token{rest->car()};
                    if (arg_token.type() != token_t::op) {
                        // the arg is not an op token
                        _args.push_back(arg_token);
                    } else {
                        throw code_error(
                            "branch statement's args may not be op tokens: %s",
                            statement->str().c_str());
                    }
                    rest = rest->pcdr();
                }
            } else {
                throw code_error(
                    "branch statement's third item (op) must be an op token: %s",
                    statement->str().c_str());
            }
        } else {
            throw code_error(
                "branch statement's second item (label) must be a label token: %s",
                statement->str().c_str());
        }
    } else {
        throw code_error(
            "branch statement must have at least 3 items: %s",
            statement->str().c_str());
    }
}

shared_ptr<value> code_branch::to_value() const {
    auto result = make_list(
        "branch",                                 // header
        make_list("label", make_symbol(_label)),  // label
        make_list("op", make_symbol(_op)));       // op

    auto tail = to_sptr<value_pair>(result->pcdr()->cdr());
    for (const auto& arg : _args) {
        auto arg_pair = make_vpair(arg.to_value(), nil);  // arg
        tail->cdr(arg_pair);
        tail = arg_pair;
    }

    return result;
}

// code_goto

code_goto::code_goto(const value_pair* statement) : code(code_t::goto_) {
    if (statement->length() == 2) {
        // exactly two items
        auto second = statement->pcdr()->car();
        _target = token(second);  // target token
    } else {
        throw code_error(
            "goto statement must have exactly 2 items: %s",
            statement->str().c_str());
    }
}

shared_ptr<value> code_goto::to_value() const {
    return make_list(
        "goto",               // header
        _target.to_value());  // target
}

// code_save

code_save::code_save(const value_pair* statement) : code(code_t::save) {
    if (statement->length() == 2) {
        // exactly two items
        auto second = statement->pcdr()->car();

        if (second->type() == value_t::symbol) {
            // the second item is a symbol
            _reg = to_ptr<value_symbol>(second)->symbol();
        } else {
            throw code_error(
                "save statement's second item (register) must be a string: %s",
                statement->str().c_str());
        }
    } else {
        throw code_error(
            "save statement must have exactly 2 items: %s",
            statement->str().c_str());
    }
}

shared_ptr<value> code_save::to_value() const {
    return make_list(
        "save",              // header
        make_symbol(_reg));  // register
}

// code_restore

code_restore::code_restore(const value_pair* statement) : code(code_t::restore) {
    if (statement->length() == 2) {
        // exactly two items
        auto second = statement->pcdr()->car();

        if (second->type() == value_t::symbol) {
            // the second item is a symbol
            _reg = to_ptr<value_symbol>(second)->symbol();
        } else {
            throw code_error(
                "restore statement's second item (register) must be a string: %s",
                statement->str().c_str());
        }
    } else {
        throw code_error(
            "restore statement must have exactly 2 items: %s",
            statement->str().c_str());
    }
}

shared_ptr<value> code_restore::to_value() const {
    return make_list(
        "restore",           // header
        make_symbol(_reg));  // register
}

// helper functions

namespace {

bool is_assign_call(const value_pair* statement) {
    if (statement->length() >= 3) {
        // at least three items
        token s{statement->pcdr()->pcdr()->car()};  // a token from the third item
        return s.type() == token_t::op;             // the token type decides
    } else {
        throw code_error(
            "assign statement must have at least 3 items: %s",
            statement->str().c_str());
    }
}

}  // namespace

vector<shared_ptr<code>> translate_to_code(const shared_ptr<value>& source) {
    vector<shared_ptr<code>> result;

    if (source != nil) {
        if (source->type() == value_t::pair) {
            // source is a list
            auto lines = to_ptr<value_pair>(source);
            for (auto p = lines->begin(); p != lines->end(); ++p) {
                // there is another statement
                if (p->type() == value_t::symbol) {
                    // the statement is a symbol
                    auto label = to_ptr<const value_symbol>(p.ptr());
                    result.push_back(make_shared<code_label>(label));
                } else if (p->type() == value_t::pair) {
                    // the statement is a list
                    auto statement = to_ptr<const value_pair>(p.ptr());
                    if (statement->car()->type() == value_t::symbol) {
                        // the statement header is a symbol
                        string header = to_ptr<value_symbol>(statement->car())->symbol();
                        if (header == "assign") {
                            if (is_assign_call(statement)) {
                                result.push_back(make_shared<code_assign_call>(statement));
                            } else {
                                result.push_back(make_shared<code_assign_copy>(statement));
                            }
                        } else if (header == "perform") {
                            result.push_back(make_shared<code_perform>(statement));
                        } else if (header == "branch") {
                            result.push_back(make_shared<code_branch>(statement));
                        } else if (header == "goto") {
                            result.push_back(make_shared<code_goto>(statement));
                        } else if (header == "save") {
                            result.push_back(make_shared<code_save>(statement));
                        } else if (header == "restore") {
                            result.push_back(make_shared<code_restore>(statement));
                        } else {
                            throw code_error(
                                "unrecognized statement header: %s",
                                header.c_str());
                        }
                    } else {
                        throw code_error(
                            "statement header must be a symbol: %s",
                            statement->car()->str().c_str());
                    }
                } else {
                    throw code_error(
                        "statement must be a symbol or a list: %s",
                        p->str().c_str());
                }
            }
        } else {
            throw code_error(
                "source must be a list of statements: %s",
                source->str().c_str());
        }
    }

    return result;
}
