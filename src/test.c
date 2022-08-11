#include "test.h"

#include <stdio.h>

#include "value.h"

static void print_value(value* v) {
    char buffer[1024];
    value_to_str(v, buffer);
    printf("'%s'\n", buffer);
}

void run_test() {
    value* v1 = value_new_number(1);
    value* v2 = value_new_symbol("x");
    value* v3 = value_new_error("err");

    value* v_null = value_new_null();
    value* v_pair = value_new_pair(v1, v2);
    value* v_list = value_new_pair(v1, value_new_pair(v2, value_new_pair(v3, v_null)));
    value* v_compound = value_new_pair(v_list, value_new_pair(v_list, v_null));
    value* v_compound_pair = value_new_pair(v1, v_list);
    value* v_compound_pair2 = value_new_pair(v1, value_new_pair(v_list, v_null));
    value* v_compound_pair3 = value_new_pair(v_list, v1);
    value* v_compound_pair4 = value_new_pair(v_null, v1);
    value* v_compound_pair5 = value_new_pair(v2, v_null);
    value* v_compound_pair6 = value_new_pair(v_null, v_null);

    print_value(v1);
    print_value(v_null);
    print_value(v_pair);
    print_value(v_list);
    print_value(v_compound);
    print_value(v_compound_pair);
    print_value(v_compound_pair2);
    print_value(v_compound_pair3);
    print_value(v_compound_pair4);
    print_value(v_compound_pair5);
    print_value(v_compound_pair6);
}
