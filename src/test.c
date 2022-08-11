#include "test.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "parse.h"
#include "value.h"

#define RUN_TEST_FN(fn)                        \
    {                                          \
        printf("[%s]\n", #fn);                 \
        printf("=========================\n"); \
        fn();                                  \
        printf("\n");                          \
    }

static int counter = 0;

static value* get_parsed(char* input) {
    char output[1024];

    value* v = parse_values(input);

    value_to_str(v, output);
    printf(
        "\x1B[34m%-5d\x1B[0m "
        "\x1B[34m[\x1B[0m%s\x1B[34m]\x1B[0m "
        "\x1B[34m-->\x1B[0m "
        "\x1B[34m[\x1B[0m%s\x1B[34m]\x1B[0m\n",
        ++counter, input, output);

    return v;
}

static void test_parse_output(char* input, char* expected) {
    value* p = get_parsed(input);

    if (p != NULL) {
        char buffer[1024];
        value_to_str(p, buffer);
        assert(strcmp(buffer, expected) == 0);
        value_dispose(p);
    }
}

static void test_parse_error(char* input, char* expected) {
    value* p = get_parsed(input);

    if (p != NULL) {
        assert(p->type == VALUE_ERROR);
        assert(strstr(p->symbol, expected));
        value_dispose(p);
    }
}

void test_parse() {
    test_parse_output("1", "(1)");
    test_parse_output("(1)", "((1))");
    test_parse_output("1 2 3", "(1 2 3)");
    test_parse_output("(1 2 3)", "((1 2 3))");
    test_parse_output("  (  1    2 3 )  ", "((1 2 3))");
    test_parse_output("1 (2 3 (4) 5 ((6 7) 8 9) 10)", "(1 (2 3 (4) 5 ((6 7) 8 9) 10))");
    test_parse_output("  1  (  2 3 (4  ) 5 (( 6  7 )))", "(1 (2 3 (4) 5 ((6 7))))");

    test_parse_output("1 2.0 3.14 4 -5.67 .123", "(1 2 3.14 4 -5.67 0.123)");
    test_parse_output("123. 1e0 1e2 1e-2 1e-10", "(123 1 100 0.01 1e-10)");

    test_parse_output("x", "(x)");
    test_parse_output("x y z", "(x y z)");
    test_parse_output("x 1 y 2 z", "(x 1 y 2 z)");

    test_parse_output("'1", "((quote 1))");
    test_parse_output("'1 2", "((quote 1) 2)");
    test_parse_output("1 2 ' (3 4 (5 6 7))", "(1 2 (quote (3 4 (5 6 7))))");
    test_parse_output("''(1 2 3)", "((quote (quote (1 2 3))))");

    test_parse_output("(1 . 2)", "((1 . 2))");
    test_parse_output("(.1 . 2.)", "((0.1 . 2))");
    test_parse_output("(1.2 . 3.4)", "((1.2 . 3.4))");
    test_parse_output("(1 2 3 . 4)", "((1 2 3 . 4))");
    test_parse_output("'(1 . 2)", "((quote (1 . 2)))");

    test_parse_output("(1 2 3); comment", "((1 2 3))");
    test_parse_output("(1 2 3)   ;   comment  ", "((1 2 3))");
    test_parse_output("(1 2 3)   ;   comment  \n  (4 5)", "((1 2 3) (4 5))");

    test_parse_output("", "()");

    test_parse_error("(1 2", "missing )");
    test_parse_error("1 2)", "premature )");
    test_parse_error("( 1 (2", "missing )");
    test_parse_error("'", "unfollowed '");
    test_parse_error("(1 .)", "unfollowed .");
    test_parse_error("(1 . 2 3)", ". followed by 2+ items");
    test_parse_error("(1 . .)", ". followed by .");
    test_parse_error("(. 2)", "nothing before .");
}

void run_test() {
    RUN_TEST_FN(test_parse);
}
