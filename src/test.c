#include "test.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "parse.h"
#include "pool.h"
#include "value.h"

#define RUN_TEST_FN(fn)                        \
    {                                          \
        printf("[%s]\n", #fn);                 \
        printf("=========================\n"); \
        fn();                                  \
        printf("\n");                          \
    }

static int test_counter = 0;

static void report_test(char* output) {
    printf(
        "\x1B[34m%-5d\x1B[0m %s\n",
        ++test_counter, output);
}

static value* get_parsed(char* input) {
    char output[1024];
    value* v = parse_values(input);
    value_to_str(v, output);

    char formatted[1024];
    sprintf(
        formatted,
        "\x1B[34m[\x1B[0m%s\x1B[34m]\x1B[0m "
        "\x1B[34m-->\x1B[0m "
        "\x1B[34m[\x1B[0m%s\x1B[34m]\x1B[0m",
        input, output);
    report_test(formatted);

    return v;
}

static void test_parse_output(char* input, char* expected) {
    value* p = get_parsed(input);

    char buffer[1024];
    value_to_str(p, buffer);
    assert(strcmp(buffer, expected) == 0);
    value_dispose(p);
}

static void test_parse_error(char* input, char* expected) {
    value* p = get_parsed(input);

    assert(p != NULL);
    assert(p->type == VALUE_ERROR);
    assert(strstr(p->symbol, expected));
    value_dispose(p);
}

void test_parse() {
    test_parse_output("", "()");
    test_parse_output("()", "(())");
    test_parse_output("(() ((()) () ())) ()", "((() ((()) () ())) ())");

    test_parse_output("1", "(1)");
    test_parse_output("(1)", "((1))");
    test_parse_output("(1 () 2)", "((1 () 2))");
    test_parse_output("1 2 3", "(1 2 3)");
    test_parse_output("(1 2 3)", "((1 2 3))");
    test_parse_output("  (  1    2 3 )  ", "((1 2 3))");
    test_parse_output("1 (2 3 (4) 5 ((6 7) 8 9) 10)", "(1 (2 3 (4) 5 ((6 7) 8 9) 10))");
    test_parse_output("  1  (  2 3 (4  ) 5 (( 6  7 )))", "(1 (2 3 (4) 5 ((6 7))))");
    test_parse_output("(() 1 (() 2) (3 ()) 4 (5 (()) 6) 7 ())", "((() 1 (() 2) (3 ()) 4 (5 (()) 6) 7 ()))");

    test_parse_output("1 2.0 3.14 4 -5.67 .123", "(1 2 3.14 4 -5.67 0.123)");
    test_parse_output("123. 1e0 1e2 1e-2 1e-10", "(123 1 100 0.01 1e-10)");

    test_parse_output("x", "(x)");
    test_parse_output("x y z", "(x y z)");
    test_parse_output("x 1 y 2 z", "(x 1 y 2 z)");

    test_parse_output("'1", "((quote 1))");
    test_parse_output("'1 2", "((quote 1) 2)");
    test_parse_output("1 2 ' (3 4 (5 6 7))", "(1 2 (quote (3 4 (5 6 7))))");
    test_parse_output("''(1 2 3)", "((quote (quote (1 2 3))))");

    test_parse_output(".", "(.)");
    test_parse_output("1 . 2 . 3", "(1 . 2 . 3)");
    test_parse_output("(1 . 2)", "((1 . 2))");
    test_parse_output("(.1 . 2.)", "((0.1 . 2))");
    test_parse_output("(1.2 . 3.4)", "((1.2 . 3.4))");
    test_parse_output("(1 2 3 . 4)", "((1 2 3 . 4))");
    test_parse_output("'(1 . 2)", "((quote (1 . 2)))");

    test_parse_output("(1 2 3); comment", "((1 2 3))");
    test_parse_output("(1 2 3)   ;   comment  ", "((1 2 3))");
    test_parse_output("\n(1 2 3)   ;   comment  \n  (4 5)", "((1 2 3) (4 5))");

    test_parse_output("\"abc\"", "(\"abc\")");
    test_parse_output("\"\"", "(\"\")");
    test_parse_output("\"x\" \"y\" \"z\"", "(\"x\" \"y\" \"z\")");
    test_parse_output("\"a\\tb\"", "(\"a\\tb\")");
    test_parse_output("\"a\tb\"", "(\"a\\tb\")");
    test_parse_output("\"a\\nb\"", "(\"a\\nb\")");
    test_parse_output("\"\na\nb\"", "(\"\\na\\nb\")");
    test_parse_output("'\"abc\"", "((quote \"abc\"))");
    test_parse_output("\"'abc\"", "(\"'abc\")");
    test_parse_output("'\"x\" \"y\" \"z\"", "((quote \"x\") \"y\" \"z\")");
    test_parse_output("'(\"x\" \"y\" \"z\")", "((quote (\"x\" \"y\" \"z\")))");

    test_parse_error("(1 2", "missing )");
    test_parse_error("1 2)", "premature )");
    test_parse_error("( 1 (2", "missing )");
    test_parse_error("'", "unfollowed '");
    test_parse_error("(1 .)", "unfollowed .");
    test_parse_error("(1 . 2 3)", ". followed by 2+ items");
    test_parse_error("(1 . .)", ". followed by .");
    test_parse_error("(. 2)", "nothing before .");
    test_parse_error("\"", "unterminated string");
    test_parse_error("\"xyz", "unterminated string");
    test_parse_error(" \" xyz ", "unterminated string");
    test_parse_error("\"xyz\" \"a", "unterminated string");
}

void test_pool() {
    // setup
    value* r1 = value_new_pair(NULL, NULL);
    value* r2 = value_new_pair(NULL, NULL);

    report_test("init");
    pool* p = malloc(sizeof(pool));
    pool_init(p);
    assert(p->size == 0);

    report_test("singleton values");
    value* num = pool_new_number(p, 3.14);
    value* sym = pool_new_symbol(p, "hello");
    value* str = pool_new_string(p, "world");
    value* err = pool_new_error(p, "error %d %s", 123, "x");
    value* inf = pool_new_error(p, "info %d %s", 456, "y");
    assert(p->size == 5);
    assert(num->number == 3.14);
    assert(strcmp(sym->symbol, "hello") == 0);
    assert(strcmp(str->symbol, "world") == 0);
    assert(strcmp(err->symbol, "error 123 x") == 0);
    assert(strcmp(inf->symbol, "info 456 y") == 0);
    pool_collect_garbage(p);
    assert(p->size == 0);

    report_test("compound values");
    pool_new_pair(
        p,
        pool_new_pair(
            p,
            pool_new_number(p, 2.71),
            NULL),
        pool_new_symbol(p, "xyz"));
    assert(p->size == 4);
    pool_collect_garbage(p);
    assert(p->size == 0);

    report_test("memory roots");
    pool_register_root(p, r1);
    r1->car = pool_new_pair(
        p,
        pool_new_number(p, 1),
        pool_new_pair(
            p,
            pool_new_number(p, 2),
            NULL));
    pool_register_root(p, r2);
    r2->car = r1->car->cdr;
    assert(p->size == 4);
    pool_collect_garbage(p);
    assert(p->size == 4);
    pool_unregister_root(p, r2);
    pool_collect_garbage(p);
    assert(p->size == 4);
    r1->car = NULL;
    pool_register_root(p, r2);
    pool_collect_garbage(p);
    assert(p->size == 2);
    pool_unregister_root(p, r1);
    pool_unregister_root(p, r2);
    pool_collect_garbage(p);
    assert(p->size == 0);

    report_test("value cycle");
    value* v1 = pool_new_pair(
        p,
        pool_new_number(p, 1),
        pool_new_pair(
            p,
            pool_new_number(p, 2),
            pool_new_pair(
                p,
                pool_new_number(p, 3),
                NULL)));
    v1->cdr->cdr->cdr = v1;
    pool_register_root(p, r1);
    r1->car = v1->cdr;
    assert(p->size == 6);
    pool_collect_garbage(p);
    assert(p->size == 6);
    pool_unregister_root(p, r1);
    pool_collect_garbage(p);
    assert(p->size == 0);

    report_test("import");
    value* source = value_new_pair(
        value_new_number(123),
        value_new_pair(
            value_new_symbol("abc"),
            NULL));
    value* dest = pool_import(p, source);
    assert(p->size == 4);
    assert(source != dest);
    assert(source->car != dest->car);
    assert(source->cdr != dest->cdr);
    assert(source->cdr->car != dest->cdr->car);
    assert(dest->car->number == 123);
    assert(strcmp(dest->cdr->car->symbol, "abc") == 0);
    pool_collect_garbage(p);
    assert(p->size == 0);
    assert(source->car->number == 123);
    assert(strcmp(source->cdr->car->symbol, "abc") == 0);
    value_dispose(source);

    report_test("cleanup");
    pool_new_number(p, 123);
    pool_new_symbol(p, "hello");
    pool_new_string(p, "world");
    assert(p->size == 3);
    pool_cleanup(p);
    assert(p->size == 0);
    free(p);

    // teardown
    r1->car = NULL;
    r2->car = NULL;
    value_dispose(r1);
    value_dispose(r2);
}

void run_test() {
    RUN_TEST_FN(test_parse);
    RUN_TEST_FN(test_pool);

    printf("all tests have passed!\n");
}
