#include <iostream>

#include "value.hpp"

using namespace std::literals;

int main() {
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

    return 0;
}
