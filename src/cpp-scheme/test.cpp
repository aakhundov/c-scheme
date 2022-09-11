#include <iomanip>
#include <iostream>
#include <sstream>

#include "value.hpp"

using namespace std::literals;

static size_t test_counter = 0;

void report_test(std::string message) {
    std::cout << "\x1B[34m"
              << std::setfill('0') << std::setw(5) << ++test_counter
              << "\x1B[0m " << message << std::endl;
}

#define run_test_function(fn)                                       \
    {                                                               \
        std::cout << "[" << #fn << "]" << std::endl;                \
        std::cout << "==============================" << std::endl; \
        fn();                                                       \
        std::cout << std::endl;                                     \
    }

#define assert_true(expr)                                                           \
    {                                                                               \
        report_test(                                                                \
            "\x1B[34m[\x1B[0m" #expr "\x1B[34m] --> [\x1B[0mtrue\x1B[34m]\x1B[0m"); \
        if (!expr) {                                                                \
            std::cout << "expected true" << std::endl;                              \
            exit(1);                                                                \
        }                                                                           \
    }

#define assert_false(expr)                                                           \
    {                                                                                \
        report_test(                                                                 \
            "\x1B[34m[\x1B[0m" #expr "\x1B[34m] --> [\x1B[0mfalse\x1B[34m]\x1B[0m"); \
        if (expr) {                                                                  \
            std::cout << "expected false" << std::endl;                              \
            exit(1);                                                                 \
        }                                                                            \
    }

#define assert_equal(expr, expected)                                                              \
    {                                                                                             \
        auto result = expr;                                                                       \
        std::stringstream buffer;                                                                 \
        buffer << result;                                                                         \
        std::string str_result = buffer.str();                                                    \
        report_test(                                                                              \
            "\x1B[34m[\x1B[0m" #expr "\x1B[34m] --> [\x1B[0m" + str_result + "\x1B[34m]\x1B[0m"); \
        if (result != expected) {                                                                 \
            std::cout << "expected: " << expected << std::endl;                                   \
            exit(1);                                                                              \
        }                                                                                         \
    }

#define assert_to_str(expr, expected)                                                             \
    {                                                                                             \
        std::stringstream buffer;                                                                 \
        buffer << expr;                                                                           \
        std::string str_result = buffer.str();                                                    \
        report_test(                                                                              \
            "\x1B[34m[\x1B[0m" #expr "\x1B[34m] --> [\x1B[0m" + str_result + "\x1B[34m]\x1B[0m"); \
        if (str_result != expected) {                                                             \
            std::cout << "expected: " << expected << std::endl;                                   \
            exit(1);                                                                              \
        }                                                                                         \
    }

#define assert_iterator(expr, expected)                                                          \
    {                                                                                            \
        value_pair pair = expr;                                                                  \
        std::string str_items;                                                                   \
        for (auto& item : pair) {                                                                \
            std::stringstream buffer;                                                            \
            buffer << *item;                                                                     \
            str_items += (buffer.str() + ", ");                                                  \
        }                                                                                        \
        str_items.pop_back();                                                                    \
        str_items.pop_back();                                                                    \
        report_test(                                                                             \
            "\x1B[34m[\x1B[0m" #expr "\x1B[34m] --> [\x1B[0m" + str_items + "\x1B[34m]\x1B[0m"); \
        if (str_items != expected) {                                                             \
            std::cout << "expected: \"" << expected << "\"" << std::endl;                        \
            exit(1);                                                                             \
        }                                                                                        \
    }

void test_to_str() {
    // number
    assert_to_str(*make_value(0), "0");
    assert_to_str(*make_value(1), "1");
    assert_to_str(*make_value(-1), "-1");
    assert_to_str(*make_value(3.14), "3.14");
    assert_to_str(*make_value(-3.14), "-3.14");
    assert_to_str(*make_value(1e-20), "1e-20");
    assert_to_str(*make_value(1e+20), "1e+20");
    assert_to_str(*make_value(3.14e2), "314");
    assert_to_str(*make_value(3.14e-2), "0.0314");
    assert_to_str(*make_value(123'456'789'012), "123456789012");
    assert_to_str(*make_value(1'234'567'890'123), "1.23456789012e+12");
    assert_to_str(*make_value(123'456.789'012), "123456.789012");
    assert_to_str(*make_value(123'456.789'012'3), "123456.789012");
    assert_to_str(*make_value(0.123'456'789'012), "0.123456789012");
    assert_to_str(*make_value(0.123'456'789'012'3), "0.123456789012");

    // symbol
    assert_to_str(*make_value(""), "");
    assert_to_str(*make_value("a"), "a");
    assert_to_str(*make_value("abc"), "abc");
    assert_to_str(*make_value("xxx"), "xxx");
    assert_to_str(*make_value("!@#$%"), "!@#$%");

    // string
    assert_to_str(*make_value(""s), "\"\"");
    assert_to_str(*make_value("a"s), "\"a\"");
    assert_to_str(*make_value("abc"s), "\"abc\"");
    assert_to_str(*make_value("xxx"s), "\"xxx\"");
    assert_to_str(*make_value("!@#$%"s), "\"!@#$%\"");

    // error
    assert_to_str(*make_error(""), "\x1B[1;31merror:\x1B[1;37m \x1B[0m");
    assert_to_str(*make_error("message"), "\x1B[1;31merror:\x1B[1;37m message\x1B[0m");
    assert_to_str(*make_error("hello world"), "\x1B[1;31merror:\x1B[1;37m hello world\x1B[0m");
    assert_to_str(*make_error("hello '%s'", "world"), "\x1B[1;31merror:\x1B[1;37m hello 'world'\x1B[0m");
    assert_to_str(*make_error("hello '%g'", 3.14), "\x1B[1;31merror:\x1B[1;37m hello '3.14'\x1B[0m");
    assert_to_str(*make_error("hi %d, %d, %d bye", 1, 2, 3), "\x1B[1;31merror:\x1B[1;37m hi 1, 2, 3 bye\x1B[0m");

    // info
    assert_to_str(*make_info(""), "\x1B[32m\x1B[0m");
    assert_to_str(*make_info("message"), "\x1B[32mmessage\x1B[0m");
    assert_to_str(*make_info("hello world"), "\x1B[32mhello world\x1B[0m");
    assert_to_str(*make_info("hello '%s'", "world"), "\x1B[32mhello 'world'\x1B[0m");
    assert_to_str(*make_info("hello '%g'", 3.14), "\x1B[32mhello '3.14'\x1B[0m");
    assert_to_str(*make_info("hi %d, %d, %d bye", 1, 2, 3), "\x1B[32mhi 1, 2, 3 bye\x1B[0m");

    // bool
    assert_to_str(*true_, "true");
    assert_to_str(*make_value(true), "true");
    assert_to_str(*false_, "false");
    assert_to_str(*make_value(false), "false");

    // bool
    assert_to_str(*nil, "()");
    assert_to_str(*make_nil(), "()");

    // identity
    assert_to_str(*make_value(make_value(1)), "1");
    assert_to_str(*make_value(make_value("abc")), "abc");
    assert_to_str(*make_value(make_value("abc"s)), "\"abc\"");
    assert_to_str(*make_value(true_), "true");
    assert_to_str(*make_value(nil), "()");

    // pair
    assert_to_str(*make_value_pair(1, 1), "(1 . 1)");
    assert_to_str(*make_value_pair(1, 2), "(1 . 2)");
    assert_to_str(*make_value_pair(1, "abc"), "(1 . abc)");
    assert_to_str(*make_value_pair("xyz"s, 3.14), "(\"xyz\" . 3.14)");
    assert_to_str(*make_value_pair(true_, false_), "(true . false)");
    assert_to_str(*make_value_pair(1, nil), "(1)");
    assert_to_str(*make_value_pair(nil, 2), "(() . 2)");
    assert_to_str(*make_value_pair(nil, nil), "(())");

    // list
    assert_to_str(*make_value_list(1), "(1)");
    assert_to_str(*make_value_list(1, 2, 3), "(1 2 3)");
    assert_to_str(*make_value_list(3.14, "abc", "xyz"s, true_, nil, false_), "(3.14 abc \"xyz\" true () false)");
    assert_to_str(*make_value_list(1, 2, make_value_list(3, 4, 5), 6), "(1 2 (3 4 5) 6)");
    assert_to_str(*make_value_list(1, 2, make_value_pair(3, 4)), "(1 2 (3 . 4))");
    assert_to_str(*make_value_pair(1, make_value_pair(2, make_value_pair(3, 4))), "(1 2 3 . 4)");
    assert_to_str(*make_value_list(make_value_list(make_value_list(1))), "(((1)))");
}

void test_pair() {
    // is_list
    assert_true(make_value_pair(1, nil)->is_list());
    assert_false(make_value_pair(nil, 2)->is_list());
    assert_true(make_value_pair(nil, nil)->is_list());
    assert_false(make_value_pair(1, 2)->is_list());
    assert_true(make_value_pair(1, make_value_pair(2, nil))->is_list());
    assert_false(make_value_pair(1, make_value_pair(2, 3))->is_list());
    assert_true(make_value_list(1)->is_list());
    assert_true(make_value_list(1, 2)->is_list());
    assert_true(make_value_list(1, 2, 3)->is_list());
    assert_true(make_value_list(1, 2, make_value_pair(3, 4))->is_list());
    assert_true(make_value_list(make_value_pair(1, 2), make_value_pair(3, 4))->is_list());
    assert_true(make_value_list(1, 2, nil)->is_list());
    assert_true(make_value_list(1, nil, nil)->is_list());
    assert_true(make_value_list(nil, nil, nil)->is_list());

    // length
    assert_equal(make_value_pair(1, nil)->length(), 1);
    assert_equal(make_value_pair(nil, 2)->length(), 2);
    assert_equal(make_value_pair(nil, nil)->length(), 1);
    assert_equal(make_value_pair(1, 2)->length(), 2);
    assert_equal(make_value_pair(1, make_value_pair(2, nil))->length(), 2);
    assert_equal(make_value_pair(1, make_value_pair(2, 3))->length(), 3);
    assert_equal(make_value_list(1)->length(), 1);
    assert_equal(make_value_list(1, 2)->length(), 2);
    assert_equal(make_value_list(1, 2, 3)->length(), 3);
    assert_equal(make_value_list(1, 2, make_value_pair(3, 4))->length(), 3);
    assert_equal(make_value_list(make_value_pair(1, 2), make_value_pair(3, 4))->length(), 2);
    assert_equal(make_value_list(1, 2, nil)->length(), 3);
    assert_equal(make_value_list(1, nil, nil)->length(), 3);
    assert_equal(make_value_list(nil, nil, nil)->length(), 3);

    // iterator
    assert_iterator(*make_value_pair(1, nil), "1");
    assert_iterator(*make_value_pair(nil, 2), "(), 2");
    assert_iterator(*make_value_pair(nil, nil), "()");
    assert_iterator(*make_value_pair(1, 2), "1, 2");
    assert_iterator(*make_value_pair(1, make_value_pair(2, nil)), "1, 2");
    assert_iterator(*make_value_pair(1, make_value_pair(2, 3)), "1, 2, 3");
    assert_iterator(*make_value_list(1), "1");
    assert_iterator(*make_value_list(1, 2), "1, 2");
    assert_iterator(*make_value_list(1, 2, 3), "1, 2, 3");
    assert_iterator(*make_value_list(1, 2, make_value_pair(3, 4)), "1, 2, (3 . 4)");
    assert_iterator(*make_value_list(make_value_pair(1, 2), make_value_pair(3, 4)), "(1 . 2), (3 . 4)");
    assert_iterator(*make_value_list(1, 2, nil), "1, 2, ()");
    assert_iterator(*make_value_list(1, nil, nil), "1, (), ()");
    assert_iterator(*make_value_list(nil, nil, nil), "(), (), ()");

    // iterator mutation
    value_pair val = *make_value_list(1, 2, 3, 4, 5);
    assert_iterator(val, "1, 2, 3, 4, 5");
    for (auto& item : val) {
        auto number = reinterpret_cast<value_number*>(item.get());
        item = make_value(number->number() * number->number());
    }
    assert_iterator(val, "1, 4, 9, 16, 25");
}

void prev() {
    auto num = make_value(1e+20);
    auto sym = make_value("abc");
    auto str = make_value("xyz"s);
    auto bool_ = make_value(true);
    auto pair = make_value_pair(num, sym);
    auto empty = make_value_list(nil);
    auto list1 = make_value_list(1, 2, 3);
    auto list2 = make_value_list(1e+20, "abc", "xyz"s, false, make_value_list(num, sym, 0, 1, 2, 3), 4);
    auto list3 = make_value_list(num, sym, str, bool_, "abc", "abc", "def", "def", "xyz"s, "zyx"s, "zyx"s);
    auto error = make_error("[%g]", 3.14);
    auto info = make_info("[%g]", 3.14);
    auto list4 = make_value_list(sym, str, error, info, nil);
    auto nested = make_value_pair(1, make_value_pair("abc", make_value_pair(3, "xxx")));
    auto nested2 = make_value_list(1, 2, make_value_pair(3, 4));

    std::shared_ptr<value_pair> external = nullptr;

    {
        auto x = make_value(true);
        auto y = make_value(false);
        auto z = make_value(3.14);

        external = make_value_pair(x, y);
    }

    std::cout << nil << " " << *nil << std::endl;
    std::cout << num << " " << *num << std::endl;
    std::cout << sym << " " << *sym << std::endl;
    std::cout << str << " " << *str << std::endl;
    std::cout << bool_ << " " << *bool_ << std::endl;
    std::cout << pair << " " << *pair << std::endl;
    std::cout << empty << " " << *empty << std::endl;
    std::cout << list1 << " " << *list1 << std::endl;
    std::cout << list2 << " " << *list2 << std::endl;
    std::cout << list3 << " " << *list3 << std::endl;
    std::cout << external << " " << *external << std::endl;
    std::cout << error << " " << *error << std::endl;
    std::cout << info << " " << *info << std::endl;
    std::cout << list4 << " " << *list4 << std::endl;
    std::cout << nested << " " << *nested << std::endl;
    std::cout << nested2 << " " << *nested2 << std::endl;
    std::cout << std::endl;

    std::cout << *pair << " is " << (pair->is_list() ? "" : "not ") << "a list" << std::endl;
    std::cout << *external << " is " << (external->is_list() ? "" : "not ") << "a list" << std::endl;
    std::cout << *list1 << " is " << (list1->is_list() ? "" : "not ") << "a list" << std::endl;
    std::cout << *list3 << " is " << (list2->is_list() ? "" : "not ") << "a list" << std::endl;
    std::cout << *list2 << " is " << (list3->is_list() ? "" : "not ") << "a list" << std::endl;
    std::cout << *list4 << " is " << (list4->is_list() ? "" : "not ") << "a list" << std::endl;
    std::cout << *nested << " is " << (nested->is_list() ? "" : "not ") << "a list" << std::endl;
    std::cout << *nested2 << " is " << (nested2->is_list() ? "" : "not ") << "a list" << std::endl;
    std::cout << std::endl;

    std::cout << "len[ " << *pair << " ] = " << pair->length() << std::endl;
    std::cout << "len[ " << *external << " ] = " << external->length() << std::endl;
    std::cout << "len[ " << *list1 << " ] = " << list1->length() << std::endl;
    std::cout << "len[ " << *list2 << " ] = " << list2->length() << std::endl;
    std::cout << "len[ " << *list3 << " ] = " << list3->length() << std::endl;
    std::cout << "len[ " << *list4 << " ] = " << list4->length() << std::endl;
    std::cout << "len[ " << *nested << " ] = " << nested->length() << std::endl;
    std::cout << "len[ " << *nested2 << " ] = " << nested2->length() << std::endl;
    std::cout << std::endl;

    std::cout << "list2" << std::endl;
    for (const auto& v : *list2) {
        std::cout << v << " -> " << *v << std::endl;
    }
    std::cout << std::endl;

    for (auto& v : *list2) {
        v = make_value("xxx");
    }

    std::cout << "list2 (after mutation)" << std::endl;
    for (const auto& v : *list2) {
        std::cout << v << " -> " << *v << std::endl;
    }
    std::cout << std::endl;

    std::cout << "nested" << std::endl;
    for (const auto& v : *nested) {
        std::cout << v << " -> " << *v << std::endl;
    }
    std::cout << std::endl;

    std::cout << "nested2" << std::endl;
    for (const auto& v : *nested2) {
        std::cout << v << " -> " << *v << std::endl;
    }
    std::cout << std::endl;

    auto num1 = make_value(3.14);
    auto num2 = make_value(3.14);
    auto num3 = make_value(6.28);
    std::cout << *num1 << " == " << *num1 << " --> " << (*num1 == *num1) << std::endl;
    std::cout << *num1 << " == " << *num2 << " --> " << (*num1 == *num2) << std::endl;
    std::cout << *num2 << " == " << *num1 << " --> " << (*num2 == *num1) << std::endl;
    std::cout << *num2 << " == " << *num2 << " --> " << (*num2 == *num2) << std::endl;
    std::cout << *num1 << " == " << *num3 << " --> " << (*num1 == *num3) << std::endl;
    std::cout << *num2 << " == " << *num3 << " --> " << (*num2 == *num3) << std::endl;
    std::cout << *num3 << " == " << *num3 << " --> " << (*num3 == *num3) << std::endl;
    std::cout << std::endl;

    auto sym1 = make_value("abc");
    auto sym2 = make_value("abc");
    auto sym3 = make_value("xyz");
    std::cout << *sym1 << " == " << *sym1 << " --> " << (*sym1 == *sym1) << std::endl;
    std::cout << *sym1 << " == " << *sym2 << " --> " << (*sym1 == *sym2) << std::endl;
    std::cout << *sym2 << " == " << *sym1 << " --> " << (*sym2 == *sym1) << std::endl;
    std::cout << *sym2 << " == " << *sym2 << " --> " << (*sym2 == *sym2) << std::endl;
    std::cout << *sym1 << " == " << *sym3 << " --> " << (*sym1 == *sym3) << std::endl;
    std::cout << *sym2 << " == " << *sym3 << " --> " << (*sym2 == *sym3) << std::endl;
    std::cout << *sym3 << " == " << *sym3 << " --> " << (*sym3 == *sym3) << std::endl;
    std::cout << std::endl;

    auto str1 = make_value("abc"s);
    auto str2 = make_value("abc"s);
    auto str3 = make_value("xyz"s);
    std::cout << *str1 << " == " << *str1 << " --> " << (*str1 == *str1) << std::endl;
    std::cout << *str1 << " == " << *str2 << " --> " << (*str1 == *str2) << std::endl;
    std::cout << *str2 << " == " << *str1 << " --> " << (*str2 == *str1) << std::endl;
    std::cout << *str2 << " == " << *str2 << " --> " << (*str2 == *str2) << std::endl;
    std::cout << *str1 << " == " << *str3 << " --> " << (*str1 == *str3) << std::endl;
    std::cout << *str2 << " == " << *str3 << " --> " << (*str2 == *str3) << std::endl;
    std::cout << *str3 << " == " << *str3 << " --> " << (*str3 == *str3) << std::endl;
    std::cout << std::endl;

    auto lst1 = make_value_list(1, 2, 3);
    auto lst2 = make_value_list(1, 2, 3);
    auto lst3 = make_value_list(1, 2, make_value_pair(3, 4));
    std::cout << *lst1 << " == " << *lst1 << " --> " << (*lst1 == *lst1) << std::endl;
    std::cout << *lst1 << " == " << *lst2 << " --> " << (*lst1 == *lst2) << std::endl;
    std::cout << *lst2 << " == " << *lst1 << " --> " << (*lst2 == *lst1) << std::endl;
    std::cout << *lst2 << " == " << *lst2 << " --> " << (*lst2 == *lst2) << std::endl;
    std::cout << *lst1 << " == " << *lst3 << " --> " << (*lst1 == *lst3) << std::endl;
    std::cout << *lst2 << " == " << *lst3 << " --> " << (*lst2 == *lst3) << std::endl;
    std::cout << *lst3 << " == " << *lst3 << " --> " << (*lst3 == *lst3) << std::endl;
    std::cout << std::endl;

    auto pr1 = make_value_pair("abc"s, 1);
    auto pr2 = make_value_pair("abc"s, 1);
    auto pr3 = make_value_pair("abc"s, 2);
    std::cout << *pr1 << " == " << *pr1 << " --> " << (*pr1 == *pr1) << std::endl;
    std::cout << *pr1 << " == " << *pr2 << " --> " << (*pr1 == *pr2) << std::endl;
    std::cout << *pr2 << " == " << *pr1 << " --> " << (*pr2 == *pr1) << std::endl;
    std::cout << *pr2 << " == " << *pr2 << " --> " << (*pr2 == *pr2) << std::endl;
    std::cout << *pr1 << " == " << *pr3 << " --> " << (*pr1 == *pr3) << std::endl;
    std::cout << *pr2 << " == " << *pr3 << " --> " << (*pr2 == *pr3) << std::endl;
    std::cout << *pr3 << " == " << *pr3 << " --> " << (*pr3 == *pr3) << std::endl;
    std::cout << std::endl;
}

int main() {
    run_test_function(test_to_str);
    run_test_function(test_pair);

    std::cout << "all tests have been passed!" << std::endl;

    return 0;
}
