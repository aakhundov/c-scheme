#include "test.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "eval.h"
#include "machine.h"
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

#define RUN_EVAL_TEST_FN(e, fn)                \
    {                                          \
        eval_reset_env(e);                     \
        printf("[%s]\n", #fn);                 \
        printf("=========================\n"); \
        fn(e);                                 \
        printf("\n");                          \
    }

#define PRINT_VALUE(name, v)               \
    {                                      \
        char buffer[16384];                \
        value_to_str(v, buffer);           \
        printf("%s = %s\n", name, buffer); \
    }

static int test_counter = 0;

static void report_test(char* output, ...) {
    static char buffer[16384];

    va_list args;
    va_start(args, output);
    vsnprintf(buffer, sizeof(buffer), output, args);
    va_end(args);

    printf(
        "\x1B[34m%04d\x1B[0m %s\n",
        ++test_counter, buffer);
}

static value* get_evaluated(eval* ev, char* input) {
    value* v = parse_from_str(input);

    if (ev != NULL && v != NULL && v->type != VALUE_ERROR) {
        assert(v->cdr == NULL);
        value* e = eval_evaluate(ev, v->car);
        value_dispose(v);
        v = e;
    }

    static char output[16384];
    value_to_str(v, output);

    static char formatted[16384];
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
    value* p = get_evaluated(NULL, input);

    static char buffer[16384];
    value_to_str(p, buffer);
    assert(strcmp(buffer, expected) == 0);
    value_dispose(p);
}

static void test_parse_error(char* input, char* expected) {
    value* p = get_evaluated(NULL, input);

    assert(p != NULL);
    assert(p->type == VALUE_ERROR);
    assert(strstr(p->symbol, expected));
    value_dispose(p);
}

static void test_eval_output(eval* ev, char* input, char* expected) {
    value* e = get_evaluated(ev, input);

    static char buffer[16384];
    value_to_str(e, buffer);
    assert(strcmp(buffer, expected) == 0);
    value_dispose(e);
}

static void test_eval_error(eval* ev, char* input, char* expected) {
    value* e = get_evaluated(ev, input);

    assert(e != NULL);
    assert(e->type == VALUE_ERROR);
    assert(strstr(e->symbol, expected));
    value_dispose(e);
}

/*
static void test_eval_number(eval* ev, char* input, double expected) {
    value* e = get_evaluated(ev, input);

    assert(e != NULL);
    assert(e->type == VALUE_NUMBER);
    assert(e->number == expected);
    value_dispose(e);
}

static void test_eval_info(eval* ev, char* input, char* expected) {
    value* e = get_evaluated(ev, input);

    assert(e != NULL);
    assert(e->type == VALUE_INFO);
    assert(strstr(e->symbol, expected));
    value_dispose(e);
}
*/

static void test_parse() {
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

    test_parse_output("nil", "(())");
    test_parse_output("true", "(true)");
    test_parse_output("false", "(false)");

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

void test_cycle_to_str() {
    value* v = NULL;
    static char buffer[16384];

    v = value_new_pair(
        value_new_number(1),
        value_new_pair(
            value_new_number(2),
            value_new_pair(
                NULL,
                NULL)));
    v->cdr->cdr->car = v;
    value_to_str(v, buffer);
    report_test("car cycle: %s", buffer);
    assert(strcmp(buffer, "(1 2 <...>)") == 0);
    value_dispose(v);

    v = value_new_pair(
        value_new_number(1),
        value_new_pair(
            value_new_number(2),
            value_new_pair(
                value_new_number(3),
                NULL)));
    v->cdr->cdr->cdr = v;
    value_to_str(v, buffer);
    report_test("cdr cycle: %s", buffer);
    assert(strcmp(buffer, "(1 2 3)") == 0);
    value_dispose(v);

    v = value_new_pair(
        value_new_number(1),
        value_new_pair(
            NULL,
            value_new_pair(
                value_new_number(3),
                NULL)));
    v->cdr->car = v;
    v->cdr->cdr->cdr = v;
    value_to_str(v, buffer);
    report_test("car and cdr cycle: %s", buffer);
    assert(strcmp(buffer, "(1 <...> 3)") == 0);
    value_dispose(v);

    v = value_new_pair(NULL, NULL);
    v->car = v;
    v->cdr = v;
    value_to_str(v, buffer);
    report_test("self cycle: %s", buffer);
    assert(strcmp(buffer, "(<...>)") == 0);
    value_dispose(v);
}

static void test_pool() {
    // setup
    value* r1 = value_new_pair(NULL, NULL);
    value* r2 = value_new_pair(NULL, NULL);
    value* source = NULL;
    value* dest = NULL;

    report_test("init");
    pool* p;
    pool_new(&p);
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
    source = pool_new_pair(
        p,
        pool_new_number(p, 1),
        pool_new_pair(
            p,
            pool_new_number(p, 2),
            pool_new_pair(
                p,
                pool_new_number(p, 3),
                NULL)));
    source->cdr->cdr->cdr = source;
    pool_register_root(p, r1);
    r1->car = source->cdr;
    assert(p->size == 6);
    pool_collect_garbage(p);
    assert(p->size == 6);
    pool_unregister_root(p, r1);
    pool_collect_garbage(p);
    assert(p->size == 0);

    report_test("import");
    source = value_new_pair(
        value_new_number(123),
        value_new_pair(
            value_new_symbol("abc"),
            NULL));
    dest = pool_import(p, source);
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

    report_test("import cycle");
    source = value_new_pair(
        value_new_number(123),
        value_new_pair(
            value_new_symbol("abc"),
            NULL));
    source->cdr->cdr = source;
    dest = pool_import(p, source);
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

    report_test("export");
    source = pool_new_pair(
        p,
        pool_new_number(p, 123),
        pool_new_pair(
            p,
            pool_new_symbol(p, "abc"),
            NULL));
    dest = pool_export(p, source);
    assert(p->size == 4);
    assert(source != dest);
    assert(source->car != dest->car);
    assert(source->cdr != dest->cdr);
    assert(source->cdr->car != dest->cdr->car);
    assert(source->car->number == 123);
    assert(strcmp(source->cdr->car->symbol, "abc") == 0);
    pool_collect_garbage(p);
    assert(p->size == 0);
    assert(dest->car->number == 123);
    assert(strcmp(dest->cdr->car->symbol, "abc") == 0);
    value_dispose(dest);

    report_test("export cycle");
    source = pool_new_pair(
        p,
        pool_new_number(p, 123),
        pool_new_pair(
            p,
            pool_new_symbol(p, "abc"),
            NULL));
    source->cdr->cdr = source;
    dest = pool_export(p, source);
    assert(p->size == 4);
    assert(source != dest);
    assert(source->car != dest->car);
    assert(source->cdr != dest->cdr);
    assert(source->cdr->car != dest->cdr->car);
    assert(source->car->number == 123);
    assert(strcmp(source->cdr->car->symbol, "abc") == 0);
    pool_collect_garbage(p);
    assert(p->size == 0);
    assert(dest->car->number == 123);
    assert(strcmp(dest->cdr->car->symbol, "abc") == 0);
    value_dispose(dest);

    report_test("cleanup");
    pool_new_number(p, 123);
    pool_new_symbol(p, "hello");
    pool_new_string(p, "world");
    assert(p->size == 3);
    pool_dispose(&p);

    // teardown
    r1->car = NULL;
    r2->car = NULL;
    value_dispose(r1);
    value_dispose(r2);
}

static value* op_rem(machine* m, value* args) {
    value* x = args->car->car;
    value* y = args->cdr->car->car;
    return pool_new_number(m->pool, (int)x->number % (int)y->number);
}

static value* op_eq(machine* m, value* args) {
    value* x = args->car->car;
    value* y = args->cdr->car->car;
    return pool_new_number(m->pool, x->number == y->number);
}

static value* op_lt(machine* m, value* args) {
    value* x = args->car->car;
    value* y = args->cdr->car->car;
    return pool_new_number(m->pool, x->number < y->number);
}

static value* op_plus(machine* m, value* args) {
    value* x = args->car->car;
    value* y = args->cdr->car->car;
    return pool_new_number(m->pool, x->number + y->number);
}

static value* op_minus(machine* m, value* args) {
    value* x = args->car->car;
    value* y = args->cdr->car->car;
    return pool_new_number(m->pool, x->number - y->number);
}

static value* op_mult(machine* m, value* args) {
    value* x = args->car->car;
    value* y = args->cdr->car->car;
    return pool_new_number(m->pool, x->number * y->number);
}

static void test_gcd_machine() {
    value* code = parse_from_file("./lib/machines/gcd.scm");
    assert(code->type != VALUE_ERROR);

    int test_data[][3] = {
        {24, 36, 12},
        {9, 16, 1},
        {10, 10, 10},
        {72, 54, 18},
        {5, 125, 5},
    };

    machine* m;
    machine_new(&m, code, "a");
    machine_bind_op(m, "rem", op_rem);
    machine_bind_op(m, "=", op_eq);

    for (size_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++) {
        int a = test_data[i][0];
        int b = test_data[i][1];
        int val = test_data[i][2];

        machine_copy_to_register(m, "a", pool_new_number(m->pool, a));
        machine_copy_to_register(m, "b", pool_new_number(m->pool, b));
        machine_run(m);

        value* result = machine_export_output(m);

        char buffer[1024];
        value_to_str(result, buffer);
        report_test("gcd(%d, %d) --> %s", a, b, buffer);
        assert(result->number == val);
        value_dispose(result);
    }

    value_dispose(code);
    machine_dispose(&m);
}

static void test_fact_machine() {
    value* code = parse_from_file("./lib/machines/factorial.scm");
    assert(code->type != VALUE_ERROR);

    int test_data[][2] = {
        {1, 1},
        {2, 2},
        {3, 6},
        {5, 120},
        {7, 5040},
        {10, 3628800},
    };

    machine* m;
    machine_new(&m, code, "val");
    machine_bind_op(m, "-", op_minus);
    machine_bind_op(m, "*", op_mult);
    machine_bind_op(m, "=", op_eq);

    for (size_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++) {
        int n = test_data[i][0];
        int val = test_data[i][1];

        machine_copy_to_register(m, "n", pool_new_number(m->pool, n));
        machine_run(m);

        value* result = machine_export_output(m);

        char buffer[1024];
        value_to_str(result, buffer);
        report_test("factorial(%d) --> %s", n, buffer);
        assert(result->number == val);
        value_dispose(result);
    }

    value_dispose(code);
    machine_dispose(&m);
}

static void test_fib_machine() {
    value* code = parse_from_file("./lib/machines/fibonacci.scm");
    assert(code->type != VALUE_ERROR);

    int test_data[][2] = {
        {0, 0},
        {1, 1},
        {2, 1},
        {5, 5},
        {8, 21},
        {10, 55},
    };

    machine* m;
    machine_new(&m, code, "val");
    machine_bind_op(m, "<", op_lt);
    machine_bind_op(m, "+", op_plus);
    machine_bind_op(m, "-", op_minus);

    for (size_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++) {
        int n = test_data[i][0];
        int val = test_data[i][1];

        machine_copy_to_register(m, "n", pool_new_number(m->pool, n));
        machine_run(m);

        value* result = machine_export_output(m);

        char buffer[1024];
        value_to_str(result, buffer);
        report_test("fibonacci(%d) --> %s", n, buffer);
        assert(result->number == val);
        value_dispose(result);
    }

    value_dispose(code);
    machine_dispose(&m);
}

static void test_machine() {
    test_gcd_machine();
    test_fact_machine();
    test_fib_machine();
}

static void test_structural(eval* e) {
    test_eval_output(e, "(car '(1))", "1");
    test_eval_output(e, "(car '(1 2 3))", "1");
    test_eval_output(e, "(car '((1 2) 3))", "(1 2)");
    test_eval_output(e, "(car '(() 3))", "()");

    test_eval_output(e, "(cdr '(1))", "()");
    test_eval_output(e, "(cdr '(1 2 3))", "(2 3)");
    test_eval_output(e, "(cdr '((1 2) 3))", "(3)");
    test_eval_output(e, "(cdr '((1 2)))", "()");
    test_eval_output(e, "(cdr '((1 2) ()))", "(())");

    test_eval_output(e, "(cons 1 2)", "(1 . 2)");
    test_eval_output(e, "(cons 1 '(2))", "(1 2)");
    test_eval_output(e, "(cons '(1) 2)", "((1) . 2)");
    test_eval_output(e, "(cons '(1) '(2))", "((1) 2)");
    test_eval_output(e, "(cons '() '())", "(())");
    test_eval_output(e, "(car (cons 1 2))", "1");
    test_eval_output(e, "(cdr (cons 1 2))", "2");

    test_eval_error(e, "(car 1)", "the arg is not a pair");
    test_eval_error(e, "(car \"abc\")", "the arg is not a pair");
    test_eval_error(e, "(car '())", "the arg is not a pair");
    test_eval_error(e, "(car)", "exactly 1 arg expected");
    test_eval_error(e, "(car 1 2)", "exactly 1 arg expected");
    test_eval_error(e, "(car '(1 2) '(3 4))", "exactly 1 arg expected");

    test_eval_error(e, "(cdr 1)", "the arg is not a pair");
    test_eval_error(e, "(cdr \"abc\")", "the arg is not a pair");
    test_eval_error(e, "(cdr '())", "the arg is not a pair");
    test_eval_error(e, "(cdr)", "exactly 1 arg expected");
    test_eval_error(e, "(cdr 1 2)", "exactly 1 arg expected");
    test_eval_error(e, "(cdr '(1 2) '(3 4))", "exactly 1 arg expected");

    test_eval_error(e, "(cons)", "exactly 2 args expected");
    test_eval_error(e, "(cons 1)", "exactly 2 args expected");
    test_eval_error(e, "(cons 1 2 3)", "exactly 2 args expected");
}

void run_test() {
    eval* e;
    eval_new(&e, "./lib/machines/evaluator.scm");

    RUN_TEST_FN(test_parse);
    RUN_TEST_FN(test_cycle_to_str);
    RUN_TEST_FN(test_pool);
    RUN_TEST_FN(test_machine);

    RUN_EVAL_TEST_FN(e, test_structural);

    printf("all tests have passed!\n");

    eval_dispose(&e);
}
