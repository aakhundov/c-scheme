#include "test.h"

#include <assert.h>
#include <math.h>
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
        "\x1B[34m%05d\x1B[0m %s\n",
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
        "\x1B[34m[\x1B[0m%s\x1B[34m] --> [\x1B[0m%s\x1B[34m]\x1B[0m",
        input, output);
    report_test("%s", formatted);

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

static void test_eval_number(eval* ev, char* input, double expected) {
    value* e = get_evaluated(ev, input);

    assert(e != NULL);
    assert(e->type == VALUE_NUMBER);
    if (!isnan(expected)) {
        assert(fabs(e->number - expected) < 1e-4);
    } else {
        assert(isnan(e->number));
    }
    value_dispose(e);
}

static void test_eval_string(eval* ev, char* input, char* expected) {
    value* e = get_evaluated(ev, input);

    assert(e != NULL);
    assert(e->type == VALUE_STRING);
    assert(strcmp(e->symbol, expected) == 0);
    value_dispose(e);
}

static void test_eval_bool(eval* ev, char* input, int expected) {
    value* e = get_evaluated(ev, input);

    assert(e != NULL);
    assert(e->type == VALUE_BOOL);
    assert(e->number == expected);
    value_dispose(e);
}

static void test_eval_error(eval* ev, char* input, char* expected) {
    value* e = get_evaluated(ev, input);

    assert(e != NULL);
    assert(e->type == VALUE_ERROR);
    assert(strstr(e->symbol, expected));
    value_dispose(e);
}

static void test_eval_info(eval* ev, char* input, char* expected) {
    value* e = get_evaluated(ev, input);

    assert(e != NULL);
    assert(e->type == VALUE_INFO);
    assert(strstr(e->symbol, expected));
    value_dispose(e);
}

static void test_parse() {
    // brackets
    test_parse_output("", "()");
    test_parse_output("()", "(())");
    test_parse_output("(() ((()) () ())) ()", "((() ((()) () ())) ())");

    // integers
    test_parse_output("1", "(1)");
    test_parse_output("(1)", "((1))");
    test_parse_output("(1 () 2)", "((1 () 2))");
    test_parse_output("1 2 3", "(1 2 3)");
    test_parse_output("(1 2 3)", "((1 2 3))");
    test_parse_output("  (  1    2 3 )  ", "((1 2 3))");
    test_parse_output("1 (2 3 (4) 5 ((6 7) 8 9) 10)", "(1 (2 3 (4) 5 ((6 7) 8 9) 10))");
    test_parse_output("  1  (  2 3 (4  ) 5 (( 6  7 )))", "(1 (2 3 (4) 5 ((6 7))))");
    test_parse_output("(() 1 (() 2) (3 ()) 4 (5 (()) 6) 7 ())", "((() 1 (() 2) (3 ()) 4 (5 (()) 6) 7 ()))");

    // decimals
    test_parse_output("1 2.0 3.14 4 -5.67 .123", "(1 2 3.14 4 -5.67 0.123)");
    test_parse_output("123. 1e0 1e2 1e-2 1e-10", "(123 1 100 0.01 1e-10)");

    // symbols
    test_parse_output("x", "(x)");
    test_parse_output("x y z", "(x y z)");
    test_parse_output("x 1 y 2 z", "(x 1 y 2 z)");

    // quote
    test_parse_output("'1", "((quote 1))");
    test_parse_output("'1 2", "((quote 1) 2)");
    test_parse_output("1 2 ' (3 4 (5 6 7))", "(1 2 (quote (3 4 (5 6 7))))");
    test_parse_output("''(1 2 3)", "((quote (quote (1 2 3))))");

    // dot
    test_parse_output(".", "(.)");
    test_parse_output("1 . 2 . 3", "(1 . 2 . 3)");
    test_parse_output("(1 . 2)", "((1 . 2))");
    test_parse_output("(.1 . 2.)", "((0.1 . 2))");
    test_parse_output("(1.2 . 3.4)", "((1.2 . 3.4))");
    test_parse_output("(1 2 3 . 4)", "((1 2 3 . 4))");
    test_parse_output("'(1 . 2)", "((quote (1 . 2)))");

    // comment
    test_parse_output("(1 2 3); comment", "((1 2 3))");
    test_parse_output("(1 2 3)   ;   comment  ", "((1 2 3))");
    test_parse_output("\n(1 2 3)   ;   comment  \n  (4 5)", "((1 2 3) (4 5))");

    // string
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

    // special symbols
    test_parse_output("nil", "(())");
    test_parse_output("true", "(true)");
    test_parse_output("false", "(false)");

    // parsing errors
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

/*
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
*/

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

static void test_syntax(eval* e) {
    // self-evaluating
    test_eval_number(e, "1", 1);
    test_eval_number(e, "-1", -1);
    test_eval_number(e, "3.14", 3.14);
    test_eval_string(e, "\"\"", "");
    test_eval_string(e, "\"abc\"", "abc");
    test_eval_bool(e, "true", 1);
    test_eval_bool(e, "false", 0);
    test_eval_output(e, "nil", "()");
    test_eval_output(e, "()", "()");

    // variable lookup
    test_eval_error(e, "a", "a is unbound");
    test_eval_info(e, "(define a 10)", "a is defined");
    test_eval_number(e, "a", 10);

    // quote
    test_eval_number(e, "(quote 1)", 1);
    test_eval_output(e, "(quote x)", "x");
    test_eval_output(e, "(quote (1 2 3))", "(1 2 3)");
    test_eval_output(e, "(quote (quote 1 2 3))", "(quote 1 2 3)");
    test_eval_output(e, "(quote ())", "()");

    // quote error
    test_eval_error(e, "(quote)", "quote: no item");
    test_eval_error(e, "(quote 1 2)", "quote: more than one item");

    // set!
    test_eval_error(e, "b", "b is unbound");
    test_eval_error(e, "(set! b 30)", "b is unbound");
    test_eval_info(e, "(define b 20)", "b is defined");
    test_eval_number(e, "b", 20);
    test_eval_info(e, "(set! b 30)", "b is updated");
    test_eval_number(e, "b", 30);

    // set! errors
    test_eval_error(e, "(set!)", "set!: no variable");
    test_eval_error(e, "(set! y)", "set!: no value");
    test_eval_error(e, "(set! 1 30)", "set!: variable is not a symbol");
    test_eval_error(e, "(set! y 30 40)", "set!: more than two items");

    // define
    test_eval_error(e, "c", "c is unbound");
    test_eval_info(e, "(define c 30)", "c is defined");
    test_eval_number(e, "c", 30);
    test_eval_info(e, "(define c 40)", "c is updated");
    test_eval_number(e, "c", 40);
    test_eval_error(e, "f", "f is unbound");
    test_eval_info(e, "(define (f x y) x y)", "f is defined");
    test_eval_output(e, "f", "(lambda (x y) x y)");
    test_eval_info(e, "(define (g) 1)", "g is defined");
    test_eval_output(e, "g", "(lambda () 1)");

    // define errors
    test_eval_error(e, "(define)", "no variable");
    test_eval_error(e, "(define ())", "can't define ()");
    test_eval_error(e, "(define (f))", "no body");
    test_eval_error(e, "(define (1) 1)", "function name is not a symbol");
    test_eval_error(e, "(define (1 x y) 1)", "function name is not a symbol");
    test_eval_error(e, "(define x 1 2)", "can't be more than one item");
    test_eval_error(e, "(define 1 2)", "either variable or function");
    test_eval_error(e, "(define true 2)", "either variable or function");
    test_eval_error(e, "(define \"x\" 2)", "either variable or function");

    // if
    test_eval_number(e, "(if 1 2 3)", 2);
    test_eval_number(e, "(if true 2 3)", 2);
    test_eval_number(e, "(if 0 2 3)", 3);
    test_eval_number(e, "(if false 2 3)", 3);
    test_eval_number(e, "(if 1 2)", 2);
    test_eval_number(e, "(if true 2)", 2);
    test_eval_bool(e, "(if 0 2)", 0);
    test_eval_bool(e, "(if false 2)", 0);

    // if errors
    test_eval_error(e, "(if)", "no predicate");
    test_eval_error(e, "(if 1)", "no consequent");
    test_eval_error(e, "(if 1 2 3 4)", "too many items");

    // lambda
    test_eval_output(e, "(lambda x x)", "(lambda x x)");
    test_eval_output(e, "(lambda () 1)", "(lambda () 1)");
    test_eval_output(e, "(lambda (x) x)", "(lambda (x) x)");
    test_eval_output(e, "(lambda (x) x x)", "(lambda (x) x x)");
    test_eval_output(e, "(lambda (x y) x)", "(lambda (x y) x)");
    test_eval_output(e, "(lambda (x y) x y)", "(lambda (x y) x y)");
    test_eval_output(e, "(lambda (x . y) x y)", "(lambda (x . y) x y)");

    // lambda errors
    test_eval_error(e, "(lambda)", "no parameters");
    test_eval_error(e, "(lambda x)", "no body");
    test_eval_error(e, "(lambda (x))", "no body");
    test_eval_error(e, "(lambda ())", "no body");
    test_eval_error(e, "(lambda 1 1)", "some parameters are not symbols");
    test_eval_error(e, "(lambda (1) 1)", "some parameters are not symbols");
    test_eval_error(e, "(lambda (x 1) 1)", "some parameters are not symbols");
    test_eval_error(e, "(lambda (x x) 1)", "duplicate parameter names");
    test_eval_error(e, "(lambda (x y z x) 1)", "duplicate parameter names");
    test_eval_error(e, "(lambda (x . x) 1)", "duplicate parameter names");
    test_eval_error(e, "(lambda (x y z . x) 1)", "duplicate parameter names");

    // begin
    test_eval_number(e, "(begin 1)", 1);
    test_eval_number(e, "(begin 1 2 3)", 3);
    test_eval_bool(e, "(begin true false)", 0);
    test_eval_output(e, "(begin '(1 2 3) '(4 5) '())", "()");

    // begin errors
    test_eval_error(e, "(begin)", "no items");

    // function without params
    test_eval_info(e, "(define (f0) 1)", "f0 is defined");
    test_eval_output(e, "(f0)", "1");
    test_eval_error(e, "(f0 1)", "arguments (1) don't match the parameters ()");

    // function with 1 param returning a constant
    test_eval_info(e, "(define (f1 x) 1)", "f1 is defined");
    test_eval_output(e, "(f1 1)", "1");
    test_eval_output(e, "(f1 2)", "1");
    test_eval_output(e, "(f1 \"abc\")", "1");
    test_eval_output(e, "(f1 '())", "1");
    test_eval_error(e, "(f1)", "arguments () don't match the parameters (x)");
    test_eval_error(e, "(f1 1 2)", "arguments (1 2) don't match the parameters (x)");

    // function with 1 param returning the 1st param
    test_eval_info(e, "(define (f2 x) x)", "f2 is defined");
    test_eval_output(e, "(f2 1)", "1");
    test_eval_output(e, "(f2 2)", "2");
    test_eval_output(e, "(f2 \"abc\")", "\"abc\"");
    test_eval_output(e, "(f2 '())", "()");
    test_eval_error(e, "(f2)", "arguments () don't match the parameters (x)");
    test_eval_error(e, "(f2 1 2)", "arguments (1 2) don't match the parameters (x)");

    // function with 2 param returning the 1st param
    test_eval_info(e, "(define (f3 x y) x)", "f3 is defined");
    test_eval_output(e, "(f3 1 1)", "1");
    test_eval_output(e, "(f3 1 2)", "1");
    test_eval_output(e, "(f3 2 1)", "2");
    test_eval_output(e, "(f3 2 2)", "2");
    test_eval_error(e, "(f3)", "arguments () don't match the parameters (x y)");
    test_eval_error(e, "(f3 1)", "arguments (1) don't match the parameters (x y)");
    test_eval_error(e, "(f3 1 2 3)", "arguments (1 2 3) don't match the parameters (x y)");

    // function with 2 param returning the 2nd param
    test_eval_info(e, "(define (f4 x y) y)", "f4 is defined");
    test_eval_output(e, "(f4 1 1)", "1");
    test_eval_output(e, "(f4 1 2)", "2");
    test_eval_output(e, "(f4 2 1)", "1");
    test_eval_output(e, "(f4 2 2)", "2");
    test_eval_error(e, "(f4)", "arguments () don't match the parameters (x y)");
    test_eval_error(e, "(f4 1)", "arguments (1) don't match the parameters (x y)");
    test_eval_error(e, "(f4 1 2 3)", "arguments (1 2 3) don't match the parameters (x y)");

    // function with 2 param returning the sum of 1st and 2nd params
    test_eval_info(e, "(define (f5 x y) (+ x y))", "f5 is defined");
    test_eval_output(e, "(f5 1 1)", "2");
    test_eval_output(e, "(f5 1 2)", "3");
    test_eval_output(e, "(f5 2 1)", "3");
    test_eval_output(e, "(f5 2 2)", "4");
    test_eval_error(e, "(f5)", "arguments () don't match the parameters (x y)");
    test_eval_error(e, "(f5 1)", "arguments (1) don't match the parameters (x y)");
    test_eval_error(e, "(f5 1 2 3)", "arguments (1 2 3) don't match the parameters (x y)");

    // function with 1 param + the rest returning the 1st param
    test_eval_info(e, "(define (f6 x . y) x)", "f6 is defined");
    test_eval_output(e, "(f6 1)", "1");
    test_eval_output(e, "(f6 1 2)", "1");
    test_eval_output(e, "(f6 1 2 3)", "1");
    test_eval_error(e, "(f6)", "arguments () don't match the parameters (x . y)");

    // function with 1 param + the rest returning the rest
    test_eval_info(e, "(define (f7 x . y) y)", "f7 is defined");
    test_eval_output(e, "(f7 1)", "()");
    test_eval_output(e, "(f7 1 2)", "(2)");
    test_eval_output(e, "(f7 1 2 3)", "(2 3)");
    test_eval_error(e, "(f7)", "arguments () don't match the parameters (x . y)");

    // function with the rest returning the rest
    test_eval_info(e, "(define (f8 . x) x)", "f8 is defined");
    test_eval_output(e, "(f8)", "()");
    test_eval_output(e, "(f8 1)", "(1)");
    test_eval_output(e, "(f8 1 2)", "(1 2)");
    test_eval_output(e, "(f8 1 2 3)", "(1 2 3)");

    // application errors
    test_eval_error(e, "(1)", "can't apply 1");
    test_eval_error(e, "(1 2 3)", "can't apply 1");
    test_eval_error(e, "(\"x\" 2 3)", "can't apply \"x\"");
    test_eval_error(e, "(true 2 3)", "can't apply true");
    test_eval_error(e, "('() 2 3)", "can't apply ()");
}

static void test_structural(eval* e) {
    // car
    test_eval_output(e, "(car '(1))", "1");
    test_eval_output(e, "(car '(1 2 3))", "1");
    test_eval_output(e, "(car '((1 2) 3))", "(1 2)");
    test_eval_output(e, "(car '(() 3))", "()");

    // car errors
    test_eval_error(e, "(car 1)", "must be pair, but is number");
    test_eval_error(e, "(car \"abc\")", "must be pair, but is string");
    test_eval_error(e, "(car '())", "must be pair, but got ()");
    test_eval_error(e, "(car)", "expects 1 arg, but got 0");
    test_eval_error(e, "(car 1 2)", "expects 1 arg, but got 2");
    test_eval_error(e, "(car '(1 2) '(3 4))", "expects 1 arg, but got 2");

    // cdr
    test_eval_output(e, "(cdr '(1))", "()");
    test_eval_output(e, "(cdr '(1 2 3))", "(2 3)");
    test_eval_output(e, "(cdr '((1 2) 3))", "(3)");
    test_eval_output(e, "(cdr '((1 2)))", "()");
    test_eval_output(e, "(cdr '((1 2) ()))", "(())");

    // cdr errors
    test_eval_error(e, "(cdr 1)", "must be pair, but is number");
    test_eval_error(e, "(cdr \"abc\")", "must be pair, but is string");
    test_eval_error(e, "(cdr '())", "must be pair, but got ()");
    test_eval_error(e, "(cdr)", "expects 1 arg, but got 0");
    test_eval_error(e, "(cdr 1 2)", "expects 1 arg, but got 2");
    test_eval_error(e, "(cdr '(1 2) '(3 4))", "expects 1 arg, but got 2");

    // cons
    test_eval_output(e, "(cons 1 2)", "(1 . 2)");
    test_eval_output(e, "(cons 1 '(2))", "(1 2)");
    test_eval_output(e, "(cons '(1) 2)", "((1) . 2)");
    test_eval_output(e, "(cons '(1) '(2))", "((1) 2)");
    test_eval_output(e, "(cons '() '())", "(())");
    test_eval_output(e, "(car (cons 1 2))", "1");
    test_eval_output(e, "(cdr (cons 1 2))", "2");

    // cons errors
    test_eval_error(e, "(cons)", "expects 2 args, but got 0");
    test_eval_error(e, "(cons 1)", "expects 2 args, but got 1");
    test_eval_error(e, "(cons 1 2 3)", "expects 2 args, but got 3");
}

static void test_arithmetic(eval* e) {
    // +
    test_eval_number(e, "(+ 1)", 1);
    test_eval_number(e, "(+ -1)", -1);
    test_eval_number(e, "(+ 0)", 0);
    test_eval_number(e, "(+ 3.14)", 3.14);
    test_eval_number(e, "(+ 1 2)", 3);
    test_eval_number(e, "(+ 1 2 3 4 5)", 15);
    test_eval_number(e, "(+ 1 -1)", 0);
    test_eval_number(e, "(+ 3.14 2.71)", 5.85);

    // + errors
    test_eval_error(e, "(+)", "expects at least 1 arg, but got 0");
    test_eval_error(e, "(+ \"a\")", "must be number, but is string");
    test_eval_error(e, "(+ 1 2 \"a\")", "must be number, but is string");
    test_eval_error(e, "(+ '())", "must be number, but got ()");

    // -
    test_eval_number(e, "(- 1)", -1);
    test_eval_number(e, "(- -1)", 1);
    test_eval_number(e, "(- 0)", 0);
    test_eval_number(e, "(- 3.14)", -3.14);
    test_eval_number(e, "(- 1 2)", -1);
    test_eval_number(e, "(- 1 2 3 4 5)", -13);
    test_eval_number(e, "(- 1 -1)", 2);
    test_eval_number(e, "(- 3.14 2.71)", 0.43);

    // - errors
    test_eval_error(e, "(-)", "expects at least 1 arg, but got 0");
    test_eval_error(e, "(- \"a\")", "must be number, but is string");
    test_eval_error(e, "(- 1 2 \"a\")", "must be number, but is string");
    test_eval_error(e, "(- '())", "must be number, but got ()");

    // *
    test_eval_number(e, "(* 1 1)", 1);
    test_eval_number(e, "(* 1 -1)", -1);
    test_eval_number(e, "(* -1 2)", -2);
    test_eval_number(e, "(* 1 2 3 4 5)", 120);
    test_eval_number(e, "(* 3.14 2.71)", 8.5094);
    test_eval_number(e, "(* 3.14 -2.71)", -8.5094);

    // * errors
    test_eval_error(e, "(*)", "expects at least 2 args, but got 0");
    test_eval_error(e, "(* 1)", "expects at least 2 args, but got 1");
    test_eval_error(e, "(* 1 \"a\")", "must be number, but is string");
    test_eval_error(e, "(* 1 -2 3 \"a\")", "must be number, but is string");
    test_eval_error(e, "(* 1 '())", "must be number, but got ()");

    // /
    test_eval_number(e, "(/ 1 1)", 1);
    test_eval_number(e, "(/ 1 2)", 0.5);
    test_eval_number(e, "(/ 2 1)", 2);
    test_eval_number(e, "(/ 1 -1)", -1);
    test_eval_number(e, "(/ -1 2)", -0.5);
    test_eval_number(e, "(/ 1 2 3 4 5)", 0.00833);
    test_eval_number(e, "(/ 3.14 2.71)", 1.15867);
    test_eval_number(e, "(/ 3.14 -2.71)", -1.15867);

    // / errors
    test_eval_error(e, "(/)", "expects at least 2 args, but got 0");
    test_eval_error(e, "(/ 1)", "expects at least 2 args, but got 1");
    test_eval_error(e, "(/ 1 \"a\")", "must be number, but is string");
    test_eval_error(e, "(/ 1 -2 3 \"a\")", "must be number, but is string");
    test_eval_error(e, "(/ 1 '())", "must be number, but got ()");
    test_eval_error(e, "(/ 1 0)", "division by zero");
    test_eval_error(e, "(/ 1 2 3 0)", "division by zero");

    // %
    test_eval_number(e, "(% 1 1)", 0);
    test_eval_number(e, "(% 1 3)", 1);
    test_eval_number(e, "(% 3 1)", 0);
    test_eval_number(e, "(% 2 3)", 2);
    test_eval_number(e, "(% 3 2)", 1);
    test_eval_number(e, "(% 1 -1)", 0);
    test_eval_number(e, "(% -1 2)", -1);
    test_eval_number(e, "(% 3 2)", 1);
    test_eval_number(e, "(% 3.14 2.71)", 0.43);
    test_eval_number(e, "(% 2 3 4 5)", 2);
    test_eval_number(e, "(% 5 4 3 2)", 1);

    // % errors
    test_eval_error(e, "(%)", "expects at least 2 args, but got 0");
    test_eval_error(e, "(% 1)", "expects at least 2 args, but got 1");
    test_eval_error(e, "(% 1 \"a\")", "must be number, but is string");
    test_eval_error(e, "(% 1 -2 3 \"a\")", "must be number, but is string");
    test_eval_error(e, "(% 1 '())", "must be number, but got ()");
    test_eval_error(e, "(% 1 0)", "division by zero");
    test_eval_error(e, "(% 1 2 3 0)", "division by zero");

    // ^
    test_eval_number(e, "(^ 1 2)", 1);
    test_eval_number(e, "(^ 2 1)", 2);
    test_eval_number(e, "(^ 2 2)", 4);
    test_eval_number(e, "(^ 2 0)", 1);
    test_eval_number(e, "(^ 0 2)", 0);
    test_eval_number(e, "(^ 1 -2)", 1);
    test_eval_number(e, "(^ 2 -1)", 0.5);
    test_eval_number(e, "(^ 2 -2)", 0.25);
    test_eval_number(e, "(^ -2 -2)", 0.25);
    test_eval_number(e, "(^ -2 -3)", -0.125);
    test_eval_number(e, "(^ 2 0.5)", 1.41421);
    test_eval_number(e, "(^ 2 1.5)", 2.82843);
    test_eval_number(e, "(^ 3.14 2)", 9.8596);
    test_eval_number(e, "(^ -3.14 3)", -30.9591);
    test_eval_number(e, "(^ 3.14 2.71)", 22.2167);
    test_eval_number(e, "(^ 2.71 3.14)", 22.8836);
    test_eval_number(e, "(^ -1 0.5)", NAN);
    test_eval_number(e, "(^ -1 1.5)", NAN);
    test_eval_number(e, "(^ 2 2 2 2 2)", 65536);
    test_eval_number(e, "(^ 1 2 3 4 5)", 1);
    test_eval_number(e, "(^ 3 4 5)", 3486784401);
    test_eval_number(e, "(^ -3 4 -5)", 2.86797e-10);

    // ^ errors
    test_eval_error(e, "(^)", "expects at least 2 args, but got 0");
    test_eval_error(e, "(^ 1)", "expects at least 2 args, but got 1");
    test_eval_error(e, "(^ 1 \"a\")", "must be number, but is string");
    test_eval_error(e, "(^ 1 -2 3 \"a\")", "must be number, but is string");
    test_eval_error(e, "(^ 1 '())", "must be number, but got ()");

    // min
    test_eval_number(e, "(min 1)", 1);
    test_eval_number(e, "(min -1)", -1);
    test_eval_number(e, "(min 1 2)", 1);
    test_eval_number(e, "(min 1 2 3)", 1);
    test_eval_number(e, "(min 1 -1 0)", -1);
    test_eval_number(e, "(min 1 1 1)", 1);
    test_eval_number(e, "(min 3.14 2.71)", 2.71);

    // min errors
    test_eval_error(e, "(min)", "expects at least 1 arg, but got 0");
    test_eval_error(e, "(min 1 \"a\")", "must be number, but is string");
    test_eval_error(e, "(min 1 -2 3 \"a\")", "must be number, but is string");
    test_eval_error(e, "(min 1 '())", "must be number, but got ()");

    // max
    test_eval_number(e, "(max 1)", 1);
    test_eval_number(e, "(max -1)", -1);
    test_eval_number(e, "(max 1 2)", 2);
    test_eval_number(e, "(max 1 2 3)", 3);
    test_eval_number(e, "(max 1 -1 0)", 1);
    test_eval_number(e, "(max 1 1 1)", 1);
    test_eval_number(e, "(max 3.14 2.71)", 3.14);

    // max errors
    test_eval_error(e, "(max)", "expects at least 1 arg, but got 0");
    test_eval_error(e, "(max 1 \"a\")", "must be number, but is string");
    test_eval_error(e, "(max 1 -2 3 \"a\")", "must be number, but is string");
    test_eval_error(e, "(max 1 '())", "must be number, but got ()");

    // compound arithmetics
    test_eval_number(e, "(+ 1 2 3 (- 4 5) 6)", 11);
    test_eval_number(e, "(min (max 1 3 5) (max 2 4 6))", 5);
    test_eval_number(e, "(+ 1 (* (^ 2 3) 4) (/ 5 6) (- 7 (% 8 9)) 10)", 42.8333);
}

void test_math(eval* e) {
    // constants
    test_eval_number(e, "PI", 3.14159265358979323846264338327950288);
    test_eval_number(e, "E", 2.71828182845904523536028747135266250);

    // abs
    test_eval_number(e, "(abs 1)", 1);
    test_eval_number(e, "(abs -1)", 1);
    test_eval_number(e, "(abs 0)", 0);
    test_eval_number(e, "(abs 3.14)", 3.14);
    test_eval_number(e, "(abs -3.14)", 3.14);

    // abs errors
    test_eval_error(e, "(abs)", "expects 1 arg, but got 0");
    test_eval_error(e, "(abs 1 2)", "expects 1 arg, but got 2");
    test_eval_error(e, "(abs \"a\")", "must be number, but is string");
    test_eval_error(e, "(abs '())", "must be number, but got ()");

    // exp
    test_eval_number(e, "(exp 0)", 1);
    test_eval_number(e, "(exp 1)", 2.71828);
    test_eval_number(e, "(exp -1)", 0.367879);
    test_eval_number(e, "(exp 2)", 7.38906);
    test_eval_number(e, "(exp -2)", 0.135335);
    test_eval_number(e, "(exp 0.5)", 1.64872);
    test_eval_number(e, "(exp -0.5)", 0.606531);

    // exp errors
    test_eval_error(e, "(exp)", "expects 1 arg, but got 0");
    test_eval_error(e, "(exp 1 2)", "expects 1 arg, but got 2");
    test_eval_error(e, "(exp \"a\")", "must be number, but is string");
    test_eval_error(e, "(exp '())", "must be number, but got ()");

    // log
    test_eval_number(e, "(log 1)", 0);
    test_eval_number(e, "(log 2)", 0.693147);
    test_eval_number(e, "(log 10)", 2.30259);
    test_eval_number(e, "(log 0.5)", -0.693147);

    // log errors
    test_eval_error(e, "(log)", "expects 1 arg, but got 0");
    test_eval_error(e, "(log 1 2)", "expects 1 arg, but got 2");
    test_eval_error(e, "(log \"a\")", "must be number, but is string");
    test_eval_error(e, "(log '())", "must be number, but got ()");
    test_eval_error(e, "(log 0)", "can't tage log of a non-positive");
    test_eval_error(e, "(log -1)", "can't tage log of a non-positive");

    // sin
    test_eval_number(e, "(sin 0)", 0);
    test_eval_number(e, "(sin (/ PI 2))", 1);
    test_eval_number(e, "(sin PI)", 0);
    test_eval_number(e, "(sin (* PI 2))", 0);
    test_eval_number(e, "(sin (- (/ PI 2)))", -1);
    test_eval_number(e, "(sin (- PI))", 0);
    test_eval_number(e, "(sin (- (* PI 2)))", 0);
    test_eval_number(e, "(sin 1)", 0.841471);
    test_eval_number(e, "(sin -1)", -0.841471);

    // sin errors
    test_eval_error(e, "(sin)", "expects 1 arg, but got 0");
    test_eval_error(e, "(sin 1 2)", "expects 1 arg, but got 2");
    test_eval_error(e, "(sin \"a\")", "must be number, but is string");
    test_eval_error(e, "(sin '())", "must be number, but got ()");

    // cos
    test_eval_number(e, "(cos 0)", 1);
    test_eval_number(e, "(cos (/ PI 2))", 0);
    test_eval_number(e, "(cos PI)", -1);
    test_eval_number(e, "(cos (* PI 2))", 1);
    test_eval_number(e, "(cos (- (/ PI 2)))", 0);
    test_eval_number(e, "(cos (- PI))", -1);
    test_eval_number(e, "(cos (- (* PI 2)))", 1);
    test_eval_number(e, "(cos 1)", 0.540302);
    test_eval_number(e, "(cos -1)", 0.540302);

    // cos errors
    test_eval_error(e, "(cos)", "expects 1 arg, but got 0");
    test_eval_error(e, "(cos 1 2)", "expects 1 arg, but got 2");
    test_eval_error(e, "(cos \"a\")", "must be number, but is string");
    test_eval_error(e, "(cos '())", "must be number, but got ()");

    // tan
    test_eval_number(e, "(tan 0)", 0);
    test_eval_number(e, "(tan PI)", 0);
    test_eval_number(e, "(tan (* PI 2))", 0);
    test_eval_number(e, "(tan (- PI))", 0);
    test_eval_number(e, "(tan (- (* PI 2)))", 0);
    test_eval_number(e, "(tan 1)", 1.55741);
    test_eval_number(e, "(tan -1)", -1.55741);

    // tan errors
    test_eval_error(e, "(tan)", "expects 1 arg, but got 0");
    test_eval_error(e, "(tan 1 2)", "expects 1 arg, but got 2");
    test_eval_error(e, "(tan \"a\")", "must be number, but is string");
    test_eval_error(e, "(tan '())", "must be number, but got ()");
}

void run_test() {
    eval* e;
    eval_new(&e, "./lib/machines/evaluator.scm");

    RUN_TEST_FN(test_parse);
    // RUN_TEST_FN(test_cycle_to_str);
    RUN_TEST_FN(test_pool);
    RUN_TEST_FN(test_machine);

    RUN_EVAL_TEST_FN(e, test_syntax);
    RUN_EVAL_TEST_FN(e, test_structural);
    RUN_EVAL_TEST_FN(e, test_arithmetic);
    RUN_EVAL_TEST_FN(e, test_math);

    printf("all tests have passed!\n");

    eval_dispose(&e);
}
