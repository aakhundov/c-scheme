#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "constants.hpp"
#include "parse.hpp"
#include "value.hpp"

using namespace std::literals;

static size_t test_counter = 0;

void report_test(std::string message) {
    std::cout << std::setfill('0') << std::setw(5)
              << BLUE(<< ++test_counter <<) << " "
              << message << '\n';
}

#define RUN_TEST_FUNCTION(fn)                            \
    {                                                    \
        std::cout << "[" << #fn << "]\n";                \
        std::cout << "==============================\n"; \
        fn();                                            \
        std::cout << '\n';                               \
    }

#define ASSERT_TRUE(expr)                                                    \
    {                                                                        \
        report_test(BLUE("[") #expr BLUE("] == [") GREEN("true") BLUE("]")); \
        if (!(expr)) {                                                       \
            std::cerr << RED("expected true") << '\n';                       \
            exit(1);                                                         \
        }                                                                    \
    }

#define ASSERT_FALSE(expr)                                                  \
    {                                                                       \
        report_test(BLUE("[") #expr BLUE("] == [") RED("false") BLUE("]")); \
        if ((expr)) {                                                       \
            std::cerr << RED("expected false") << '\n';                     \
            exit(1);                                                        \
        }                                                                   \
    }

#define ASSERT_EQUAL(expr, expected)                                          \
    {                                                                         \
        auto result = (expr);                                                 \
        std::ostringstream buffer;                                            \
        buffer << result;                                                     \
        std::string str_result = buffer.str();                                \
        report_test(BLUE("[") #expr BLUE("] == [") + str_result + BLUE("]")); \
        if (result != (expected)) {                                           \
            std::cerr << RED("expected " #expected) << '\n';                  \
            exit(1);                                                          \
        }                                                                     \
    }

#define ASSERT_TO_STR(expr, expected)                                          \
    {                                                                          \
        std::string str_result = (expr).str();                                 \
        report_test(BLUE("[") #expr BLUE("] --> [") + str_result + BLUE("]")); \
        if (str_result != (expected)) {                                        \
            std::cerr << RED("expected " #expected) << '\n';                   \
            exit(1);                                                           \
        }                                                                      \
    }

#define ASSERT_ITERATOR(expr, expected)                                       \
    {                                                                         \
        value_pair pair = (expr);                                             \
        std::string str_items;                                                \
        for (auto& item : pair) {                                             \
            str_items += ((item->str()) + ", ");                              \
        }                                                                     \
        str_items.pop_back();                                                 \
        str_items.pop_back();                                                 \
        report_test(BLUE("[") #expr BLUE("] --> [") + str_items + BLUE("]")); \
        if (str_items != (expected)) {                                        \
            std::cerr << RED("expected " #expected) << '\n';                  \
            exit(1);                                                          \
        }                                                                     \
    }

#define ASSERT_EXCEPTION(code, type)                                             \
    {                                                                            \
        bool raised = false;                                                     \
        try {                                                                    \
            code                                                                 \
        } catch (type&) {                                                        \
            raised = true;                                                       \
        }                                                                        \
        report_test(BLUE("[") #code BLUE("] --> [") BOLD(RED(#type)) BLUE("]")); \
        if (!raised) {                                                           \
            std::cerr << RED("expected " #type " exception") << '\n';            \
            exit(1);                                                             \
        }                                                                        \
    }

#define ASSERT_PARSE_TO_STR(text, expected)                                    \
    {                                                                          \
        std::istringstream is{text};                                           \
        std::shared_ptr<value> v = parse_values(is);                           \
        std::string str_result = v->str();                                     \
        report_test(BLUE("[") #text BLUE("] --> [") + str_result + BLUE("]")); \
        if (str_result != (expected)) {                                        \
            std::cerr << RED("expected " #expected) << '\n';                   \
            exit(1);                                                           \
        }                                                                      \
    }

#define ASSERT_PARSE_ERROR(text, expected)                                              \
    {                                                                                   \
        std::istringstream is{text};                                                    \
        std::shared_ptr<value> v = parse_values(is);                                    \
        std::string str_result = v->str();                                              \
        report_test(BLUE("[") #text BLUE("] --> [") + str_result + BLUE("]"));          \
        if (v->type() != value_t::error) {                                              \
            std::cerr << RED("expected error") << '\n';                                 \
            exit(1);                                                                    \
        }                                                                               \
        std::shared_ptr<value_error> e = std::reinterpret_pointer_cast<value_error>(v); \
        if (e->string().find(expected) == std::string::npos) {                          \
            std::cerr << RED("expected error with " #expected) << '\n';                 \
            exit(1);                                                                    \
        }                                                                               \
    }

void test_value() {
    // number
    std::shared_ptr<value_number> num1, num2;
    ASSERT_TO_STR(*(num1 = make_number(3.14)), "3.14");
    ASSERT_TRUE(num1->type() == value_t::number);
    ASSERT_EQUAL(num1->number(), 3.14);
    ASSERT_TO_STR(*(num2 = make_number(3.14)), "3.14");
    ASSERT_TRUE(*num1 == *num2);
    ASSERT_FALSE(num1 == num2);

    // symbol
    std::shared_ptr<value_symbol> sym1, sym2;
    ASSERT_TO_STR(*(sym1 = make_symbol("abc")), "abc");
    ASSERT_TRUE(sym1->type() == value_t::symbol);
    ASSERT_EQUAL(sym1->symbol(), "abc");
    ASSERT_TO_STR(*(sym2 = make_symbol("abc")), "abc");
    ASSERT_TRUE(*sym1 == *sym2);
    ASSERT_TRUE(sym1 == sym2);

    // string
    std::shared_ptr<value_string> str1, str2;
    ASSERT_TO_STR(*(str1 = make_string("abc")), "\"abc\"");
    ASSERT_TRUE(str1->type() == value_t::string);
    ASSERT_EQUAL(str1->string(), "abc");
    ASSERT_TO_STR(*(str2 = make_string("abc")), "\"abc\"");
    ASSERT_TRUE(*str1 == *str2);
    ASSERT_FALSE(str1 == str2);

    // error
    std::shared_ptr<value_error> error1, error2;
    ASSERT_TO_STR(*(error1 = make_error("hello '%g'", 3.14)), BOLD(RED("error:") " " WHITE("hello '3.14'")));
    ASSERT_TRUE(error1->type() == value_t::error);
    ASSERT_EQUAL(error1->string(), "hello '3.14'");
    ASSERT_TO_STR(*(error2 = make_error("hello '%g'", 3.14)), BOLD(RED("error:") " " WHITE("hello '3.14'")));
    ASSERT_TRUE(*error1 == *error2);
    ASSERT_FALSE(error1 == error2);

    // info
    std::shared_ptr<value_info> info1, info2;
    ASSERT_TO_STR(*(info1 = make_info("hello '%g'", 3.14)), GREEN("hello '3.14'"));
    ASSERT_TRUE(info1->type() == value_t::info);
    ASSERT_EQUAL(info1->string(), "hello '3.14'");
    ASSERT_TO_STR(*(info2 = make_info("hello '%g'", 3.14)), GREEN("hello '3.14'"));
    ASSERT_TRUE(*info1 == *info2);
    ASSERT_FALSE(info1 == info2);

    // bool
    std::shared_ptr<value_bool> bool1, bool2;
    ASSERT_TO_STR(*(bool1 = make_bool(true)), "true");
    ASSERT_TRUE(bool1->type() == value_t::bool_);
    ASSERT_EQUAL(bool1->truth(), true);
    ASSERT_TO_STR(*(bool2 = make_bool(true)), "true");
    ASSERT_TRUE(*bool1 == *bool2);
    ASSERT_TRUE(bool1 == bool2);

    // nil
    std::shared_ptr<value_nil> nil1, nil2;
    ASSERT_TO_STR(*(nil1 = make_nil()), "()");
    ASSERT_TRUE(nil1->type() == value_t::nil);
    ASSERT_TO_STR(*(nil2 = make_nil()), "()");
    ASSERT_TRUE(*nil1 == *nil2);
    ASSERT_TRUE(nil1 == nil2);

    // pair
    std::shared_ptr<value_pair> pair1, pair2;
    ASSERT_TO_STR(*(pair1 = make_value_pair(3.14, "abc")), "(3.14 . abc)");
    ASSERT_TRUE(pair1->type() == value_t::pair);
    ASSERT_TO_STR(*pair1->car(), "3.14");
    ASSERT_TO_STR(*pair1->cdr(), "abc");
    ASSERT_TO_STR(*(pair2 = make_value_pair(3.14, "abc")), "(3.14 . abc)");
    ASSERT_TRUE(*pair1 == *pair2);
    ASSERT_FALSE(pair1 == pair2);

    // list
    std::shared_ptr<value_pair> list1, list2;
    ASSERT_TO_STR(*(list1 = make_value_list(3.14, "abc", nil, false)), "(3.14 abc () false)");
    ASSERT_TRUE(list1->type() == value_t::pair);
    ASSERT_TO_STR(*list1->car(), "3.14");
    ASSERT_TO_STR(*list1->cdr(), "(abc () false)");
    ASSERT_TO_STR(*list1->pcdr()->car(), "abc");
    ASSERT_TO_STR(*list1->pcdr()->cdr(), "(() false)");
    ASSERT_TO_STR(*list1->pcdr()->pcdr()->car(), "()");
    ASSERT_TO_STR(*list1->pcdr()->pcdr()->cdr(), "(false)");
    ASSERT_TO_STR(*list1->pcdr()->pcdr()->pcdr()->car(), "false");
    ASSERT_TO_STR(*list1->pcdr()->pcdr()->pcdr()->cdr(), "()");
    ASSERT_TO_STR(*(list2 = make_value_list(3.14, "abc", nil, false)), "(3.14 abc () false)");
    ASSERT_TRUE(*list1 == *list2);
    ASSERT_FALSE(list1 == list2);
}

void test_to_str() {
    // number
    ASSERT_TO_STR(*make_value(0), "0");
    ASSERT_TO_STR(*make_value(1), "1");
    ASSERT_TO_STR(*make_value(-1), "-1");
    ASSERT_TO_STR(*make_value(3.14), "3.14");
    ASSERT_TO_STR(*make_value(-3.14), "-3.14");
    ASSERT_TO_STR(*make_value(1e-20), "1e-20");
    ASSERT_TO_STR(*make_value(1e+20), "1e+20");
    ASSERT_TO_STR(*make_value(3.14e2), "314");
    ASSERT_TO_STR(*make_value(3.14e-2), "0.0314");
    ASSERT_TO_STR(*make_value(123'456'789'012), "123456789012");
    ASSERT_TO_STR(*make_value(1'234'567'890'123), "1.23456789012e+12");
    ASSERT_TO_STR(*make_value(123'456.789'012), "123456.789012");
    ASSERT_TO_STR(*make_value(123'456.789'012'3), "123456.789012");
    ASSERT_TO_STR(*make_value(0.123'456'789'012), "0.123456789012");
    ASSERT_TO_STR(*make_value(0.123'456'789'012'3), "0.123456789012");

    // symbol
    ASSERT_TO_STR(*make_value(""), "");
    ASSERT_TO_STR(*make_value("a"), "a");
    ASSERT_TO_STR(*make_value("abc"), "abc");
    ASSERT_TO_STR(*make_value("xxx"), "xxx");
    ASSERT_TO_STR(*make_value("!@#$%"), "!@#$%");

    // string
    ASSERT_TO_STR(*make_value(""s), "\"\"");
    ASSERT_TO_STR(*make_value("a"s), "\"a\"");
    ASSERT_TO_STR(*make_value("abc"s), "\"abc\"");
    ASSERT_TO_STR(*make_value("xxx"s), "\"xxx\"");
    ASSERT_TO_STR(*make_value("!@#$%"s), "\"!@#$%\"");

    // error
    ASSERT_TO_STR(*make_error(""), BOLD(RED("error:") " " WHITE("")));
    ASSERT_TO_STR(*make_error("message"), BOLD(RED("error:") " " WHITE("message")));
    ASSERT_TO_STR(*make_error("hello world"), BOLD(RED("error:") " " WHITE("hello world")));
    ASSERT_TO_STR(*make_error("hello '%s'", "world"), BOLD(RED("error:") " " WHITE("hello 'world'")));
    ASSERT_TO_STR(*make_error("hello '%g'", 3.14), BOLD(RED("error:") " " WHITE("hello '3.14'")));
    ASSERT_TO_STR(*make_error("hi %d, %d, %d bye", 1, 2, 3), BOLD(RED("error:") " " WHITE("hi 1, 2, 3 bye")));

    // info
    ASSERT_TO_STR(*make_info(""), GREEN(""));
    ASSERT_TO_STR(*make_info("message"), GREEN("message"));
    ASSERT_TO_STR(*make_info("hello world"), GREEN("hello world"));
    ASSERT_TO_STR(*make_info("hello '%s'", "world"), GREEN("hello 'world'"));
    ASSERT_TO_STR(*make_info("hello '%g'", 3.14), GREEN("hello '3.14'"));
    ASSERT_TO_STR(*make_info("hi %d, %d, %d bye", 1, 2, 3), GREEN("hi 1, 2, 3 bye"));

    // bool
    ASSERT_TO_STR(*true_, "true");
    ASSERT_TO_STR(*make_value(true), "true");
    ASSERT_TO_STR(*false_, "false");
    ASSERT_TO_STR(*make_value(false), "false");

    // bool
    ASSERT_TO_STR(*nil, "()");
    ASSERT_TO_STR(*make_nil(), "()");

    // identity
    ASSERT_TO_STR(*make_value(make_value(1)), "1");
    ASSERT_TO_STR(*make_value(make_value("abc")), "abc");
    ASSERT_TO_STR(*make_value(make_value("abc"s)), "\"abc\"");
    ASSERT_TO_STR(*make_value(true_), "true");
    ASSERT_TO_STR(*make_value(nil), "()");

    // pair
    ASSERT_TO_STR(*make_value_pair(1, 1), "(1 . 1)");
    ASSERT_TO_STR(*make_value_pair(1, 2), "(1 . 2)");
    ASSERT_TO_STR(*make_value_pair(1, "abc"), "(1 . abc)");
    ASSERT_TO_STR(*make_value_pair("xyz"s, 3.14), "(\"xyz\" . 3.14)");
    ASSERT_TO_STR(*make_value_pair(true_, false_), "(true . false)");
    ASSERT_TO_STR(*make_value_pair(1, nil), "(1)");
    ASSERT_TO_STR(*make_value_pair(nil, 2), "(() . 2)");
    ASSERT_TO_STR(*make_value_pair(nil, nil), "(())");

    // list
    ASSERT_TO_STR(*make_value_list(1), "(1)");
    ASSERT_TO_STR(*make_value_list(1, 2, 3), "(1 2 3)");
    ASSERT_TO_STR(*make_value_list(3.14, "abc", "xyz"s, true_, nil, false_), "(3.14 abc \"xyz\" true () false)");
    ASSERT_TO_STR(*make_value_list(1, 2, make_value_list(3, 4, 5), 6), "(1 2 (3 4 5) 6)");
    ASSERT_TO_STR(*make_value_list(1, 2, make_value_pair(3, 4)), "(1 2 (3 . 4))");
    ASSERT_TO_STR(*make_value_pair(1, make_value_pair(2, make_value_pair(3, 4))), "(1 2 3 . 4)");
    ASSERT_TO_STR(*make_value_list(make_value_list(make_value_list(1))), "(((1)))");
}

void test_pair() {
    // is_list
    ASSERT_TRUE(make_value_pair(1, nil)->is_list());
    ASSERT_FALSE(make_value_pair(nil, 2)->is_list());
    ASSERT_TRUE(make_value_pair(nil, nil)->is_list());
    ASSERT_FALSE(make_value_pair(1, 2)->is_list());
    ASSERT_TRUE(make_value_pair(1, make_value_pair(2, nil))->is_list());
    ASSERT_FALSE(make_value_pair(1, make_value_pair(2, 3))->is_list());
    ASSERT_TRUE(make_value_list(1)->is_list());
    ASSERT_TRUE(make_value_list(1, 2)->is_list());
    ASSERT_TRUE(make_value_list(1, 2, 3)->is_list());
    ASSERT_TRUE(make_value_list(1, 2, make_value_pair(3, 4))->is_list());
    ASSERT_TRUE(make_value_list(make_value_pair(1, 2), make_value_pair(3, 4))->is_list());
    ASSERT_TRUE(make_value_list(1, 2, nil)->is_list());
    ASSERT_TRUE(make_value_list(1, nil, nil)->is_list());
    ASSERT_TRUE(make_value_list(nil, nil, nil)->is_list());

    // length
    ASSERT_EQUAL(make_value_pair(1, nil)->length(), 1);
    ASSERT_EQUAL(make_value_pair(nil, 2)->length(), 2);
    ASSERT_EQUAL(make_value_pair(nil, nil)->length(), 1);
    ASSERT_EQUAL(make_value_pair(1, 2)->length(), 2);
    ASSERT_EQUAL(make_value_pair(1, make_value_pair(2, nil))->length(), 2);
    ASSERT_EQUAL(make_value_pair(1, make_value_pair(2, 3))->length(), 3);
    ASSERT_EQUAL(make_value_list(1)->length(), 1);
    ASSERT_EQUAL(make_value_list(1, 2)->length(), 2);
    ASSERT_EQUAL(make_value_list(1, 2, 3)->length(), 3);
    ASSERT_EQUAL(make_value_list(1, 2, make_value_pair(3, 4))->length(), 3);
    ASSERT_EQUAL(make_value_list(make_value_pair(1, 2), make_value_pair(3, 4))->length(), 2);
    ASSERT_EQUAL(make_value_list(1, 2, nil)->length(), 3);
    ASSERT_EQUAL(make_value_list(1, nil, nil)->length(), 3);
    ASSERT_EQUAL(make_value_list(nil, nil, nil)->length(), 3);

    // iterator
    ASSERT_ITERATOR(*make_value_pair(1, nil), "1");
    ASSERT_ITERATOR(*make_value_pair(nil, 2), "(), 2");
    ASSERT_ITERATOR(*make_value_pair(nil, nil), "()");
    ASSERT_ITERATOR(*make_value_pair(1, 2), "1, 2");
    ASSERT_ITERATOR(*make_value_pair(1, make_value_pair(2, nil)), "1, 2");
    ASSERT_ITERATOR(*make_value_pair(1, make_value_pair(2, 3)), "1, 2, 3");
    ASSERT_ITERATOR(*make_value_pair(1, "2"), "1, 2");
    ASSERT_ITERATOR(*make_value_pair(1, "2"s), "1, \"2\"");
    ASSERT_ITERATOR(*make_value_list(1), "1");
    ASSERT_ITERATOR(*make_value_list(1, 2), "1, 2");
    ASSERT_ITERATOR(*make_value_list(1, 2, 3), "1, 2, 3");
    ASSERT_ITERATOR(*make_value_list(1, 2, make_value_pair(3, 4)), "1, 2, (3 . 4)");
    ASSERT_ITERATOR(*make_value_list(make_value_pair(1, 2), make_value_pair(3, 4)), "(1 . 2), (3 . 4)");
    ASSERT_ITERATOR(*make_value_list(1, 2, nil), "1, 2, ()");
    ASSERT_ITERATOR(*make_value_list(1, nil, nil), "1, (), ()");
    ASSERT_ITERATOR(*make_value_list(nil, nil, nil), "(), (), ()");
    ASSERT_ITERATOR(*make_value_list(1, "2", "3"s, make_value_pair(nil, false)), "1, 2, \"3\", (() . false)");

    // iterator mutation
    value_pair val = *make_value_list(1, 2, 3, 4, 5);
    ASSERT_ITERATOR(val, "1, 2, 3, 4, 5");
    for (auto& item : val) {
        value_number* number = reinterpret_cast<value_number*>(item.get());
        item = make_value(number->number() * number->number());
    }
    ASSERT_ITERATOR(val, "1, 4, 9, 16, 25");

    // cycle
    std::shared_ptr<value_pair> v1, v2, v3, v4;
    v1 = make_value_pair(1, 2);
    ASSERT_EXCEPTION({ v1->car(v1); }, cycle_error);
    v1 = make_value_pair(1, 2);
    v2 = make_value_pair(3, 4);
    ASSERT_EXCEPTION({ v1->car(v2); v2->car(v1); }, cycle_error);
    v1 = make_value_pair(1, 2);
    v2 = make_value_pair(3, 4);
    ASSERT_EXCEPTION({ v1->car(v2); v2->cdr(v1); }, cycle_error);
    v1 = make_value_pair(1, 2);
    v2 = make_value_pair(3, 4);
    ASSERT_EXCEPTION({ v1->cdr(v2); v2->car(v1); }, cycle_error);
    v1 = make_value_pair(1, 2);
    v2 = make_value_pair(3, 4);
    ASSERT_EXCEPTION({ v1->cdr(v2); v2->cdr(v1); }, cycle_error);
    v1 = make_value_pair(1, 2);
    v2 = make_value_pair(3, 4);
    v3 = make_value_pair(5, 6);
    ASSERT_EXCEPTION({ v1->car(v2); v2->car(v3); v3->car(v1); }, cycle_error);
    v1 = make_value_pair(1, 2);
    v2 = make_value_pair(3, 4);
    v3 = make_value_pair(5, 6);
    ASSERT_EXCEPTION({ v1->cdr(v2); v2->car(v3); v3->cdr(v1); }, cycle_error);
    v1 = make_value_pair(1, 2);
    v2 = make_value_pair(3, 4);
    v3 = make_value_pair(5, 6);
    ASSERT_EXCEPTION({ v1->car(v2); v2->cdr(v3); v3->car(v1); }, cycle_error);
    v1 = make_value_pair(1, 2);
    v2 = make_value_pair(3, 4);
    v3 = make_value_pair(5, 6);
    ASSERT_EXCEPTION({ v1->cdr(v2); v2->cdr(v3); v3->cdr(v1); }, cycle_error);
    v1 = make_value_pair(1, 2);
    v2 = make_value_pair(3, 4);
    v3 = make_value_pair(5, 6);
    v4 = make_value_pair(7, 8);
    ASSERT_EXCEPTION({ v1->car(v2); v2->car(v3); v3->car(v4); v4->car(v1); }, cycle_error);
    v1 = make_value_pair(1, 2);
    v2 = make_value_pair(3, 4);
    v3 = make_value_pair(5, 6);
    v4 = make_value_pair(7, 8);
    ASSERT_EXCEPTION({ v1->car(v2); v2->cdr(v3); v3->car(v4); v4->cdr(v1); }, cycle_error);
    v1 = make_value_pair(1, 2);
    v2 = make_value_pair(3, 4);
    v3 = make_value_pair(5, 6);
    v4 = make_value_pair(7, 8);
    ASSERT_EXCEPTION({ v1->cdr(v2); v2->car(v3); v3->cdr(v4); v4->car(v1); }, cycle_error);
    v1 = make_value_pair(1, 2);
    v2 = make_value_pair(3, 4);
    v3 = make_value_pair(5, 6);
    v4 = make_value_pair(7, 8);
    ASSERT_EXCEPTION({ v1->cdr(v2); v2->cdr(v3); v3->cdr(v4); v4->cdr(v1); }, cycle_error);
}

void test_equal() {
    // number
    std::shared_ptr<value> num1, num2, num3;
    ASSERT_TO_STR(*(num1 = make_value(3.14)), "3.14");
    ASSERT_TO_STR(*(num2 = make_value(3.14)), "3.14");
    ASSERT_TO_STR(*(num3 = make_value(6.28)), "6.28");

    ASSERT_TRUE(*num1 == *num1);
    ASSERT_TRUE(*num1 == *num2);
    ASSERT_FALSE(*num1 == *num3);
    ASSERT_TRUE(*num2 == *num1);
    ASSERT_TRUE(*num2 == *num2);
    ASSERT_FALSE(*num2 == *num3);
    ASSERT_FALSE(*num3 == *num1);
    ASSERT_FALSE(*num3 == *num2);
    ASSERT_TRUE(*num3 == *num3);

    ASSERT_FALSE(num1 == num2);
    ASSERT_FALSE(num2 == num3);
    ASSERT_FALSE(num1 == num3);

    // symbol
    std::shared_ptr<value> sym1, sym2, sym3;
    ASSERT_TO_STR(*(sym1 = make_value("abc")), "abc");
    ASSERT_TO_STR(*(sym2 = make_value("abc")), "abc");
    ASSERT_TO_STR(*(sym3 = make_value("ab")), "ab");

    ASSERT_TRUE(*sym1 == *sym1);
    ASSERT_TRUE(*sym1 == *sym2);
    ASSERT_FALSE(*sym1 == *sym3);
    ASSERT_TRUE(*sym2 == *sym1);
    ASSERT_TRUE(*sym2 == *sym2);
    ASSERT_FALSE(*sym2 == *sym3);
    ASSERT_FALSE(*sym3 == *sym1);
    ASSERT_FALSE(*sym3 == *sym2);
    ASSERT_TRUE(*sym3 == *sym3);

    ASSERT_TRUE(sym1 == sym2);  // singleton
    ASSERT_FALSE(sym2 == sym3);
    ASSERT_FALSE(sym1 == sym3);

    // string
    std::shared_ptr<value> str1, str2, str3;
    ASSERT_TO_STR(*(str1 = make_value("abc"s)), "\"abc\"");
    ASSERT_TO_STR(*(str2 = make_value("abc"s)), "\"abc\"");
    ASSERT_TO_STR(*(str3 = make_value("ab"s)), "\"ab\"");

    ASSERT_TRUE(*str1 == *str1);
    ASSERT_TRUE(*str1 == *str2);
    ASSERT_FALSE(*str1 == *str3);
    ASSERT_TRUE(*str2 == *str1);
    ASSERT_TRUE(*str2 == *str2);
    ASSERT_FALSE(*str2 == *str3);
    ASSERT_FALSE(*str3 == *str1);
    ASSERT_FALSE(*str3 == *str2);
    ASSERT_TRUE(*str3 == *str3);

    ASSERT_FALSE(str1 == str2);
    ASSERT_FALSE(str2 == str3);
    ASSERT_FALSE(str1 == str3);

    // bool
    std::shared_ptr<value> bool1, bool2, bool3;
    ASSERT_TO_STR(*(bool1 = make_value(true)), "true");
    ASSERT_TO_STR(*(bool2 = make_value(true)), "true");
    ASSERT_TO_STR(*(bool3 = make_value(false)), "false");

    ASSERT_TRUE(*bool1 == *bool1);
    ASSERT_TRUE(*bool1 == *bool2);
    ASSERT_FALSE(*bool1 == *bool3);
    ASSERT_TRUE(*bool2 == *bool1);
    ASSERT_TRUE(*bool2 == *bool2);
    ASSERT_FALSE(*bool2 == *bool3);
    ASSERT_FALSE(*bool3 == *bool1);
    ASSERT_FALSE(*bool3 == *bool2);
    ASSERT_TRUE(*bool3 == *bool3);

    ASSERT_TRUE(bool1 == bool2);  // singleton
    ASSERT_FALSE(bool2 == bool3);
    ASSERT_FALSE(bool1 == bool3);

    ASSERT_TRUE(bool1 == true_);   // predefined
    ASSERT_TRUE(bool2 == true_);   // predefined
    ASSERT_TRUE(bool3 == false_);  // predefined

    // nil
    std::shared_ptr<value> nil1, nil2, nil3;
    ASSERT_TO_STR(*(nil1 = make_nil()), "()");
    ASSERT_TO_STR(*(nil2 = make_nil()), "()");
    ASSERT_TO_STR(*(nil3 = nil), "()");

    ASSERT_TRUE(*nil1 == *nil1);
    ASSERT_TRUE(*nil1 == *nil2);
    ASSERT_TRUE(*nil1 == *nil3);
    ASSERT_TRUE(*nil2 == *nil1);
    ASSERT_TRUE(*nil2 == *nil2);
    ASSERT_TRUE(*nil2 == *nil3);
    ASSERT_TRUE(*nil3 == *nil1);
    ASSERT_TRUE(*nil3 == *nil2);
    ASSERT_TRUE(*nil3 == *nil3);

    ASSERT_TRUE(nil1 == nil2);
    ASSERT_TRUE(nil2 == nil3);

    ASSERT_TRUE(nil1 == nil);  // predefined
    ASSERT_TRUE(nil2 == nil);  // predefined
    ASSERT_TRUE(nil3 == nil);  // predefined

    // pair
    std::shared_ptr<value> pair1, pair2, pair3;
    ASSERT_TO_STR(*(pair1 = make_value_pair(3.14, "abc")), "(3.14 . abc)");
    ASSERT_TO_STR(*(pair2 = make_value_pair(3.14, "abc")), "(3.14 . abc)");
    ASSERT_TO_STR(*(pair3 = make_value_pair(3.14, "abc"s)), "(3.14 . \"abc\")");

    ASSERT_TRUE(*pair1 == *pair1);
    ASSERT_TRUE(*pair1 == *pair2);
    ASSERT_FALSE(*pair1 == *pair3);
    ASSERT_TRUE(*pair2 == *pair1);
    ASSERT_TRUE(*pair2 == *pair2);
    ASSERT_FALSE(*pair2 == *pair3);
    ASSERT_FALSE(*pair3 == *pair1);
    ASSERT_FALSE(*pair3 == *pair2);
    ASSERT_TRUE(*pair3 == *pair3);

    ASSERT_FALSE(pair1 == pair2);
    ASSERT_FALSE(pair2 == pair3);
    ASSERT_FALSE(pair1 == pair3);

    // list
    std::shared_ptr<value> list1, list2, list3;
    ASSERT_TO_STR(*(list1 = make_value_list(1, "2", make_value_pair("3"s, 4))), "(1 2 (\"3\" . 4))");
    ASSERT_TO_STR(*(list2 = make_value_list(1, "2", make_value_pair("3"s, 4))), "(1 2 (\"3\" . 4))");
    ASSERT_TO_STR(*(list3 = make_value_list(1, "2", "3"s, 4)), "(1 2 \"3\" 4)");

    ASSERT_TRUE(*list1 == *list1);
    ASSERT_TRUE(*list1 == *list2);
    ASSERT_FALSE(*list1 == *list3);
    ASSERT_TRUE(*list2 == *list1);
    ASSERT_TRUE(*list2 == *list2);
    ASSERT_FALSE(*list2 == *list3);
    ASSERT_FALSE(*list3 == *list1);
    ASSERT_FALSE(*list3 == *list2);
    ASSERT_TRUE(*list3 == *list3);

    ASSERT_FALSE(list1 == list2);
    ASSERT_FALSE(list2 == list3);
    ASSERT_FALSE(list1 == list3);
}

void test_parse() {
    // brackets
    ASSERT_PARSE_TO_STR("", "()");
    ASSERT_PARSE_TO_STR("()()", "(() ())");
    ASSERT_PARSE_TO_STR("() ()", "(() ())");
    ASSERT_PARSE_TO_STR(" ()  ()  ", "(() ())");
    ASSERT_PARSE_TO_STR("(()())", "((() ()))");
    ASSERT_PARSE_TO_STR("()", "(())");
    ASSERT_PARSE_TO_STR("(  )", "(())");
    ASSERT_PARSE_TO_STR("  (  )   ", "(())");
    ASSERT_PARSE_TO_STR("(() ((()) () ())) ()", "((() ((()) () ())) ())");
    ASSERT_PARSE_TO_STR(" ( () ( (( )) ()  () ) )  () ", "((() ((()) () ())) ())");

    // integers
    ASSERT_PARSE_TO_STR("1", "(1)");
    ASSERT_PARSE_TO_STR("1 2", "(1 2)");
    ASSERT_PARSE_TO_STR("123", "(123)");
    ASSERT_PARSE_TO_STR("123456789", "(123456789)");
    ASSERT_PARSE_TO_STR("1234 56789", "(1234 56789)");
    ASSERT_PARSE_TO_STR("(1)", "((1))");
    ASSERT_PARSE_TO_STR("(1 2)", "((1 2))");
    ASSERT_PARSE_TO_STR("((1))", "(((1)))");
    ASSERT_PARSE_TO_STR("((1) (2))", "(((1) (2)))");
    ASSERT_PARSE_TO_STR("(1 () 2)", "((1 () 2))");
    ASSERT_PARSE_TO_STR("1 2 3", "(1 2 3)");
    ASSERT_PARSE_TO_STR("(1 2 3)", "((1 2 3))");
    ASSERT_PARSE_TO_STR("  (  1    2 3 )  ", "((1 2 3))");
    ASSERT_PARSE_TO_STR("1 (2 3 (4) 5 ((6 7) 8 9) 10)", "(1 (2 3 (4) 5 ((6 7) 8 9) 10))");
    ASSERT_PARSE_TO_STR("  1  (  2 3 (4  ) 5 (( 6  7 )))", "(1 (2 3 (4) 5 ((6 7))))");
    ASSERT_PARSE_TO_STR("(() 1 (() 2) (3 ()) 4 (5 (()) 6) 7 ())", "((() 1 (() 2) (3 ()) 4 (5 (()) 6) 7 ()))");
    ASSERT_PARSE_TO_STR("1 (23 (4) 5 ((67) 89) 10)", "(1 (23 (4) 5 ((67) 89) 10))");

    // decimals
    ASSERT_PARSE_TO_STR(".456 1. 2.0 3.14 -4. -5.67 -.123", "(0.456 1 2 3.14 -4 -5.67 -0.123)");
    ASSERT_PARSE_TO_STR("1e10 1e2 1e1 1e0 1e-1 1e-2 1e-10", "(10000000000 100 10 1 0.1 0.01 1e-10)");

    // symbols
    ASSERT_PARSE_TO_STR("x", "(x)");
    ASSERT_PARSE_TO_STR("x y", "(x y)");
    ASSERT_PARSE_TO_STR("abc xyz", "(abc xyz)");
    ASSERT_PARSE_TO_STR("!?_ */- \\%^", "(!?_ */- \\%^)");
    ASSERT_PARSE_TO_STR("x y z", "(x y z)");
    ASSERT_PARSE_TO_STR("x 1 y 2 z", "(x 1 y 2 z)");
    ASSERT_PARSE_TO_STR("xyz a1 abc 2b def", "(xyz a1 abc 2b def)");

    // quote
    ASSERT_PARSE_TO_STR("'1", "((quote 1))");
    ASSERT_PARSE_TO_STR("  '1", "((quote 1))");
    ASSERT_PARSE_TO_STR("  '  1", "((quote 1))");
    ASSERT_PARSE_TO_STR("  '  1  ", "((quote 1))");
    ASSERT_PARSE_TO_STR("'1 2", "((quote 1) 2)");
    ASSERT_PARSE_TO_STR("'1 '2", "((quote 1) (quote 2))");
    ASSERT_PARSE_TO_STR("''1", "((quote (quote 1)))");
    ASSERT_PARSE_TO_STR("'''1", "((quote (quote (quote 1))))");
    ASSERT_PARSE_TO_STR("1 2 ' (3 4 (5 6 7))", "(1 2 (quote (3 4 (5 6 7))))");
    ASSERT_PARSE_TO_STR("'1 2 ' (3 4 (5 '6 7))", "((quote 1) 2 (quote (3 4 (5 (quote 6) 7))))");
    ASSERT_PARSE_TO_STR("1 2 '(3 4 '(5 6 7))", "(1 2 (quote (3 4 (quote (5 6 7)))))");
    ASSERT_PARSE_TO_STR("''(1 2 3)", "((quote (quote (1 2 3))))");

    // comment
    ASSERT_PARSE_TO_STR("(1 2 3); comment", "((1 2 3))");
    ASSERT_PARSE_TO_STR("(1 2 3); (4 5 \" 6 7", "((1 2 3))");
    ASSERT_PARSE_TO_STR("(1 2 3)   ;   comment  ", "((1 2 3))");
    ASSERT_PARSE_TO_STR("\n(1 2 3)   ;   comment  \n  (4 5)", "((1 2 3) (4 5))");
    ASSERT_PARSE_TO_STR(" ; comment\n(1 2 3)   ;   comment  \n  (4 5)", "((1 2 3) (4 5))");

    // string
    ASSERT_PARSE_TO_STR("\"abc\"", "(\"abc\")");
    ASSERT_PARSE_TO_STR("\"\"", "(\"\")");
    ASSERT_PARSE_TO_STR("\"x\" \"y\" \"z\"", "(\"x\" \"y\" \"z\")");
    ASSERT_PARSE_TO_STR("\"a\\tb\"", "(\"a\\tb\")");
    ASSERT_PARSE_TO_STR("\"a\tb\"", "(\"a\tb\")");
    ASSERT_PARSE_TO_STR("\"a\\nb\"", "(\"a\\nb\")");
    ASSERT_PARSE_TO_STR("\"\na\nb\"", "(\"\na\nb\")");
    ASSERT_PARSE_TO_STR("'\"abc\"", "((quote \"abc\"))");
    ASSERT_PARSE_TO_STR("\"'abc\"", "(\"'abc\")");
    ASSERT_PARSE_TO_STR("'\"x\" \"y\" \"z\"", "((quote \"x\") \"y\" \"z\")");
    ASSERT_PARSE_TO_STR("'(\"x\" \"y\" \"z\")", "((quote (\"x\" \"y\" \"z\")))");

    // special symbols
    ASSERT_PARSE_TO_STR("nil", "(())");
    ASSERT_PARSE_TO_STR("true", "(true)");
    ASSERT_PARSE_TO_STR("false", "(false)");
    ASSERT_PARSE_TO_STR("#t", "(true)");
    ASSERT_PARSE_TO_STR("#f", "(false)");

    // parsing errors
    ASSERT_PARSE_ERROR("(1 2", "unterminated list");
    ASSERT_PARSE_ERROR("1 2)", "premature end of list");
    ASSERT_PARSE_ERROR("( 1 (2", "unterminated list");
    ASSERT_PARSE_ERROR("'", "unfollowed quote");
    ASSERT_PARSE_ERROR("'  a '  ", "unfollowed quote");
    ASSERT_PARSE_ERROR("\"", "unterminated string");
    ASSERT_PARSE_ERROR("\"xyz", "unterminated string");
    ASSERT_PARSE_ERROR(" \" xyz ", "unterminated string");
    ASSERT_PARSE_ERROR("\"xyz\" \"a", "unterminated string");
    ASSERT_PARSE_ERROR("@", "unexpected character: '@'");
    ASSERT_PARSE_ERROR("  @", "unexpected character: '@'");
    ASSERT_PARSE_ERROR("  @  ", "unexpected character: '@'");
    ASSERT_PARSE_ERROR("\n\n@", "unexpected character: '@'");
    ASSERT_PARSE_ERROR(" a bc\nde \n f@", "unexpected character: '@'");
    ASSERT_PARSE_ERROR(" a bc\nde \n f@ g h\n\ni j ", "unexpected character: '@'");
    ASSERT_PARSE_ERROR("@(1 (2 \n 3) (4\n 5  6\n 7)\n)", "unexpected character: '@'");
    ASSERT_PARSE_ERROR("(1 (2 \n 3) (4\n 5 @ 6\n 7)\n)", "unexpected character: '@'");
    ASSERT_PARSE_ERROR("(1 (2 \n 3) (4\n 5  6\n 7)\n)@", "unexpected character: '@'");
}

int main() {
    RUN_TEST_FUNCTION(test_value);
    RUN_TEST_FUNCTION(test_to_str);
    RUN_TEST_FUNCTION(test_pair);
    RUN_TEST_FUNCTION(test_equal);
    RUN_TEST_FUNCTION(test_parse);

    std::cout << "all tests have been passed!\n";

    return 0;
}
