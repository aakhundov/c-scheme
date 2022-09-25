#include <cassert>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#include "code.hpp"
#include "constants.hpp"
#include "machine.hpp"
#include "parse.hpp"
#include "value.hpp"

using namespace std::literals;

using std::cerr;
using std::cout;
using std::function;
using std::ostringstream;
using std::pair;
using std::setfill;
using std::setw;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::filesystem::directory_iterator;
using std::filesystem::path;

static size_t test_counter = 0;

void report_test(string message) {
    // increment the test_counter and report
    cout << BLUE(<< setfill('0') << setw(5) << ++test_counter <<) << " "
         << message << '\n';
}

void run_test_function(function<void(void)> fn, string fn_str) {
    cout << "[" << fn_str << "]\n";
    cout << "==============================\n";

    fn();

    cout << '\n';
}

void assert_true(bool result, string expr_str) {
    report_test(BLUE("[") + expr_str + BLUE("] == [") + GREEN("true") BLUE("]"));

    if (!result) {
        // the result is false
        cerr << RED("expected true") << '\n';
        exit(EXIT_FAILURE);
    }
}

void assert_false(bool result, string expr_str) {
    report_test(BLUE("[") + expr_str + BLUE("] == [") RED("false") BLUE("]"));

    if (result) {
        // the result is true
        cerr << RED("expected false") << '\n';
        exit(EXIT_FAILURE);
    }
}

template <typename T1, typename T2>
void assert_equal(T1 expr, T2 expected, string expr_str, string expected_str) {
    ostringstream buffer;
    buffer << (expr);
    string str_result = buffer.str();

    report_test(BLUE("[") + expr_str + BLUE("] == [") + str_result + BLUE("]"));

    if ((expr) != (expected)) {
        // the expr is not equal to the expected
        cerr << RED("expected " + expected_str +) << '\n';
        exit(EXIT_FAILURE);
    }
}

void assert_to_str(const value& v, string expected, string v_str) {
    string str_result = v.str();

    report_test(BLUE("[") + v_str + BLUE("] --> [") + str_result + BLUE("]"));

    if (str_result != expected) {
        // string version of v is not equal to the expected
        cerr << RED("expected \"" + expected + "\"") << '\n';
        exit(EXIT_FAILURE);
    }
}

void assert_iterator(value_pair& v, string str_expected, string v_str) {
    // join v's items with ", "
    string str_result;
    for (const auto& item : v) {
        str_result += ((item.str()) + ", ");
    }
    str_result.pop_back();  // drop trailing comma
    str_result.pop_back();  // drop trailing space

    report_test(BLUE("[") + v_str + BLUE("] --> [") + str_result + BLUE("]"));

    // compare to the expected string
    if (str_result != str_expected) {
        // ", "-joined items of v don't equal the str_expected
        cerr << RED("expected \"" + str_expected + "\"") << '\n';
        exit(EXIT_FAILURE);
    }
}

void assert_parse(string text, const shared_ptr<value_pair>& expected) {
    shared_ptr<value> values = parse_values_from(text);

    report_test(BLUE("[") + text + BLUE("] --> [") + values->str() + BLUE("]"));

    if (*values != *expected) {
        // the parsed values don't equal to the expected
        cerr << RED("expected \"" + expected->str() + "\"\n");
        exit(EXIT_FAILURE);
    }
}

void assert_exception_thrown(string code, string type, string what, string substring) {
    report_test(BLUE("[") + code + BLUE("] --> [") RED("" + type + ": " + what + "") BLUE("]"));

    if (what.find(substring) == string::npos) {
        // the expected exception was thrown, but the substring not in e.what()
        cerr << RED("expected " + type + " with \"" + substring + "\"") << '\n';
        exit(EXIT_FAILURE);
    }
}

void assert_exception_not_thrown(string code, string type, string substring) {
    report_test(BLUE("[") + code + BLUE("] --> [") "no exception" BLUE("]"));

    // the expected expection was not thrown
    cerr << RED("expected " + type + " with \"" + substring + "\"") << '\n';
    exit(EXIT_FAILURE);
}

#define RUN_TEST_FUNCTION(fn) \
    { run_test_function(fn, #fn); }

#define ASSERT_TRUE(expr) \
    { assert_true((expr), #expr); }

#define ASSERT_FALSE(expr) \
    { assert_false((expr), #expr); }

#define ASSERT_EQUAL(expr, expected) \
    { assert_equal((expr), (expected), #expr, #expected); }

#define ASSERT_TO_STR(expr, expected) \
    { assert_to_str((expr), (expected), #expr); }

#define ASSERT_ITERATOR(expr, str_expected) \
    { assert_iterator((expr), (str_expected), #expr); }

#define ASSERT_PARSE(text, ...) \
    { assert_parse(text, make_list(__VA_ARGS__)); }

#define ASSERT_EXCEPTION(code, type, substring)                         \
    {                                                                   \
        bool thrown = false;                                            \
        try {                                                           \
            code                                                        \
        } catch (type & e) {                                            \
            assert_exception_thrown(#code, #type, e.what(), substring); \
            thrown = true;                                              \
        }                                                               \
        if (!thrown) {                                                  \
            assert_exception_not_thrown(#code, #type, substring);       \
        }                                                               \
    }

#define ASSERT_PARSE_ERROR(text, substring)                                       \
    {                                                                             \
        ASSERT_EXCEPTION({ parse_values_from(text); }, parsing_error, substring); \
    }

void test_value() {
    // number
    shared_ptr<value_number> num1, num2;
    ASSERT_TO_STR(*(num1 = make_number(3.14)), "3.14");
    ASSERT_TRUE(num1->type() == value_t::number);
    ASSERT_EQUAL(num1->number(), 3.14);
    ASSERT_TO_STR(*(num2 = make_number(3.14)), "3.14");
    ASSERT_TRUE(*num1 == *num2);
    ASSERT_FALSE(num1 == num2);

    // symbol
    shared_ptr<value_symbol> sym1, sym2;
    ASSERT_TO_STR(*(sym1 = make_symbol("abc")), "abc");
    ASSERT_TRUE(sym1->type() == value_t::symbol);
    ASSERT_EQUAL(sym1->symbol(), "abc");
    ASSERT_TO_STR(*(sym2 = make_symbol("abc")), "abc");
    ASSERT_TRUE(*sym1 == *sym2);
    ASSERT_TRUE(sym1 == sym2);

    // string
    shared_ptr<value_string> str1, str2;
    ASSERT_TO_STR(*(str1 = make_string("abc")), "\"abc\"");
    ASSERT_TRUE(str1->type() == value_t::string);
    ASSERT_EQUAL(str1->string_(), "abc");
    ASSERT_TO_STR(*(str2 = make_string("abc")), "\"abc\"");
    ASSERT_TRUE(*str1 == *str2);
    ASSERT_FALSE(str1 == str2);

    // error
    shared_ptr<value_error> error1, error2;
    ASSERT_TO_STR(*(error1 = make_error("hello '%g'", 3.14)), BOLD(RED("error:") " " WHITE("hello '3.14'")));
    ASSERT_TRUE(error1->type() == value_t::error);
    ASSERT_EQUAL(error1->string_(), "hello '3.14'");
    ASSERT_TO_STR(*(error2 = make_error("hello '%g'", 3.14)), BOLD(RED("error:") " " WHITE("hello '3.14'")));
    ASSERT_TRUE(*error1 == *error2);
    ASSERT_FALSE(error1 == error2);

    // info
    shared_ptr<value_info> info1, info2;
    ASSERT_TO_STR(*(info1 = make_info("hello '%g'", 3.14)), GREEN("hello '3.14'"));
    ASSERT_TRUE(info1->type() == value_t::info);
    ASSERT_EQUAL(info1->string_(), "hello '3.14'");
    ASSERT_TO_STR(*(info2 = make_info("hello '%g'", 3.14)), GREEN("hello '3.14'"));
    ASSERT_TRUE(*info1 == *info2);
    ASSERT_FALSE(info1 == info2);

    // bool
    shared_ptr<value_bool> bool1, bool2;
    ASSERT_TO_STR(*(bool1 = make_bool(true)), "true");
    ASSERT_TRUE(bool1->type() == value_t::bool_);
    ASSERT_EQUAL(bool1->truth(), true);
    ASSERT_TO_STR(*(bool2 = make_bool(true)), "true");
    ASSERT_TRUE(*bool1 == *bool2);
    ASSERT_TRUE(bool1 == bool2);

    // nil
    shared_ptr<value_nil> nil1, nil2;
    ASSERT_TO_STR(*(nil1 = make_nil()), "()");
    ASSERT_TRUE(nil1->type() == value_t::nil);
    ASSERT_TO_STR(*(nil2 = make_nil()), "()");
    ASSERT_TRUE(*nil1 == *nil2);
    ASSERT_TRUE(nil1 == nil2);

    // pair
    shared_ptr<value_pair> pair1, pair2;
    ASSERT_TO_STR(*(pair1 = make_vpair(3.14, "abc")), "(3.14 . abc)");
    ASSERT_TRUE(pair1->type() == value_t::pair);
    ASSERT_TO_STR(*pair1->car(), "3.14");
    ASSERT_TO_STR(*pair1->cdr(), "abc");
    ASSERT_TO_STR(*(pair2 = make_vpair(3.14, "abc")), "(3.14 . abc)");
    ASSERT_TRUE(*pair1 == *pair2);
    ASSERT_FALSE(pair1 == pair2);

    // list
    shared_ptr<value_pair> list1, list2;
    ASSERT_TO_STR(*(list1 = make_list(3.14, "abc", nil, false)), "(3.14 abc () false)");
    ASSERT_TRUE(list1->type() == value_t::pair);
    ASSERT_TO_STR(*list1->car(), "3.14");
    ASSERT_TO_STR(*list1->cdr(), "(abc () false)");
    ASSERT_TO_STR(*list1->pcdr()->car(), "abc");
    ASSERT_TO_STR(*list1->pcdr()->cdr(), "(() false)");
    ASSERT_TO_STR(*list1->pcdr()->pcdr()->car(), "()");
    ASSERT_TO_STR(*list1->pcdr()->pcdr()->cdr(), "(false)");
    ASSERT_TO_STR(*list1->pcdr()->pcdr()->pcdr()->car(), "false");
    ASSERT_TO_STR(*list1->pcdr()->pcdr()->pcdr()->cdr(), "()");
    ASSERT_TO_STR(*(list2 = make_list(3.14, "abc", nil, false)), "(3.14 abc () false)");
    ASSERT_TRUE(*list1 == *list2);
    ASSERT_FALSE(list1 == list2);
}

void test_pair() {
    // is_list
    ASSERT_TRUE(make_vpair(1, nil)->is_list());
    ASSERT_FALSE(make_vpair(nil, 2)->is_list());
    ASSERT_TRUE(make_vpair(nil, nil)->is_list());
    ASSERT_FALSE(make_vpair(1, 2)->is_list());
    ASSERT_TRUE(make_vpair(1, make_vpair(2, nil))->is_list());
    ASSERT_FALSE(make_vpair(1, make_vpair(2, 3))->is_list());
    ASSERT_TRUE(make_list(1)->is_list());
    ASSERT_TRUE(make_list(1, 2)->is_list());
    ASSERT_TRUE(make_list(1, 2, 3)->is_list());
    ASSERT_TRUE(make_list(1, 2, make_vpair(3, 4))->is_list());
    ASSERT_TRUE(make_list(make_vpair(1, 2), make_vpair(3, 4))->is_list());
    ASSERT_TRUE(make_list(1, 2, nil)->is_list());
    ASSERT_TRUE(make_list(1, nil, nil)->is_list());
    ASSERT_TRUE(make_list(nil, nil, nil)->is_list());

    // length
    ASSERT_EQUAL(make_vpair(1, nil)->length(), 1ull);
    ASSERT_EQUAL(make_vpair(nil, 2)->length(), 2ull);
    ASSERT_EQUAL(make_vpair(nil, nil)->length(), 1ull);
    ASSERT_EQUAL(make_vpair(1, 2)->length(), 2ull);
    ASSERT_EQUAL(make_vpair(1, make_vpair(2, nil))->length(), 2ull);
    ASSERT_EQUAL(make_vpair(1, make_vpair(2, 3))->length(), 3ull);
    ASSERT_EQUAL(make_list(1)->length(), 1ull);
    ASSERT_EQUAL(make_list(1, 2)->length(), 2ull);
    ASSERT_EQUAL(make_list(1, 2, 3)->length(), 3ull);
    ASSERT_EQUAL(make_list(1, 2, make_vpair(3, 4))->length(), 3ull);
    ASSERT_EQUAL(make_list(make_vpair(1, 2), make_vpair(3, 4))->length(), 2ull);
    ASSERT_EQUAL(make_list(1, 2, nil)->length(), 3ull);
    ASSERT_EQUAL(make_list(1, nil, nil)->length(), 3ull);
    ASSERT_EQUAL(make_list(nil, nil, nil)->length(), 3ull);

    // iterator
    ASSERT_ITERATOR(*make_vpair(1, nil), "1");
    ASSERT_ITERATOR(*make_vpair(nil, 2), "(), 2");
    ASSERT_ITERATOR(*make_vpair(nil, nil), "()");
    ASSERT_ITERATOR(*make_vpair(1, 2), "1, 2");
    ASSERT_ITERATOR(*make_vpair(1, make_vpair(2, nil)), "1, 2");
    ASSERT_ITERATOR(*make_vpair(1, make_vpair(2, 3)), "1, 2, 3");
    ASSERT_ITERATOR(*make_vpair(1, "2"), "1, 2");
    ASSERT_ITERATOR(*make_vpair(1, "2"s), "1, \"2\"");
    ASSERT_ITERATOR(*make_list(1), "1");
    ASSERT_ITERATOR(*make_list(1, 2), "1, 2");
    ASSERT_ITERATOR(*make_list(1, 2, 3), "1, 2, 3");
    ASSERT_ITERATOR(*make_list(1, 2, make_vpair(3, 4)), "1, 2, (3 . 4)");
    ASSERT_ITERATOR(*make_list(make_vpair(1, 2), make_vpair(3, 4)), "(1 . 2), (3 . 4)");
    ASSERT_ITERATOR(*make_list(1, 2, nil), "1, 2, ()");
    ASSERT_ITERATOR(*make_list(1, nil, nil), "1, (), ()");
    ASSERT_ITERATOR(*make_list(nil, nil, nil), "(), (), ()");
    ASSERT_ITERATOR(*make_list(1, "2", "3"s, make_vpair(nil, false)), "1, 2, \"3\", (() . false)");

    // iterator mutation
    value_pair val = *make_list(1, 2, 3, 4, 5);
    ASSERT_ITERATOR(val, "1, 2, 3, 4, 5");
    for (value& item : val) {
        // mutate the value in-place with a setter
        auto number = reinterpret_cast<value_number*>(&item);
        number->number(number->number() * number->number());
    }
    ASSERT_ITERATOR(val, "1, 4, 9, 16, 25");
    for (auto p = val.begin(); p != val.end(); p++) {
        // replace the value in the shared_ptr with iterator's ptr()
        auto old_number = to<value_number>(p.ptr());
        auto new_number = make_number(2 * old_number->number());
        p.ptr(new_number);
    }
    ASSERT_ITERATOR(val, "2, 8, 18, 32, 50");

    // cycle
    shared_ptr<value_pair> v1, v2, v3, v4;
    v1 = make_vpair(1, 2);
    ASSERT_EXCEPTION({ v1->car(v1); }, cycle_error, "cycle from (1 . 2)");
    v1 = make_vpair(1, 2);
    v2 = make_vpair(3, 4);
    ASSERT_EXCEPTION({ v1->car(v2); v2->car(v1); }, cycle_error, "cycle from (3 . 4)");
    v1 = make_vpair(1, 2);
    v2 = make_vpair(3, 4);
    ASSERT_EXCEPTION({ v1->car(v2); v2->cdr(v1); }, cycle_error, "cycle from (3 . 4)");
    v1 = make_vpair(1, 2);
    v2 = make_vpair(3, 4);
    ASSERT_EXCEPTION({ v1->cdr(v2); v2->car(v1); }, cycle_error, "cycle from (3 . 4)");
    v1 = make_vpair(1, 2);
    v2 = make_vpair(3, 4);
    ASSERT_EXCEPTION({ v1->cdr(v2); v2->cdr(v1); }, cycle_error, "cycle from (3 . 4)");
    v1 = make_vpair(1, 2);
    v2 = make_vpair(3, 4);
    v3 = make_vpair(5, 6);
    ASSERT_EXCEPTION({ v1->car(v2); v2->car(v3); v3->car(v1); }, cycle_error, "cycle from (5 . 6)");
    v1 = make_vpair(1, 2);
    v2 = make_vpair(3, 4);
    v3 = make_vpair(5, 6);
    ASSERT_EXCEPTION({ v1->cdr(v2); v2->car(v3); v3->cdr(v1); }, cycle_error, "cycle from (5 . 6)");
    v1 = make_vpair(1, 2);
    v2 = make_vpair(3, 4);
    v3 = make_vpair(5, 6);
    ASSERT_EXCEPTION({ v1->car(v2); v2->cdr(v3); v3->car(v1); }, cycle_error, "cycle from (5 . 6)");
    v1 = make_vpair(1, 2);
    v2 = make_vpair(3, 4);
    v3 = make_vpair(5, 6);
    ASSERT_EXCEPTION({ v1->cdr(v2); v2->cdr(v3); v3->cdr(v1); }, cycle_error, "cycle from (5 . 6)");
    v1 = make_vpair(1, 2);
    v2 = make_vpair(3, 4);
    v3 = make_vpair(5, 6);
    v4 = make_vpair(7, 8);
    ASSERT_EXCEPTION({ v1->car(v2); v2->car(v3); v3->car(v4); v4->car(v1); }, cycle_error, "cycle from (7 . 8)");
    v1 = make_vpair(1, 2);
    v2 = make_vpair(3, 4);
    v3 = make_vpair(5, 6);
    v4 = make_vpair(7, 8);
    ASSERT_EXCEPTION({ v1->car(v2); v2->cdr(v3); v3->car(v4); v4->cdr(v1); }, cycle_error, "cycle from (7 . 8)");
    v1 = make_vpair(1, 2);
    v2 = make_vpair(3, 4);
    v3 = make_vpair(5, 6);
    v4 = make_vpair(7, 8);
    ASSERT_EXCEPTION({ v1->cdr(v2); v2->car(v3); v3->cdr(v4); v4->car(v1); }, cycle_error, "cycle from (7 . 8)");
    v1 = make_vpair(1, 2);
    v2 = make_vpair(3, 4);
    v3 = make_vpair(5, 6);
    v4 = make_vpair(7, 8);
    ASSERT_EXCEPTION({ v1->cdr(v2); v2->cdr(v3); v3->cdr(v4); v4->cdr(v1); }, cycle_error, "cycle from (7 . 8)");
}

void test_equal() {
    // number
    shared_ptr<value> num1, num2, num3;
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
    shared_ptr<value> sym1, sym2, sym3;
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
    shared_ptr<value> str1, str2, str3;
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
    shared_ptr<value> bool1, bool2, bool3;
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
    shared_ptr<value> nil1, nil2, nil3;
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
    shared_ptr<value> pair1, pair2, pair3;
    ASSERT_TO_STR(*(pair1 = make_vpair(3.14, "abc")), "(3.14 . abc)");
    ASSERT_TO_STR(*(pair2 = make_vpair(3.14, "abc")), "(3.14 . abc)");
    ASSERT_TO_STR(*(pair3 = make_vpair(3.14, "abc"s)), "(3.14 . \"abc\")");

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
    shared_ptr<value> list1, list2, list3;
    ASSERT_TO_STR(*(list1 = make_list(1, "2", make_vpair("3"s, 4))), "(1 2 (\"3\" . 4))");
    ASSERT_TO_STR(*(list2 = make_list(1, "2", make_vpair("3"s, 4))), "(1 2 (\"3\" . 4))");
    ASSERT_TO_STR(*(list3 = make_list(1, "2", "3"s, 4)), "(1 2 \"3\" 4)");

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
    ASSERT_TO_STR(*make_vpair(1, 1), "(1 . 1)");
    ASSERT_TO_STR(*make_vpair(1, 2), "(1 . 2)");
    ASSERT_TO_STR(*make_vpair(1, "abc"), "(1 . abc)");
    ASSERT_TO_STR(*make_vpair("xyz"s, 3.14), "(\"xyz\" . 3.14)");
    ASSERT_TO_STR(*make_vpair(true_, false_), "(true . false)");
    ASSERT_TO_STR(*make_vpair(1, nil), "(1)");
    ASSERT_TO_STR(*make_vpair(nil, 2), "(() . 2)");
    ASSERT_TO_STR(*make_vpair(nil, nil), "(())");
    ASSERT_TO_STR(*make_vpair("quote", 1), "(quote . 1)");
    ASSERT_TO_STR(*make_vpair("quote", nil), "(quote)");
    ASSERT_TO_STR(*make_vpair(1, "quote"), "(1 . quote)");
    ASSERT_TO_STR(*make_vpair(nil, "quote"), "(() . quote)");
    ASSERT_TO_STR(*make_vpair("quote", "quote"), "(quote . quote)");

    // list
    ASSERT_TO_STR(*make_list(1), "(1)");
    ASSERT_TO_STR(*make_list(1, 2, 3), "(1 2 3)");
    ASSERT_TO_STR(*make_list(3.14, "abc", "xyz"s, true_, nil, false_), "(3.14 abc \"xyz\" true () false)");
    ASSERT_TO_STR(*make_list(1, 2, make_list(3, 4, 5), 6), "(1 2 (3 4 5) 6)");
    ASSERT_TO_STR(*make_list(1, 2, make_vpair(3, 4)), "(1 2 (3 . 4))");
    ASSERT_TO_STR(*make_vpair(1, make_vpair(2, make_vpair(3, 4))), "(1 2 3 . 4)");
    ASSERT_TO_STR(*make_list(make_list(make_list(1))), "(((1)))");
    ASSERT_TO_STR(*make_list("quote"), "(quote)");
    ASSERT_TO_STR(*make_list("quote", 1), "'1");
    ASSERT_TO_STR(*make_list("quote", 1, 2), "(quote 1 2)");
    ASSERT_TO_STR(*make_list("quote", make_vpair(1, 2)), "'(1 . 2)");
    ASSERT_TO_STR(*make_list("quote", make_vpair(1, 2), 3), "(quote (1 . 2) 3)");
    ASSERT_TO_STR(*make_list("quote", make_list(1, 2, 3)), "'(1 2 3)");
    ASSERT_TO_STR(*make_list("quote", make_list(1, 2, 3), make_list(4, 5)), "(quote (1 2 3) (4 5))");
    ASSERT_TO_STR(*make_list("quote", make_list("quote", 1)), "''1");
    ASSERT_TO_STR(*make_list("quote", make_list("quote", make_list("quote", 1))), "'''1");
}

void test_parse() {
    // nil
    ASSERT_PARSE("()", nil);
    ASSERT_PARSE("(  )", nil);
    ASSERT_PARSE("  (  )   ", nil);
    ASSERT_PARSE("()()", nil, nil);
    ASSERT_PARSE("() ()", nil, nil);
    ASSERT_PARSE(" ()  ()  ", nil, nil);
    ASSERT_PARSE("(()())", make_list(nil, nil));
    ASSERT_PARSE("(() ((()) () ())) ()", make_list(nil, make_list(make_list(nil), nil, nil)), nil);
    ASSERT_PARSE(" ( () ( (( )) ()  () ) )  () ", make_list(nil, make_list(make_list(nil), nil, nil)), nil);

    // integers
    ASSERT_PARSE("1", 1);
    ASSERT_PARSE("1 2", 1, 2);
    ASSERT_PARSE("123", 123);
    ASSERT_PARSE("123456789", 123456789);
    ASSERT_PARSE("1234 56789", 1234, 56789);
    ASSERT_PARSE("(1)", make_list(1));
    ASSERT_PARSE("(1 2)", make_list(1, 2));
    ASSERT_PARSE("((1))", make_list(make_list(1)));
    ASSERT_PARSE("((1) (2))", make_list(make_list(1), make_list(2)));
    ASSERT_PARSE("(1 () 2)", make_list(1, nil, 2));
    ASSERT_PARSE("1 2 3", 1, 2, 3);
    ASSERT_PARSE("(1 2 3)", make_list(1, 2, 3));
    ASSERT_PARSE("  (  1    2 3 )  ", make_list(1, 2, 3));
    ASSERT_PARSE("1 (2 3 (4) 5 ((6 7) 8 9) 10)",
                 1, make_list(2, 3, make_list(4), 5, make_list(make_list(6, 7), 8, 9), 10));
    ASSERT_PARSE("  1  (  2 3 (4  ) 5 (( 6  7 )))",
                 1, make_list(2, 3, make_list(4), 5, make_list(make_list(6, 7))));
    ASSERT_PARSE("1 (23 (4) 5 ((67) 89) 10)",
                 1, make_list(23, make_list(4), 5, make_list(make_list(67), 89), 10));

    // decimals
    ASSERT_PARSE(".456 1. 2.0 3.14 -4. -5.67 -.123", 0.456, 1, 2, 3.14, -4, -5.67, -0.123);
    ASSERT_PARSE("1e10 1e2 1e1 1e0 1e-1 1e-2 1e-10", 1e10, 1e2, 1e1, 1e0, 1e-1, 1e-2, 1e-10);

    // symbols
    ASSERT_PARSE("x", "x");
    ASSERT_PARSE("x y", "x", "y");
    ASSERT_PARSE("abc xyz", "abc", "xyz");
    ASSERT_PARSE("!?_ */- \\%^", "!?_", "*/-", "\\%^");
    ASSERT_PARSE("x y z", "x", "y", "z");
    ASSERT_PARSE("x 1 y 2 z", "x", 1, "y", 2, "z");
    ASSERT_PARSE("xyz a1 abc 2b def", "xyz", "a1", "abc", "2b", "def");

    // quote
    ASSERT_PARSE("'1", make_list("quote", 1));
    ASSERT_PARSE("  '1", make_list("quote", 1));
    ASSERT_PARSE("  '  1", make_list("quote", 1));
    ASSERT_PARSE("  '  1  ", make_list("quote", 1));
    ASSERT_PARSE("'1 2", make_list("quote", 1), 2);
    ASSERT_PARSE("1 '2", 1, make_list("quote", 2));
    ASSERT_PARSE("'1 '2", make_list("quote", 1), make_list("quote", 2));
    ASSERT_PARSE("''1", make_list("quote", make_list("quote", 1)));
    ASSERT_PARSE("'''1", make_list("quote", make_list("quote", make_list("quote", 1))));
    ASSERT_PARSE("1 2 ' (3 4 (5 6 7))", 1, 2, make_list("quote", make_list(3, 4, make_list(5, 6, 7))));
    ASSERT_PARSE("'1 2 ' (3 4 (5 '6 7))",
                 make_list("quote", 1), 2,
                 make_list("quote", make_list(3, 4, make_list(5, make_list("quote", 6), 7))));
    ASSERT_PARSE("1 2 '(3 4 '(5 6 7))",
                 1, 2, make_list("quote", make_list(3, 4, make_list("quote", make_list(5, 6, 7)))));
    ASSERT_PARSE("''(1 2 3)", make_list("quote", make_list("quote", make_list(1, 2, 3))));

    // dot
    ASSERT_PARSE(".", ".");
    ASSERT_PARSE(". . . . .", ".", ".", ".", ".", ".");
    ASSERT_PARSE("1 . 2 . 3", 1, ".", 2, ".", 3);
    ASSERT_PARSE("(1 2) . 3 . (4 5)", make_list(1, 2), ".", 3, ".", make_list(4, 5));
    ASSERT_PARSE("(1 . 2)", make_vpair(1, 2));
    ASSERT_PARSE("(1 2 . 3)", make_vpair(1, make_vpair(2, 3)));
    ASSERT_PARSE("(1 (2 . 3) . 4)", make_vpair(1, make_vpair(make_vpair(2, 3), 4)));
    ASSERT_PARSE("(1 (2 (3 . 4) . 5) . 6)",
                 make_vpair(1, make_vpair(make_vpair(2, make_vpair(make_vpair(3, 4), 5)), 6)));
    ASSERT_PARSE("(1 (2 . 3))", make_list(1, make_vpair(2, 3)));
    ASSERT_PARSE("((1 2) . 3)", make_vpair(make_list(1, 2), 3));
    ASSERT_PARSE("((1 . 2) . 3)", make_vpair(make_vpair(1, 2), 3));
    ASSERT_PARSE("(1 . (2 3))", make_vpair(1, make_list(2, 3)));
    ASSERT_PARSE("(1 2 3 . (4 5 6))", make_list(1, 2, 3, 4, 5, 6));
    ASSERT_PARSE("(. 1)", 1);
    ASSERT_PARSE("(. (1 2))", make_list(1, 2));
    ASSERT_PARSE("(. (1 . 2))", make_vpair(1, 2));
    ASSERT_PARSE("(.1 . 2.)", make_vpair(0.1, 2));
    ASSERT_PARSE("(1.2 . 3.4)", make_vpair(1.2, 3.4));
    ASSERT_PARSE("(1 2 3 . 4)", make_vpair(1, make_vpair(2, make_vpair(3, 4))));
    ASSERT_PARSE("'(1 . 2)", make_list("quote", make_vpair(1, 2)));
    ASSERT_PARSE("'(. 2)", make_list("quote", 2));

    // comment
    ASSERT_PARSE("(1 2 3); comment", make_list(1, 2, 3));
    ASSERT_PARSE("(1 2 3); (4 5 \" 6 7", make_list(1, 2, 3));
    ASSERT_PARSE("(1 2 3)   ;   comment  ", make_list(1, 2, 3));
    ASSERT_PARSE("\n(1 2 3)   ;   comment  \n  (4 5)", make_list(1, 2, 3), make_list(4, 5));
    ASSERT_PARSE(" ; comment\n(1 2 3)   ;   comment  \n  (4 5)", make_list(1, 2, 3), make_list(4, 5));

    // string
    ASSERT_PARSE("\"\"", ""s);
    ASSERT_PARSE("\"abc\"", "abc"s);
    ASSERT_PARSE("\"abc def\"", "abc def"s);
    ASSERT_PARSE("\"  abc   def   \"", "  abc   def   "s);
    ASSERT_PARSE("\"  abc \n  def  \n \"", "  abc \n  def  \n "s);
    ASSERT_PARSE("\"x\" \"y\" \"z\"", "x"s, "y"s, "z"s);
    ASSERT_PARSE("\"a\\tb\"", "a\\tb"s);
    ASSERT_PARSE("\"a\tb\"", "a\tb"s);
    ASSERT_PARSE("\"a\\nb\"", "a\\nb"s);
    ASSERT_PARSE("\"\na\nb\"", "\na\nb"s);
    ASSERT_PARSE("'\"abc\"", make_list("quote", "abc"s));
    ASSERT_PARSE("\"'abc\"", "'abc"s);
    ASSERT_PARSE("'\"x\" \"y\" \"z\"", make_list("quote", "x"s), "y"s, "z"s);
    ASSERT_PARSE("'(\"x\" \"y\" \"z\")", make_list("quote", make_list("x"s, "y"s, "z"s)));

    // special symbols
    ASSERT_PARSE("nil", nil);
    ASSERT_PARSE("true", true_);
    ASSERT_PARSE("false", false_);
    ASSERT_PARSE("#t", true_);
    ASSERT_PARSE("#f", false_);

    // parsing errors
    ASSERT_PARSE_ERROR("(1 2", "unterminated list");
    ASSERT_PARSE_ERROR("1 2)", "premature end of list");
    ASSERT_PARSE_ERROR("( 1 (2", "unterminated list");
    ASSERT_PARSE_ERROR("'", "unfollowed quote");
    ASSERT_PARSE_ERROR("'  a '  ", "unfollowed quote");
    ASSERT_PARSE_ERROR("(1 .)", "unfollowed . in (1 .)");
    ASSERT_PARSE_ERROR("(1 2 3 .)", "unfollowed . in (1 2 3 .)");
    ASSERT_PARSE_ERROR("(1 . 2 3)", "2+ items after . in (1 . 2 3)");
    ASSERT_PARSE_ERROR("(. 2 3)", "2+ items after . in (. 2 3)");
    ASSERT_PARSE_ERROR("(1 . .)", ". followed by . in (1 . .)");
    ASSERT_PARSE_ERROR("(. . 1)", "2+ items after . in (. . 1)");
    ASSERT_PARSE_ERROR("(. .)", ". followed by . in (. .)");
    ASSERT_PARSE_ERROR("(. . . .)", "2+ items after . in (. . . .)");
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

void test_code() {
    // reconstruct the machines' code
    path machines{"./lib/machines"};
    for (auto& code_file : directory_iterator(machines)) {
        report_test(code_file.path());

        // parse and translate the code from the file
        auto source = parse_values_from(code_file.path());
        auto code = translate_to_code(source);

        auto running = source;
        for (const auto& translated : code) {
            // compare each line of the code with the original
            const auto& original_line = running->car();
            const shared_ptr<value> translated_line = translated->to_value();

            if (*translated_line != *original_line) {
                cerr << RED("" + translated_line->str() + " != " + original_line->str() + "") << '\n';
                exit(EXIT_FAILURE);
            }

            running = running->pcdr();
        }
    }
}

void test_machine() {
    // data for the machine unit tests
    vector<tuple<path, string, vector<pair<vector<pair<string, shared_ptr<value>>>, shared_ptr<value>>>>> data = {
        {
            "./lib/machines/gcd.scm",
            "a",
            {
                {{{"a", make_number(24)}, {"b", make_number(36)}}, make_number(12)},
                {{{"a", make_number(9)}, {"b", make_number(16)}}, make_number(1)},
                {{{"a", make_number(10)}, {"b", make_number(10)}}, make_number(10)},
                {{{"a", make_number(72)}, {"b", make_number(54)}}, make_number(18)},
                {{{"a", make_number(5)}, {"b", make_number(125)}}, make_number(5)},
            },
        },
        {
            "./lib/machines/factorial.scm",
            "val",
            {
                {{{"n", make_number(1)}}, make_number(1)},
                {{{"n", make_number(2)}}, make_number(2)},
                {{{"n", make_number(3)}}, make_number(6)},
                {{{"n", make_number(5)}}, make_number(120)},
                {{{"n", make_number(7)}}, make_number(5040)},
                {{{"n", make_number(10)}}, make_number(3628800)},
            },
        },
        {
            "./lib/machines/fibonacci.scm",
            "val",
            {
                {{{"n", make_number(0)}}, make_number(0)},
                {{{"n", make_number(1)}}, make_number(1)},
                {{{"n", make_number(2)}}, make_number(1)},
                {{{"n", make_number(5)}}, make_number(5)},
                {{{"n", make_number(8)}}, make_number(21)},
                {{{"n", make_number(10)}}, make_number(55)},
            },
        },
    };

    // primitives to bind for the tests

    auto add = [](const vector<shared_ptr<value_pair>>& args) -> shared_ptr<value> {
        return make_number(
            to<value_number>(args[0]->car())->number() +
            to<value_number>(args[1]->car())->number());
    };

    auto subtract = [](const vector<shared_ptr<value_pair>>& args) -> shared_ptr<value> {
        return make_number(
            to<value_number>(args[0]->car())->number() -
            to<value_number>(args[1]->car())->number());
    };

    auto multiply = [](const vector<shared_ptr<value_pair>>& args) -> shared_ptr<value> {
        return make_number(
            to<value_number>(args[0]->car())->number() *
            to<value_number>(args[1]->car())->number());
    };

    auto remainder = [](const vector<shared_ptr<value_pair>>& args) -> shared_ptr<value> {
        return make_number(std::fmod(
            to<value_number>(args[0]->car())->number(),
            to<value_number>(args[1]->car())->number()));
    };

    auto equal = [](const vector<shared_ptr<value_pair>>& args) -> shared_ptr<value> {
        return (to<value_number>(args[0]->car())->number() ==
                to<value_number>(args[1]->car())->number())
                   ? true_
                   : false_;
    };

    auto less = [](const vector<shared_ptr<value_pair>>& args) -> shared_ptr<value> {
        return (to<value_number>(args[0]->car())->number() <
                to<value_number>(args[1]->car())->number())
                   ? true_
                   : false_;
    };

    for (const auto& [path, output_register, test_cases] : data) {
        // create the machine from source
        shared_ptr<value_pair> source = parse_values_from(path);
        std::vector<shared_ptr<code>> code = translate_to_code(source);
        machine m{code};

        // bind the primitives
        m.bind_op("+", add);
        m.bind_op("-", subtract);
        m.bind_op("*", multiply);
        m.bind_op("rem", remainder);
        m.bind_op("=", equal);
        m.bind_op("<", less);

        for (const auto& [inputs, expected] : test_cases) {
            auto result = m.run(inputs, output_register);

            // contruct the report line
            ostringstream s;
            s << path.filename().stem().string() << "(";  // filename
            for (const auto& p : inputs) {
                s << *p.second << ", ";  // input
            }
            s.seekp(-2, s.cur);      // drop trailing ", "
            s << ") = " << *result;  // output
            report_test(s.str());

            if (*result != *expected) {
                cerr << RED("expected \"" + expected->str() + "\"") << '\n';
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main() {
    RUN_TEST_FUNCTION(test_value);
    RUN_TEST_FUNCTION(test_pair);
    RUN_TEST_FUNCTION(test_equal);
    RUN_TEST_FUNCTION(test_to_str);
    RUN_TEST_FUNCTION(test_parse);
    RUN_TEST_FUNCTION(test_code);
    RUN_TEST_FUNCTION(test_machine);

    cout << "all tests have been passed!\n";

    return EXIT_SUCCESS;
}
