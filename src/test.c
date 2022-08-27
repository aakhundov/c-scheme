#include "test.h"

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "const.h"
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
        static char buffer[BUFFER_SIZE];   \
        value_to_str(v, buffer);           \
        printf("%s = %s\n", name, buffer); \
    }

static int test_counter = 0;

static void report_test(char* output, ...) {
    static char buffer[BUFFER_SIZE];

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

    static char output[BUFFER_SIZE];
    value_to_str(v, output);

    static char formatted[BUFFER_SIZE];
    sprintf(
        formatted,
        "\x1B[34m[\x1B[0m%s\x1B[34m] --> [\x1B[0m%s\x1B[34m]\x1B[0m",
        input, output);
    report_test("%s", formatted);

    return v;
}

static void test_parse_output(char* input, char* expected) {
    value* p = get_evaluated(NULL, input);

    static char buffer[BUFFER_SIZE];
    value_to_str(p, buffer);
    if (strcmp(buffer, expected) != 0) {
        printf("expected output: \"%s\"\n", expected);
        exit(EXIT_FAILURE);
    }
    value_dispose(p);
}

static void test_parse_error(char* input, char* expected) {
    value* p = get_evaluated(NULL, input);

    assert(p != NULL);
    assert(p->type == VALUE_ERROR);
    if (!strstr(p->symbol, expected)) {
        printf("expected substring: \"%s\"\n", expected);
        exit(EXIT_FAILURE);
    }
    value_dispose(p);
}

static void test_eval_output(eval* ev, char* input, char* expected) {
    value* e = get_evaluated(ev, input);

    static char buffer[BUFFER_SIZE];
    value_to_str(e, buffer);
    if (strcmp(buffer, expected) != 0) {
        printf("expected output: \"%s\"\n", expected);
        exit(EXIT_FAILURE);
    }
    value_dispose(e);
}

static void test_eval_number(eval* ev, char* input, double expected) {
    value* e = get_evaluated(ev, input);

    assert(e != NULL);
    assert(e->type == VALUE_NUMBER);
    if (!isnan(expected)) {
        if (fabs(e->number - expected) > 1e-4) {
            printf("expected number: %g\n", expected);
            exit(EXIT_FAILURE);
        }
    } else {
        if (!isnan(e->number)) {
            printf("expected number: nan\n");
            exit(EXIT_FAILURE);
        }
    }
    value_dispose(e);
}

static void test_eval_string(eval* ev, char* input, char* expected) {
    value* e = get_evaluated(ev, input);

    assert(e != NULL);
    assert(e->type == VALUE_STRING);
    if (strcmp(e->symbol, expected) != 0) {
        printf("expected string: \"%s\"\n", expected);
        exit(EXIT_FAILURE);
    }
    value_dispose(e);
}

static void test_eval_bool(eval* ev, char* input, int expected) {
    value* e = get_evaluated(ev, input);

    assert(e != NULL);
    assert(e->type == VALUE_BOOL);
    if ((expected && !value_is_true(e))) {
        printf("expected bool: true\n");
        exit(EXIT_FAILURE);
    }
    if (!expected && value_is_true(e)) {
        printf("expected bool: false\n");
        exit(EXIT_FAILURE);
    }

    value_dispose(e);
}

static void test_eval_error(eval* ev, char* input, char* expected) {
    value* e = get_evaluated(ev, input);

    assert(e != NULL);
    assert(e->type == VALUE_ERROR);
    if (!strstr(e->symbol, expected)) {
        printf("expected substring: \"%s\"\n", expected);
        exit(EXIT_FAILURE);
    }
    value_dispose(e);
}

static void test_eval_info(eval* ev, char* input, char* expected) {
    value* e = get_evaluated(ev, input);

    assert(e != NULL);
    assert(e->type == VALUE_INFO);
    if (!strstr(e->symbol, expected)) {
        printf("expected substring: \"%s\"\n", expected);
        exit(EXIT_FAILURE);
    }
    value_dispose(e);
}

static void test_to_str_output(char* name, value* v, char* expected) {
    static char buffer[BUFFER_SIZE];

    value_to_str(v, buffer);
    report_test("%s \x1B[34m--> [\x1B[0m%s\x1B[34m]\x1B[0m", name, buffer);
    if (strcmp(buffer, expected) != 0) {
        printf("expected string: \"%s\"\n", expected);
        exit(EXIT_FAILURE);
    }

    if (v != NULL) {
        value_dispose(v);
    }
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
    test_parse_output("(. 2)", "(2)");
    test_parse_output("(.1 . 2.)", "((0.1 . 2))");
    test_parse_output("(1.2 . 3.4)", "((1.2 . 3.4))");
    test_parse_output("(1 2 3 . 4)", "((1 2 3 . 4))");
    test_parse_output("'(1 . 2)", "((quote (1 . 2)))");
    test_parse_output("'(. 2)", "((quote 2))");

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

    // parsing errors
    test_parse_error("(1 2", "missing )");
    test_parse_error("1 2)", "premature )");
    test_parse_error("( 1 (2", "missing )");
    test_parse_error("'", "unfollowed '");
    test_parse_error("(1 .)", "unfollowed .");
    test_parse_error("(1 . 2 3)", ". followed by 2+ items");
    test_parse_error("(1 . .)", ". followed by .");
    test_parse_error("(. 2 3)", ". followed by 2+ items");
    test_parse_error("\"", "unterminated string");
    test_parse_error("\"xyz", "unterminated string");
    test_parse_error(" \" xyz ", "unterminated string");
    test_parse_error("\"xyz\" \"a", "unterminated string");
}

void test_to_str() {
    value* v = NULL;

    test_to_str_output("integer", value_new_number(1), "1");
    test_to_str_output("negative", value_new_number(-1), "-1");
    test_to_str_output("decimal", value_new_number(3.14), "3.14");
    test_to_str_output("symbol", value_new_symbol("abc"), "abc");
    test_to_str_output("empty symbol", value_new_symbol(""), "");
    test_to_str_output("empty symbol", value_new_string("abc"), "\"abc\"");
    test_to_str_output("empty string", value_new_string(""), "\"\"");
    test_to_str_output("true", value_new_bool(1), "true");
    test_to_str_output("false", value_new_bool(0), "false");
    test_to_str_output("nil", NULL, "()");

    v = value_new_pair(
        value_new_number(1),
        value_new_number(2));
    test_to_str_output("singleton pair", v, "(1 . 2)");

    v = value_new_pair(
        value_new_number(1),
        value_new_pair(
            value_new_number(2),
            value_new_number(3)));
    test_to_str_output("multiple pair", v, "(1 2 . 3)");

    v = value_new_pair(
        value_new_pair(
            value_new_number(1),
            value_new_pair(
                value_new_number(2),
                NULL)),
        value_new_number(3));
    test_to_str_output("nested pair", v, "((1 2) . 3)");

    v = value_new_pair(
        value_new_number(1),
        NULL);
    test_to_str_output("singleton list", v, "(1)");

    v = value_new_pair(
        value_new_number(1),
        value_new_pair(
            value_new_number(2),
            value_new_pair(
                value_new_number(3),
                NULL)));
    test_to_str_output("multiple list", v, "(1 2 3)");

    v = value_new_pair(
        value_new_number(1),
        value_new_pair(
            value_new_number(2),
            value_new_pair(
                value_new_number(3),
                NULL)));
    test_to_str_output("multiple list", v, "(1 2 3)");

    v = value_new_pair(
        value_new_pair(
            value_new_number(1),
            value_new_pair(
                value_new_number(2),
                NULL)),
        value_new_pair(
            value_new_number(3),
            value_new_pair(
                value_new_number(4),
                value_new_pair(
                    value_new_number(5),
                    NULL))));
    test_to_str_output("nested list 1", v, "((1 2) 3 4 5)");

    v = value_new_pair(
        value_new_pair(
            value_new_number(1),
            value_new_pair(
                value_new_number(2),
                NULL)),
        value_new_pair(
            value_new_pair(
                value_new_number(3),
                value_new_pair(
                    value_new_number(4),
                    value_new_pair(
                        value_new_number(5),
                        NULL))),
            NULL));
    test_to_str_output("nested list 2", v, "((1 2) (3 4 5))");

    v = value_new_pair(
        value_new_number(1),
        value_new_pair(
            value_new_number(2),
            value_new_pair(
                NULL,
                NULL)));
    v->cdr->cdr->car = v;
    test_to_str_output("car cycle", v, "(1 2 <cycle>)");

    v = value_new_pair(
        value_new_number(1),
        value_new_pair(
            value_new_number(2),
            value_new_pair(
                value_new_number(3),
                NULL)));
    v->cdr->cdr->cdr = v;
    test_to_str_output("cdr cycle", v, "(1 2 3 . <cycle>)");

    v = value_new_pair(
        value_new_number(1),
        value_new_pair(
            NULL,
            value_new_pair(
                value_new_number(3),
                NULL)));
    v->cdr->car = v;
    v->cdr->cdr->cdr = v;
    test_to_str_output("car and cdr cycle", v, "(1 <cycle> 3 . <cycle>)");

    v = value_new_pair(NULL, NULL);
    v->car = v;
    v->cdr = v;
    test_to_str_output("self cycle", v, "(<cycle> . <cycle>)");

    v = value_new_error("some text");
    test_to_str_output("error", v, "\x1B[31msome text\x1B[0m");

    v = value_new_error("(%d %g %s)", 1, 3.14, "hello");
    test_to_str_output("error with args", v, "\x1B[31m(1 3.14 hello)\x1B[0m");

    v = value_new_info("some text");
    test_to_str_output("info", v, "\x1B[32msome text\x1B[0m");

    v = value_new_info("(%d %g %s)", 1, 3.14, "hello");
    test_to_str_output("info with args", v, "\x1B[32m(1 3.14 hello)\x1B[0m");

    v = value_new_lambda(
        value_new_pair(
            value_new_pair(
                value_new_symbol("x"),
                value_new_pair(
                    value_new_symbol("y"),
                    NULL)),
            value_new_pair(
                value_new_pair(
                    value_new_symbol("+"),
                    value_new_pair(
                        value_new_symbol("x"),
                        value_new_pair(
                            value_new_symbol("y"),
                            NULL))),
                NULL)),
        value_new_env());
    test_to_str_output("lambda", v, "(lambda (x y) (+ x y))");

    test_to_str_output("code", value_new_code(NULL, NULL), "<code>");
    test_to_str_output("env", value_new_env(), "<env>");
}

static void test_pool() {
    // setup
    value* r1 = value_new_pair(NULL, NULL);
    value* r2 = value_new_pair(NULL, NULL);
    value* source = NULL;
    value* dest = NULL;

    // init
    report_test("init");
    pool* p = pool_new();
    assert(p->size == 0);

    // singleton values
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

    // compound values
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

    // memory roots
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

    // value cycle
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

    // import
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

    // import cycle
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

    // export
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

    // export cycle
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

    // cleanup
    report_test("cleanup");
    pool_new_number(p, 123);
    pool_new_symbol(p, "hello");
    pool_new_string(p, "world");
    assert(p->size == 3);
    pool_dispose(p);

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

    machine* m = machine_new(code, "a");

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

        static char buffer[BUFFER_SIZE];
        value_to_str(result, buffer);
        report_test("gcd(%d, %d) --> %s", a, b, buffer);
        assert(result->number == val);
        value_dispose(result);
    }

    value_dispose(code);
    machine_dispose(m);
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

    machine* m = machine_new(code, "val");

    machine_bind_op(m, "-", op_minus);
    machine_bind_op(m, "*", op_mult);
    machine_bind_op(m, "=", op_eq);

    for (size_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++) {
        int n = test_data[i][0];
        int val = test_data[i][1];

        machine_copy_to_register(m, "n", pool_new_number(m->pool, n));
        machine_run(m);

        value* result = machine_export_output(m);

        static char buffer[BUFFER_SIZE];
        value_to_str(result, buffer);
        report_test("factorial(%d) --> %s", n, buffer);
        assert(result->number == val);
        value_dispose(result);
    }

    value_dispose(code);
    machine_dispose(m);
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

    machine* m = machine_new(code, "val");

    machine_bind_op(m, "<", op_lt);
    machine_bind_op(m, "+", op_plus);
    machine_bind_op(m, "-", op_minus);

    for (size_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++) {
        int n = test_data[i][0];
        int val = test_data[i][1];

        machine_copy_to_register(m, "n", pool_new_number(m->pool, n));
        machine_run(m);

        value* result = machine_export_output(m);

        static char buffer[BUFFER_SIZE];
        value_to_str(result, buffer);
        report_test("fibonacci(%d) --> %s", n, buffer);
        assert(result->number == val);
        value_dispose(result);
    }

    value_dispose(code);
    machine_dispose(m);
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

    // quote errors
    test_eval_error(e, "(quote)", "no expression");
    test_eval_error(e, "(quote 1 2)", "more than one item");
    test_eval_error(e, "(quote 1 . 2)", "non-list structure");
    test_eval_error(e, "(quote . 1)", "non-list structure");

    // set!
    test_eval_error(e, "b", "b is unbound");
    test_eval_error(e, "(set! b 30)", "b is unbound");
    test_eval_info(e, "(define b 20)", "b is defined");
    test_eval_number(e, "b", 20);
    test_eval_info(e, "(set! b 30)", "b is updated");
    test_eval_number(e, "b", 30);
    test_eval_error(e, "s1", "s1 is unbound");
    test_eval_info(e, "(define (sf1) (set! s1 10) s1)", "sf1 is defined");
    test_eval_error(e, "(sf1)", "s1 is unbound");
    test_eval_error(e, "s1", "s1 is unbound");
    test_eval_info(e, "(define s1 20)", "s1 is defined");
    test_eval_number(e, "s1", 20);
    test_eval_number(e, "(sf1)", 10);
    test_eval_number(e, "s1", 10);

    // set! errors
    test_eval_error(e, "(set!)", "no variable");
    test_eval_error(e, "(set! y)", "no value");
    test_eval_error(e, "(set! 1 30)", "variable is not a symbol");
    test_eval_error(e, "(set! y 30 40)", "more than two items");
    test_eval_error(e, "(set! 1 . 2)", "non-list structure");
    test_eval_error(e, "(set! . 1)", "non-list structure");

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
    test_eval_error(e, "d1", "d1 is unbound");
    test_eval_info(e, "(define (df1) (define d1 10) d1)", "df1 is defined");
    test_eval_number(e, "(df1)", 10);
    test_eval_error(e, "d1", "d1 is unbound");
    test_eval_info(e, "(define d1 20)", "d1 is defined");
    test_eval_number(e, "d1", 20);
    test_eval_number(e, "(df1)", 10);
    test_eval_number(e, "d1", 20);

    // define errors
    test_eval_error(e, "(define)", "no variable");
    test_eval_error(e, "(define ())", "can't define ()");
    test_eval_error(e, "(define (f))", "no body");
    test_eval_error(e, "(define (1) 1)", "function name is not a symbol");
    test_eval_error(e, "(define (1 x y) 1)", "function name is not a symbol");
    test_eval_error(e, "(define x 1 2)", "can't be more than one item");
    test_eval_error(e, "(define 1 2)", "either variable or function must be defined");
    test_eval_error(e, "(define \"x\" 2)", "either variable or function must be defined");
    test_eval_error(e, "(define 1 . 2)", "non-list structure");
    test_eval_error(e, "(define . 1)", "non-list structure");

    // if
    test_eval_number(e, "(if 1 2 3)", 2);
    test_eval_number(e, "(if 0 2 3)", 2);
    test_eval_number(e, "(if \"\" 2 3)", 2);
    test_eval_number(e, "(if \"x\" 2 3)", 2);
    test_eval_number(e, "(if () 2 3)", 2);
    test_eval_number(e, "(if nil 2 3)", 2);
    test_eval_number(e, "(if true 2 3)", 2);
    test_eval_number(e, "(if false 2 3)", 3);
    test_eval_number(e, "(if 1 2)", 2);
    test_eval_number(e, "(if true 2)", 2);
    test_eval_number(e, "(if 0 2)", 2);
    test_eval_bool(e, "(if false 2)", 0);

    // if errors
    test_eval_error(e, "(if)", "no predicate");
    test_eval_error(e, "(if 1)", "no consequent");
    test_eval_error(e, "(if 1 2 3 4)", "too many items");
    test_eval_error(e, "(if 1 . 2)", "non-list structure");
    test_eval_error(e, "(if . 1)", "non-list structure");

    // lambda
    test_eval_output(e, "(lambda x x)", "(lambda x x)");
    test_eval_output(e, "(lambda (. x) x)", "(lambda x x)");
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
    test_eval_error(e, "(lambda 1 . 2)", "non-list structure");
    test_eval_error(e, "(lambda . 1)", "non-list structure");

    // let
    test_eval_number(e, "(let () 1)", 1);
    test_eval_number(e, "(let ((x 10)) x)", 10);
    test_eval_bool(e, "(let ((x true)) x)", 1);
    test_eval_string(e, "(let ((x \"abc\")) x)", "abc");
    test_eval_output(e, "(let ((x '(1 2 3))) x)", "(1 2 3)");
    test_eval_output(e, "(let ((x ())) x)", "()");
    test_eval_number(e, "(let ((x 10) (y 20)) x y)", 20);
    test_eval_number(e, "(let ((x 10) (y 20)) (+ x y))", 30);
    test_eval_number(e, "(let ((x 10) (y 20)) (let ((z 30)) (+ (* x y) z)))", 230);
    test_eval_number(e, "(let ((x 10)) (let ((y 20)) (let ((z 30)) (+ x (* y z)))))", 610);

    // let errors
    test_eval_error(e, "(let)", "no variables");
    test_eval_error(e, "(let ((x 10)))", "no body");
    test_eval_error(e, "(let 1 2)", "non-list variables");
    test_eval_error(e, "(let (()) 1)", "no variable name");
    test_eval_error(e, "(let (1) 2)", "non-list variable pair");
    test_eval_error(e, "(let ((a . 1)) 3)", "non-list variable pair");
    test_eval_error(e, "(let ((a)) 1)", "no variable value");
    test_eval_error(e, "(let ((a 1 2)) 3)", "too many items in a variable pair");
    test_eval_error(e, "(let ((1 2)) 3)", "variable name must be a symbol");
    test_eval_error(e, "(let ((1)) 3)", "variable name must be a symbol");
    test_eval_error(e, "(let 1 . 2)", "non-list structure");
    test_eval_error(e, "(let . 1)", "non-list structure");

    // begin
    test_eval_number(e, "(begin 1)", 1);
    test_eval_number(e, "(begin 1 2 3)", 3);
    test_eval_bool(e, "(begin true false)", 0);
    test_eval_output(e, "(begin '(1 2 3) '(4 5) '())", "()");

    // begin errors
    test_eval_error(e, "(begin)", "no expressions");
    test_eval_error(e, "(begin 1 . 2)", "non-list structure");
    test_eval_error(e, "(begin . 1)", "non-list structure");

    // cond
    test_eval_number(e, "(cond (else 1))", 1);
    test_eval_number(e, "(cond (1 2))", 2);
    test_eval_number(e, "(cond (1 2) (else 3))", 2);
    test_eval_number(e, "(cond (false 2) (1 3))", 3);
    test_eval_bool(e, "(cond (false 2))", 0);
    test_eval_number(e, "(cond (false 2) (else 3))", 3);
    test_eval_number(e, "(cond (1 2) (1 3) (else 4))", 2);
    test_eval_number(e, "(cond (false 2) (1 3) (else 4))", 3);
    test_eval_bool(e, "(cond (false 2) (false 3))", 0);
    test_eval_number(e, "(cond (false 2) (false 3) (else 4))", 4);
    test_eval_info(e, "(define (c1 x) (cond ((> x 0) 1) ((< x 0) -1) (else 0)))", "c1 is defined");
    test_eval_number(e, "(c1 1)", 1);
    test_eval_number(e, "(c1 10)", 1);
    test_eval_number(e, "(c1 100)", 1);
    test_eval_number(e, "(c1 -1)", -1);
    test_eval_number(e, "(c1 -10)", -1);
    test_eval_number(e, "(c1 -100)", -1);
    test_eval_number(e, "(c1 0)", 0);
    test_eval_info(e, "(define (c2 x) (cond ((> x 0) 1) ((< x 0) -1)))", "c2 is defined");
    test_eval_number(e, "(c2 1)", 1);
    test_eval_number(e, "(c2 10)", 1);
    test_eval_number(e, "(c2 100)", 1);
    test_eval_number(e, "(c2 -1)", -1);
    test_eval_number(e, "(c2 -10)", -1);
    test_eval_number(e, "(c2 -100)", -1);
    test_eval_bool(e, "(c2 0)", 0);
    test_eval_info(e, "(define (c3 x) (cond (else 3.14)))", "c3 is defined");
    test_eval_number(e, "(c3 -100)", 3.14);
    test_eval_number(e, "(c3 100)", 3.14);
    test_eval_number(e, "(c3 0)", 3.14);

    // cond errors
    test_eval_error(e, "(cond)", "no clauses");
    test_eval_error(e, "(cond ())", "empty clause");
    test_eval_error(e, "(cond (1 2) ())", "empty clause");
    test_eval_error(e, "(cond 1)", "non-list clause");
    test_eval_error(e, "(cond (1 2) 1)", "non-list clause");
    test_eval_error(e, "(cond (1))", "actionless clause");
    test_eval_error(e, "(cond (1 2) (1))", "actionless clause");
    test_eval_error(e, "(cond (else 2) (1))", "else clause must be the last");
    test_eval_error(e, "(cond 1 . 2)", "non-list structure");
    test_eval_error(e, "(cond . 1)", "non-list structure");

    // and
    test_eval_bool(e, "(and)", 1);
    test_eval_number(e, "(and 1)", 1);
    test_eval_number(e, "(and 2)", 2);
    test_eval_number(e, "(and 1 2)", 2);
    test_eval_number(e, "(and 1 2 3)", 3);
    test_eval_number(e, "(and 3 2 1)", 1);
    test_eval_string(e, "(and 3 2 \"abc\")", "abc");
    test_eval_bool(e, "(and 3 2 true)", 1);
    test_eval_output(e, "(and 3 2 ())", "()");
    test_eval_bool(e, "(and false)", 0);
    test_eval_bool(e, "(and 1 false)", 0);
    test_eval_bool(e, "(and false 1)", 0);
    test_eval_bool(e, "(and 1 2 3 false)", 0);
    test_eval_bool(e, "(and 3 2 1 false)", 0);
    test_eval_bool(e, "(and false 1 2 3)", 0);
    test_eval_bool(e, "(and false (/ 1 0))", 0);
    test_eval_error(e, "(and (/ 1 0) false)", "division by zero");
    test_eval_error(e, "and1", "and1 is unbound");
    test_eval_info(e, "(and (define and1 1) (define and1 2) (define and1 3))", "and1 is updated");
    test_eval_number(e, "and1", 3);
    test_eval_bool(e, "(and (= and1 1) (= and1 2) (= and1 3))", 0);
    test_eval_bool(e, "(and (> and1 0) (> and1 1) (> and1 2))", 1);

    // and errors
    test_eval_error(e, "(and 1 . 2)", "non-list structure");
    test_eval_error(e, "(and . 1)", "non-list structure");

    // or
    test_eval_bool(e, "(or)", 0);
    test_eval_number(e, "(or 1)", 1);
    test_eval_number(e, "(or 2)", 2);
    test_eval_number(e, "(or 1 2)", 1);
    test_eval_number(e, "(or 1 2 3)", 1);
    test_eval_number(e, "(or 3 2 1)", 3);
    test_eval_number(e, "(or 3 2 \"abc\")", 3);
    test_eval_string(e, "(or \"abc\" 3 2)", "abc");
    test_eval_number(e, "(or 3 2 true)", 3);
    test_eval_bool(e, "(or true 3 2)", 1);
    test_eval_number(e, "(or 3 2 ())", 3);
    test_eval_output(e, "(or () 3 2)", "()");
    test_eval_bool(e, "(or false)", 0);
    test_eval_number(e, "(or 1 false)", 1);
    test_eval_number(e, "(or false 1)", 1);
    test_eval_number(e, "(or 1 2 3 false)", 1);
    test_eval_number(e, "(or false 1 2 3)", 1);
    test_eval_number(e, "(or false 3 2 1)", 3);
    test_eval_bool(e, "(or true (/ 1 0))", 1);
    test_eval_error(e, "(or (/ 1 0) true)", "division by zero");
    test_eval_error(e, "or1", "or1 is unbound");
    test_eval_info(e, "(or (define or1 1) (define or1 2) (define or1 3))", "or1 is defined");
    test_eval_number(e, "or1", 1);
    test_eval_bool(e, "(or (= or1 1) (= or1 2) (= or1 3))", 1);
    test_eval_bool(e, "(or (> or1 1) (> or1 2) (> or1 3))", 0);

    // or errors
    test_eval_error(e, "(or 1 . 2)", "non-list structure");
    test_eval_error(e, "(or . 1)", "non-list structure");

    // eval
    test_eval_number(e, "(eval 1)", 1);
    test_eval_number(e, "(eval 3.14)", 3.14);
    test_eval_bool(e, "(eval true)", 1);
    test_eval_output(e, "(eval ())", "()");
    test_eval_info(e, "(define e1 10)", "e1 is defined");
    test_eval_number(e, "(eval e1)", 10);
    test_eval_number(e, "(eval 'e1)", 10);
    test_eval_output(e, "(eval ''e1)", "e1");
    test_eval_number(e, "(eval '(+ 1 2 3))", 6);
    test_eval_number(e, "(eval (cons + '(1 2 3)))", 6);
    test_eval_info(e, "(define e2 '(+ 1 2 3))", "e2 is defined");
    test_eval_number(e, "(eval e2)", 6);
    test_eval_output(e, "(eval 'e2)", "(+ 1 2 3)");
    test_eval_output(e, "(eval ''e2)", "e2");
    test_eval_info(e, "(define e3 '(cons + '(1 2 3)))", "e3 is defined");
    test_eval_output(e, "(eval e3)", "(<builtin '+'> 1 2 3)");
    test_eval_output(e, "(eval 'e3)", "(cons + (quote (1 2 3)))");
    test_eval_number(e, "(eval (eval e3))", 6);

    // eval errors
    test_eval_error(e, "(eval)", "no expression");
    test_eval_error(e, "(eval 1 2)", "too many items");
    test_eval_error(e, "(eval 1 . 2)", "non-list structure");
    test_eval_error(e, "(eval . 1)", "non-list structure");

    // apply
    test_eval_number(e, "(apply + '(1))", 1);
    test_eval_number(e, "(apply - '(1))", -1);
    test_eval_number(e, "(apply + '(1 2 3))", 6);
    test_eval_info(e, "(define a1 10)", "a1 is defined");
    test_eval_number(e, "(apply (if true + -) (list 1 a1 3))", 14);
    test_eval_number(e, "(apply (if false + -) (list 1 a1 3))", -12);
    test_eval_output(e, "(apply car '((1 2)))", "1");
    test_eval_output(e, "(apply cdr '((1 2)))", "(2)");
    test_eval_output(e, "(apply cons '(0 (1 2)))", "(0 1 2)");
    test_eval_number(e, "(apply (car (list + -)) (cdr '(1 2 3)))", 5);
    test_eval_number(e, "(apply (eval (car '(+ -))) (cdr '(1 2 3)))", 5);
    test_eval_number(e, "(+ (apply (eval (car '(+ -))) (cdr '(1 2 3))) a1)", 15);
    test_eval_output(e, "(apply car '('(1 2)))", "quote");
    test_eval_output(e, "(apply cdr '('(1 2)))", "((1 2))");
    test_eval_output(e, "(apply cons '('0 '(1 2)))", "((quote 0) quote (1 2))");
    test_eval_number(e, "(apply * (list (apply + '(1 2 3)) (apply - '(4 5 6))))", -42);

    // apply errors
    test_eval_error(e, "(apply)", "no operator");
    test_eval_error(e, "(apply 1)", "no arguments");
    test_eval_error(e, "(apply 1 2 3)", "too many items");
    test_eval_error(e, "(apply 1 '(1 2 3))", "can't apply 1");
    test_eval_error(e, "(apply + 1)", "can't apply to 1");
    test_eval_error(e, "(apply + \"a\")", "can't apply to \"a\"");
    test_eval_error(e, "(apply + '(1 . 2))", "can't apply to (1 . 2)");
    test_eval_error(e, "(apply 1 . 2)", "non-list structure");
    test_eval_error(e, "(apply . 1)", "non-list structure");

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
    test_eval_error(e, "(+ 1 . 2)", "can't apply to (1 . 2)");
    test_eval_error(e, "(+ . 1)", "can't apply to 1");
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

    // list
    test_eval_output(e, "(list)", "()");
    test_eval_output(e, "(list 1)", "(1)");
    test_eval_output(e, "(list 1 2 3 4)", "(1 2 3 4)");
    test_eval_output(e, "(list 3.14 \"a\" true '(1 2) '())", "(3.14 \"a\" true (1 2) ())");

    // set-car!
    test_eval_error(e, "x", "x is unbound");
    test_eval_info(e, "(define x '(1 . 2))", "x is defined");
    test_eval_output(e, "x", "(1 . 2)");
    test_eval_info(e, "(set-car! x 3)", "car is set");
    test_eval_output(e, "x", "(3 . 2)");
    test_eval_info(e, "(set-car! x (+ (car x) 1))", "car is set");
    test_eval_output(e, "x", "(4 . 2)");
    test_eval_info(e, "(set-car! x (cdr x))", "car is set");
    test_eval_output(e, "x", "(2 . 2)");
    test_eval_info(e, "(set-car! x (+ (cdr x) 1))", "car is set");
    test_eval_output(e, "x", "(3 . 2)");
    test_eval_info(e, "(define x '(1))", "x is updated");
    test_eval_output(e, "x", "(1)");
    test_eval_info(e, "(set-car! x 2)", "car is set");
    test_eval_output(e, "x", "(2)");
    test_eval_info(e, "(set-car! x 3)", "car is set");
    test_eval_output(e, "x", "(3)");
    test_eval_info(e, "(set-car! x '(2))", "car is set");
    test_eval_output(e, "x", "((2))");
    test_eval_info(e, "(set-car! x '(3))", "car is set");
    test_eval_output(e, "x", "((3))");

    // set-car! errors
    test_eval_error(e, "(set-car!)", "expects 2 args, but got 0");
    test_eval_error(e, "(set-car! x)", "expects 2 args, but got 1");
    test_eval_error(e, "(set-car! x 1 2)", "expects 2 args, but got 3");
    test_eval_error(e, "(set-car! 1 2)", "must be pair, but is number");
    test_eval_error(e, "(set-car! () 2)", "must be pair, but got ()");

    // set-cdr!
    test_eval_error(e, "y", "y is unbound");
    test_eval_info(e, "(define y '(1 . 2))", "y is defined");
    test_eval_output(e, "y", "(1 . 2)");
    test_eval_info(e, "(set-cdr! y 3)", "cdr is set");
    test_eval_output(e, "y", "(1 . 3)");
    test_eval_info(e, "(set-cdr! y (+ (cdr y) 1))", "cdr is set");
    test_eval_output(e, "y", "(1 . 4)");
    test_eval_info(e, "(set-cdr! y (car y))", "cdr is set");
    test_eval_output(e, "y", "(1 . 1)");
    test_eval_info(e, "(set-cdr! y (+ (car y) 1))", "cdr is set");
    test_eval_output(e, "y", "(1 . 2)");
    test_eval_info(e, "(define x '(1))", "x is updated");
    test_eval_output(e, "x", "(1)");
    test_eval_info(e, "(set-cdr! x 2)", "cdr is set");
    test_eval_output(e, "x", "(1 . 2)");
    test_eval_info(e, "(set-cdr! x 3)", "cdr is set");
    test_eval_output(e, "x", "(1 . 3)");
    test_eval_info(e, "(set-cdr! x '(2))", "cdr is set");
    test_eval_output(e, "x", "(1 2)");
    test_eval_info(e, "(set-cdr! x '(3))", "cdr is set");
    test_eval_output(e, "x", "(1 3)");

    // set-cdr! errors
    test_eval_error(e, "(set-cdr!)", "expects 2 args, but got 0");
    test_eval_error(e, "(set-cdr! y)", "expects 2 args, but got 1");
    test_eval_error(e, "(set-cdr! y 1 2)", "expects 2 args, but got 3");
    test_eval_error(e, "(set-cdr! 1 2)", "must be pair, but is number");
    test_eval_error(e, "(set-cdr! () 2)", "must be pair, but got ()");
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

    // atan
    test_eval_number(e, "(atan 0)", 0);
    test_eval_number(e, "(atan PI)", 1.26263);
    test_eval_number(e, "(atan (* PI 2))", 1.41297);
    test_eval_number(e, "(atan (- PI))", -1.26263);
    test_eval_number(e, "(atan (- (* PI 2)))", -1.41297);
    test_eval_number(e, "(atan 1)", 0.785398);
    test_eval_number(e, "(atan -1)", -0.785398);

    // atan errors
    test_eval_error(e, "(atan)", "expects 1 arg, but got 0");
    test_eval_error(e, "(atan 1 2)", "expects 1 arg, but got 2");
    test_eval_error(e, "(atan \"a\")", "must be number, but is string");
    test_eval_error(e, "(atan '())", "must be number, but got ()");

    // atan2
    test_eval_number(e, "(atan2 0 0)", 0);
    test_eval_number(e, "(atan2 0 1)", 0);
    test_eval_number(e, "(atan2 1 0)", 1.5708);
    test_eval_number(e, "(atan2 1 1)", 0.785398);
    test_eval_number(e, "(atan2 0 PI)", 0);
    test_eval_number(e, "(atan2 1 0)", 1.5708);
    test_eval_number(e, "(atan2 PI 1)", 1.26263);

    // atan2 errors
    test_eval_error(e, "(atan2)", "expects 2 args, but got 0");
    test_eval_error(e, "(atan2 1)", "expects 2 args, but got 1");
    test_eval_error(e, "(atan2 1 2 3)", "expects 2 args, but got 3");
    test_eval_error(e, "(atan2 1 \"a\")", "must be number, but is string");
    test_eval_error(e, "(atan2 1 '())", "must be number, but got ()");
}

void test_relational(eval* e) {
    // =
    test_eval_bool(e, "(= 1 1)", 1);
    test_eval_bool(e, "(= 1 2)", 0);
    test_eval_bool(e, "(= 2 1)", 0);
    test_eval_bool(e, "(= 1 1 1)", 1);
    test_eval_bool(e, "(= 1 2 1)", 0);
    test_eval_bool(e, "(= 1 1 2)", 0);
    test_eval_bool(e, "(= 1 2 2)", 0);
    test_eval_bool(e, "(= 2 2 2)", 1);
    test_eval_bool(e, "(= 1 1 1 1 1)", 1);
    test_eval_bool(e, "(= 1 1 1 1 2)", 0);

    // = errors
    test_eval_error(e, "(=)", "expects at least 2 args, but got 0");
    test_eval_error(e, "(= 1)", "expects at least 2 args, but got 1");
    test_eval_error(e, "(= 1 \"a\")", "must be number, but is string");
    test_eval_error(e, "(= 1 '())", "must be number, but got ()");

    // <
    test_eval_bool(e, "(< 1 2)", 1);
    test_eval_bool(e, "(< 1 1)", 0);
    test_eval_bool(e, "(< 2 1)", 0);
    test_eval_bool(e, "(< 1 1 1)", 0);
    test_eval_bool(e, "(< 1 2 1)", 0);
    test_eval_bool(e, "(< 1 1 2)", 0);
    test_eval_bool(e, "(< 1 2 2)", 0);
    test_eval_bool(e, "(< 2 2 2)", 0);
    test_eval_bool(e, "(< 1 2 3)", 1);
    test_eval_bool(e, "(< 1 2 2)", 0);
    test_eval_bool(e, "(< 3 2 1)", 0);
    test_eval_bool(e, "(< 3 2 2)", 0);

    // < errors
    test_eval_error(e, "(<)", "expects at least 2 args, but got 0");
    test_eval_error(e, "(< 1)", "expects at least 2 args, but got 1");
    test_eval_error(e, "(< 1 \"a\")", "must be number, but is string");
    test_eval_error(e, "(< 1 '())", "must be number, but got ()");

    // <=
    test_eval_bool(e, "(<= 1 2)", 1);
    test_eval_bool(e, "(<= 1 1)", 1);
    test_eval_bool(e, "(<= 2 1)", 0);
    test_eval_bool(e, "(<= 1 1 1)", 1);
    test_eval_bool(e, "(<= 1 2 1)", 0);
    test_eval_bool(e, "(<= 1 1 2)", 1);
    test_eval_bool(e, "(<= 1 2 2)", 1);
    test_eval_bool(e, "(<= 2 2 2)", 1);
    test_eval_bool(e, "(<= 1 2 3)", 1);
    test_eval_bool(e, "(<= 1 2 2)", 1);
    test_eval_bool(e, "(<= 3 2 1)", 0);
    test_eval_bool(e, "(<= 3 2 2)", 0);

    // <= errors
    test_eval_error(e, "(<=)", "expects at least 2 args, but got 0");
    test_eval_error(e, "(<= 1)", "expects at least 2 args, but got 1");
    test_eval_error(e, "(<= 1 \"a\")", "must be number, but is string");
    test_eval_error(e, "(<= 1 '())", "must be number, but got ()");

    // >
    test_eval_bool(e, "(> 1 2)", 0);
    test_eval_bool(e, "(> 1 1)", 0);
    test_eval_bool(e, "(> 2 1)", 1);
    test_eval_bool(e, "(> 1 1 1)", 0);
    test_eval_bool(e, "(> 1 2 1)", 0);
    test_eval_bool(e, "(> 1 1 2)", 0);
    test_eval_bool(e, "(> 1 2 2)", 0);
    test_eval_bool(e, "(> 2 2 2)", 0);
    test_eval_bool(e, "(> 1 2 3)", 0);
    test_eval_bool(e, "(> 1 2 2)", 0);
    test_eval_bool(e, "(> 3 2 1)", 1);
    test_eval_bool(e, "(> 3 2 2)", 0);

    // > errors
    test_eval_error(e, "(>)", "expects at least 2 args, but got 0");
    test_eval_error(e, "(> 1)", "expects at least 2 args, but got 1");
    test_eval_error(e, "(> 1 \"a\")", "must be number, but is string");
    test_eval_error(e, "(> 1 '())", "must be number, but got ()");

    // >=
    test_eval_bool(e, "(>= 1 2)", 0);
    test_eval_bool(e, "(>= 1 1)", 1);
    test_eval_bool(e, "(>= 2 1)", 1);
    test_eval_bool(e, "(>= 1 1 1)", 1);
    test_eval_bool(e, "(>= 1 2 1)", 0);
    test_eval_bool(e, "(>= 1 1 2)", 0);
    test_eval_bool(e, "(>= 1 2 2)", 0);
    test_eval_bool(e, "(>= 2 2 2)", 1);
    test_eval_bool(e, "(>= 1 2 3)", 0);
    test_eval_bool(e, "(>= 1 2 2)", 0);
    test_eval_bool(e, "(>= 3 2 1)", 1);
    test_eval_bool(e, "(>= 3 2 2)", 1);

    // >= errors
    test_eval_error(e, "(>=)", "expects at least 2 args, but got 0");
    test_eval_error(e, "(>= 1)", "expects at least 2 args, but got 1");
    test_eval_error(e, "(>= 1 \"a\")", "must be number, but is string");
    test_eval_error(e, "(>= 1 '())", "must be number, but got ()");

    // not
    test_eval_bool(e, "(not 1)", 0);
    test_eval_bool(e, "(not 0)", 0);
    test_eval_bool(e, "(not true)", 0);
    test_eval_bool(e, "(not false)", 1);
    test_eval_bool(e, "(not \"a\")", 0);
    test_eval_bool(e, "(not \"\")", 0);
    test_eval_bool(e, "(not '(1 2 3))", 0);
    test_eval_bool(e, "(not '())", 0);
    test_eval_bool(e, "(not (< 1 2))", 0);
    test_eval_bool(e, "(not (< 2 1))", 1);

    // not errors
    test_eval_error(e, "(not)", "expects 1 arg, but got 0");
}

void test_predicates(eval* e) {
    // number?
    test_eval_bool(e, "(number? 1)", 1);
    test_eval_bool(e, "(number? 0)", 1);
    test_eval_bool(e, "(number? 0.1)", 1);
    test_eval_bool(e, "(number? -0.1)", 1);
    test_eval_bool(e, "(number? 'x)", 0);
    test_eval_bool(e, "(number? \"a\")", 0);
    test_eval_bool(e, "(number? true)", 0);
    test_eval_bool(e, "(number? '(1 2 3))", 0);
    test_eval_bool(e, "(number? '())", 0);

    // number? errors
    test_eval_error(e, "(number?)", "expects 1 arg, but got 0");
    test_eval_error(e, "(number? 1 2)", "expects 1 arg, but got 2");

    // symbol?
    test_eval_bool(e, "(symbol? 1)", 0);
    test_eval_bool(e, "(symbol? 'x)", 1);
    test_eval_bool(e, "(symbol? 'car)", 1);
    test_eval_bool(e, "(symbol? \"a\")", 0);
    test_eval_bool(e, "(symbol? true)", 0);
    test_eval_bool(e, "(symbol? '(1 2 3))", 0);
    test_eval_bool(e, "(symbol? '())", 0);

    // symbol? errors
    test_eval_error(e, "(symbol?)", "expects 1 arg, but got 0");
    test_eval_error(e, "(symbol? 1 2)", "expects 1 arg, but got 2");

    // string?
    test_eval_bool(e, "(string? 1)", 0);
    test_eval_bool(e, "(string? 'x)", 0);
    test_eval_bool(e, "(string? \"\")", 1);
    test_eval_bool(e, "(string? \"abc\")", 1);
    test_eval_bool(e, "(string? true)", 0);
    test_eval_bool(e, "(string? '(1 2 3))", 0);
    test_eval_bool(e, "(string? '())", 0);

    // string? errors
    test_eval_error(e, "(string?)", "expects 1 arg, but got 0");
    test_eval_error(e, "(string? 1 2)", "expects 1 arg, but got 2");

    // bool?
    test_eval_bool(e, "(bool? 1)", 0);
    test_eval_bool(e, "(bool? 'x)", 0);
    test_eval_bool(e, "(bool? \"a\")", 0);
    test_eval_bool(e, "(bool? true)", 1);
    test_eval_bool(e, "(bool? false)", 1);
    test_eval_bool(e, "(bool? '(1 2 3))", 0);
    test_eval_bool(e, "(bool? '())", 0);

    // bool? errors
    test_eval_error(e, "(bool?)", "expects 1 arg, but got 0");
    test_eval_error(e, "(bool? 1 2)", "expects 1 arg, but got 2");

    // pair?
    test_eval_bool(e, "(pair? 1)", 0);
    test_eval_bool(e, "(pair? 'x)", 0);
    test_eval_bool(e, "(pair? \"a\")", 0);
    test_eval_bool(e, "(pair? true)", 0);
    test_eval_bool(e, "(pair? '(1))", 1);
    test_eval_bool(e, "(pair? '(1 2))", 1);
    test_eval_bool(e, "(pair? '(1 2 3))", 1);
    test_eval_bool(e, "(pair? '(1 . 2))", 1);
    test_eval_bool(e, "(pair? '(1 2 . 3))", 1);
    test_eval_bool(e, "(pair? '())", 0);

    // pair? errors
    test_eval_error(e, "(pair?)", "expects 1 arg, but got 0");
    test_eval_error(e, "(pair? 1 2)", "expects 1 arg, but got 2");

    // list?
    test_eval_bool(e, "(list? 1)", 0);
    test_eval_bool(e, "(list? 'x)", 0);
    test_eval_bool(e, "(list? \"a\")", 0);
    test_eval_bool(e, "(list? true)", 0);
    test_eval_bool(e, "(list? '(1))", 1);
    test_eval_bool(e, "(list? '(1 2))", 1);
    test_eval_bool(e, "(list? '(1 2 3))", 1);
    test_eval_bool(e, "(list? '(1 . 2))", 0);
    test_eval_bool(e, "(list? '(1 2 . 3))", 0);
    test_eval_bool(e, "(list? '())", 1);

    // list? errors
    test_eval_error(e, "(list?)", "expects 1 arg, but got 0");
    test_eval_error(e, "(list? 1 2)", "expects 1 arg, but got 2");

    // null?
    test_eval_bool(e, "(null? 1)", 0);
    test_eval_bool(e, "(null? 'x)", 0);
    test_eval_bool(e, "(null? \"a\")", 0);
    test_eval_bool(e, "(null? true)", 0);
    test_eval_bool(e, "(null? '(1))", 0);
    test_eval_bool(e, "(null? '(1 2))", 0);
    test_eval_bool(e, "(null? '(1 2 3))", 0);
    test_eval_bool(e, "(null? '(1 . 2))", 0);
    test_eval_bool(e, "(null? '(1 2 . 3))", 0);
    test_eval_bool(e, "(null? '())", 1);

    // null? errors
    test_eval_error(e, "(null?)", "expects 1 arg, but got 0");
    test_eval_error(e, "(null? 1 2)", "expects 1 arg, but got 2");

    // equal?
    test_eval_bool(e, "(equal? 0 0)", 1);
    test_eval_bool(e, "(equal? 1 1)", 1);
    test_eval_bool(e, "(equal? 1 1.0)", 1);
    test_eval_bool(e, "(equal? 0 1)", 0);
    test_eval_bool(e, "(equal? 'a 'a)", 1);
    test_eval_bool(e, "(equal? 'a 'b)", 0);
    test_eval_bool(e, "(equal? \"a\" \"a\")", 1);
    test_eval_bool(e, "(equal? \"a\" \"b\")", 0);
    test_eval_bool(e, "(equal? \"a\" \"\")", 0);
    test_eval_bool(e, "(equal? true true)", 1);
    test_eval_bool(e, "(equal? false false)", 1);
    test_eval_bool(e, "(equal? true false)", 0);
    test_eval_bool(e, "(equal? '(1) '(1))", 1);
    test_eval_bool(e, "(equal? '(1) '(2))", 0);
    test_eval_bool(e, "(equal? '(1 2 3) '(1 2 3))", 1);
    test_eval_bool(e, "(equal? '(1 2 3) '(1 2 4))", 0);
    test_eval_bool(e, "(equal? '(1 2 . 3) '(1 2 . 3))", 1);
    test_eval_bool(e, "(equal? '(1 2 . 3) '(4 2 . 3))", 0);
    test_eval_bool(e, "(equal? '(1 2 . 3) '(1 2 . 4))", 0);
    test_eval_bool(e, "(equal? '() '())", 1);
    test_eval_bool(e, "(equal? '() '(1))", 0);

    // equal? errors
    test_eval_error(e, "(equal?)", "expects 2 args, but got 0");
    test_eval_error(e, "(equal? 1)", "expects 2 args, but got 1");
    test_eval_error(e, "(equal? 1 2 3)", "expects 2 args, but got 3");

    // eq?
    test_eval_bool(e, "(eq? PI E)", 0);
    test_eval_bool(e, "(eq? PI PI)", 1);
    test_eval_bool(e, "(eq? car car)", 1);
    test_eval_bool(e, "(eq? car cdr)", 0);
    test_eval_bool(e, "(eq? cons cons)", 1);
    test_eval_bool(e, "(eq? (cons car cdr) (cons car cdr))", 0);
    test_eval_bool(e, "(eq? true true)", 1);
    test_eval_bool(e, "(eq? false false)", 1);
    test_eval_bool(e, "(eq? nil nil)", 1);
    test_eval_bool(e, "(eq? 'a 'a)", 0);
    test_eval_bool(e, "(eq? \"a\" \"a\")", 0);
    test_eval_bool(e, "(eq? '(1) '(1))", 0);
    test_eval_bool(e, "(eq? '(1 . 2) '(1 . 2))", 0);
    test_eval_bool(e, "(eq? '(1 2 3) '(1 2 3))", 0);
    test_eval_bool(e, "(eq? '() '())", 1);

    // eq? errors
    test_eval_error(e, "(eq?)", "expects 2 args, but got 0");
    test_eval_error(e, "(eq? 1)", "expects 2 args, but got 1");
    test_eval_error(e, "(eq? 1 2 3)", "expects 2 args, but got 3");
}

void test_other(eval* e) {
    // error
    test_eval_error(e, "(error \"\")", "");
    test_eval_error(e, "(error \"hello world\")", "hello world");
    test_eval_error(e, "(error \"hello '%s'\" 1)", "hello '1'");
    test_eval_error(e, "(error \"(%s, %s, %s, %s, %s)\" 'x \"a\" true '(1) '())", "(x, \"a\", true, (1), ())");

    // error errors
    test_eval_error(e, "(error)", "expects at least 1 arg, but got 0");
    test_eval_error(e, "(error 1)", "must be string, but is number");

    // info
    test_eval_info(e, "(info \"\")", "");
    test_eval_info(e, "(info \"hello world\")", "hello world");
    test_eval_info(e, "(info \"hello '%s'\" 1)", "hello '1'");
    test_eval_info(e, "(info \"(%s, %s, %s, %s, %s)\" 'x \"a\" true '(1) '())", "(x, \"a\", true, (1), ())");

    // info errors
    test_eval_error(e, "(info)", "expects at least 1 arg, but got 0");
    test_eval_error(e, "(info 1)", "must be string, but is number");
}

void run_test() {
    eval* e = eval_new(EVALUATOR_PATH);

    RUN_TEST_FN(test_parse);
    RUN_TEST_FN(test_to_str);

    RUN_TEST_FN(test_pool);
    RUN_TEST_FN(test_machine);

    RUN_EVAL_TEST_FN(e, test_syntax);
    RUN_EVAL_TEST_FN(e, test_structural);
    RUN_EVAL_TEST_FN(e, test_arithmetic);
    RUN_EVAL_TEST_FN(e, test_math);
    RUN_EVAL_TEST_FN(e, test_relational);
    RUN_EVAL_TEST_FN(e, test_predicates);
    RUN_EVAL_TEST_FN(e, test_other);

    printf("all tests have been passed!\n");

    eval_dispose(e);
}
