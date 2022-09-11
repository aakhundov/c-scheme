#include "prim.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

#include "comp.h"
#include "const.h"
#include "machine.h"
#include "map.h"
#include "syntax.h"
#include "value.h"

static map* primitive_map = NULL;

#define ASSERT_NUM_ARGS(p, args, expected_num_args)  \
    {                                                \
        size_t num_args = 0;                         \
        const value* arg = args;                     \
        while (arg != NULL) {                        \
            num_args++;                              \
            arg = arg->cdr;                          \
        }                                            \
        if (num_args != expected_num_args) {         \
            return pool_new_error(                   \
                p, "expects %d arg%s, but got %d",   \
                expected_num_args,                   \
                (expected_num_args == 1 ? "" : "s"), \
                num_args);                           \
        }                                            \
    }

#define ASSERT_MIN_NUM_ARGS(p, args, min_expected_num_args) \
    {                                                       \
        size_t num_args = 0;                                \
        const value* arg = args;                            \
        while (arg != NULL) {                               \
            num_args++;                                     \
            arg = arg->cdr;                                 \
        }                                                   \
        if (num_args < min_expected_num_args) {             \
            return pool_new_error(                          \
                p, "expects at least %d arg%s, but got %d", \
                min_expected_num_args,                      \
                (min_expected_num_args == 1 ? "" : "s"),    \
                num_args);                                  \
        }                                                   \
    }

#define ASSERT_MAX_NUM_ARGS(p, args, max_expected_num_args) \
    {                                                       \
        size_t num_args = 0;                                \
        const value* arg = args;                            \
        while (arg != NULL) {                               \
            num_args++;                                     \
            arg = arg->cdr;                                 \
        }                                                   \
        if (num_args > max_expected_num_args) {             \
            return pool_new_error(                          \
                p, "expects at most %d arg%s, but got %d",  \
                max_expected_num_args,                      \
                (max_expected_num_args == 1 ? "" : "s"),    \
                num_args);                                  \
        }                                                   \
    }

#define ASSERT_ARG_TYPE(p, args, ordinal, expected_type) \
    {                                                    \
        size_t i = 0;                                    \
        const value* arg = args;                         \
        while (arg != NULL && i < ordinal) {             \
            arg = arg->cdr;                              \
            i++;                                         \
        }                                                \
        if (i < ordinal || arg == NULL) {                \
            return pool_new_error(                       \
                p, "arg #%d of type %s is missing",      \
                ordinal, get_type_name(expected_type));  \
        } else if (arg->car == NULL) {                   \
            return pool_new_error(                       \
                p, "arg #%d must be %s, but got ()",     \
                ordinal, get_type_name(expected_type));  \
        } else if (arg->car->type != expected_type) {    \
            static char buffer[BUFFER_SIZE];             \
            value_to_str(arg->car, buffer);              \
            return pool_new_error(                       \
                p, "arg #%d must be %s, but is %s %s",   \
                ordinal,                                 \
                get_type_name(expected_type),            \
                get_type_name(arg->car->type),           \
                buffer);                                 \
        }                                                \
    }

#define ASSERT_ALL_ARGS_TYPE(p, args, offset, expected_type)   \
    {                                                          \
        size_t i = 0;                                          \
        const value* arg = args;                               \
        while (arg != NULL) {                                  \
            if (i >= offset) {                                 \
                if (arg->car == NULL) {                        \
                    return pool_new_error(                     \
                        p, "arg #%d must be %s, but got ()",   \
                        i, get_type_name(expected_type));      \
                } else if (arg->car->type != expected_type) {  \
                    static char buffer[BUFFER_SIZE];           \
                    value_to_str(arg->car, buffer);            \
                    return pool_new_error(                     \
                        p, "arg #%d must be %s, but is %s %s", \
                        i,                                     \
                        get_type_name(expected_type),          \
                        get_type_name(arg->car->type),         \
                        buffer);                               \
                }                                              \
            }                                                  \
            arg = arg->cdr;                                    \
            i++;                                               \
        }                                                      \
    }

#define GET_OPTIONAL_ARG(p, args, ordinal, expected_type, target) \
    {                                                             \
        size_t i = 0;                                             \
        const value* arg = args;                                  \
        while (arg != NULL && i < ordinal) {                      \
            arg = arg->cdr;                                       \
            i++;                                                  \
        }                                                         \
        if (arg != NULL) {                                        \
            if (arg->car == NULL) {                               \
                return pool_new_error(                            \
                    p, "arg #%d must be %s, but got ()",          \
                    ordinal, get_type_name(expected_type));       \
            } else if (arg->car->type != expected_type) {         \
                static char buffer[BUFFER_SIZE];                  \
                value_to_str(arg->car, buffer);                   \
                return pool_new_error(                            \
                    p, "arg #%d must be %s, but is %s %s",        \
                    ordinal,                                      \
                    get_type_name(expected_type),                 \
                    get_type_name(arg->car->type),                \
                    buffer);                                      \
            } else {                                              \
                target = arg->car;                                \
            }                                                     \
        }                                                         \
    }

static value* prim_car(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_PAIR);

    value* pair = args->car;

    return pair->car;
}

static value* prim_cdr(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_PAIR);

    value* pair = args->car;

    return pair->cdr;
}

static value* prim_cons(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 2);

    value* first = args->car;
    value* second = args->cdr->car;

    return pool_new_pair(m->pool, first, second);
}

static value* prim_list(machine* m, const value* args) {
    value* result = NULL;
    value* running = NULL;
    while (args != NULL) {
        value* pair = pool_new_pair(m->pool, args->car, NULL);
        if (running == NULL) {
            result = pair;
        } else {
            running->cdr = pair;
        }
        running = pair;
        args = args->cdr;
    }

    return result;
}

static value* prim_set_car(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 2);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_PAIR);

    value* first = args->car;
    value* second = args->cdr->car;

    first->car = second;

    return NULL;
}

static value* prim_set_cdr(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 2);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_PAIR);

    value* first = args->car;
    value* second = args->cdr->car;

    first->cdr = second;

    return NULL;
}

static value* prim_add(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        result += args->car->number;
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_sub(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    if (args == NULL) {
        result = -result;
    } else {
        while (args != NULL) {
            result -= args->car->number;
            args = args->cdr;
        }
    }

    return pool_new_number(m->pool, result);
}

static value* prim_mul(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        result *= args->car->number;
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_div(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (args->car->number == 0) {
            return pool_new_error(m->pool, "division by zero");
        }
        result /= args->car->number;
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_remainder(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (args->car->number == 0) {
            return pool_new_error(m->pool, "division by zero");
        }
        result = fmod(result, args->car->number);
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_expt(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        result = pow(result, args->car->number);
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_min(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (args->car->number < result) {
            result = args->car->number;
        }
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_max(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (args->car->number > result) {
            result = args->car->number;
        }
        args = args->cdr;
    }

    return pool_new_number(m->pool, result);
}

static value* prim_abs(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = fabs(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_exp(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = exp(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_log(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    if (args->car->number <= 0) {
        return pool_new_error(m->pool, "can't tage log of a non-positive number");
    }

    double result = log(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_sin(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = sin(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_cos(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = cos(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_tan(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = tan(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_atan(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = atan(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_atan2(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double y = args->car->number;
    double x = args->cdr->car->number;

    double result = atan2(y, x);

    return pool_new_number(m->pool, result);
}

static value* prim_round(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = round(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_floor(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = floor(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_ceiling(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double result = ceil(args->car->number);

    return pool_new_number(m->pool, result);
}

static value* prim_eq(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (value != args->car->number) {
            return pool_new_bool(m->pool, 0);
        }
        args = args->cdr;
    }

    return pool_new_bool(m->pool, 1);
}

static value* prim_lt(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (value >= args->car->number) {
            return pool_new_bool(m->pool, 0);
        }
        value = args->car->number;
        args = args->cdr;
    }

    return pool_new_bool(m->pool, 1);
}

static value* prim_lte(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (value > args->car->number) {
            return pool_new_bool(m->pool, 0);
        }
        value = args->car->number;
        args = args->cdr;
    }

    return pool_new_bool(m->pool, 1);
}

static value* prim_gt(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (value <= args->car->number) {
            return pool_new_bool(m->pool, 0);
        }
        value = args->car->number;
        args = args->cdr;
    }

    return pool_new_bool(m->pool, 1);
}

static value* prim_gte(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 2);
    ASSERT_ALL_ARGS_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;
    args = args->cdr;

    while (args != NULL) {
        if (value < args->car->number) {
            return pool_new_bool(m->pool, 0);
        }
        value = args->car->number;
        args = args->cdr;
    }

    return pool_new_bool(m->pool, 1);
}

static value* prim_not(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    if (is_true(m->pool, arg)) {
        return pool_new_bool(m->pool, 0);
    } else {
        return pool_new_bool(m->pool, 1);
    }
}

static value* prim_number_q(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, arg != NULL && arg->type == VALUE_NUMBER);
}

static value* prim_symbol_q(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, arg != NULL && arg->type == VALUE_SYMBOL);
}

static value* prim_string_q(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, arg != NULL && arg->type == VALUE_STRING);
}

static value* prim_bool_q(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, arg != NULL && arg->type == VALUE_BOOL);
}

static value* prim_pair_q(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, arg != NULL && arg->type == VALUE_PAIR);
}

static value* prim_list_q(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;
    while (arg != NULL) {
        if (arg->type != VALUE_PAIR) {
            return pool_new_bool(m->pool, 0);
        }
        arg = arg->cdr;
    }

    return pool_new_bool(m->pool, 1);
}

static value* prim_null_q(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, arg == NULL);
}

static value* prim_true_q(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, is_true(m->pool, arg));
}

static value* prim_false_q(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* arg = args->car;

    return pool_new_bool(m->pool, is_false(m->pool, arg));
}

static value* prim_equal_q(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 2);

    value* v1 = args->car;
    value* v2 = args->cdr->car;

    return pool_new_bool(m->pool, value_equal(v1, v2));
}

static value* prim_eq_q(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 2);

    value* v1 = args->car;
    value* v2 = args->cdr->car;

    return pool_new_bool(m->pool, v1 == v2);
}

static value* prim_even_q(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;

    if ((long)value != value) {
        return pool_new_bool(m->pool, 0);
    } else {
        return pool_new_bool(m->pool, (long)value % 2 == 0);
    }
}

static value* prim_odd_q(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_NUMBER);

    double value = args->car->number;

    if ((long)value != value) {
        return pool_new_bool(m->pool, 0);
    } else {
        return pool_new_bool(m->pool, (long)value % 2 != 0);
    }
}

static value* prim_error(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_STRING);

    value* error_message = args->car;
    value* error_args = args->cdr;

    static char buffer[BUFFER_SIZE];
    format_args(error_message, error_args, buffer);

    return pool_new_error(m->pool, buffer);
}

static value* prim_info(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_STRING);

    value* info_message = args->car;
    value* info_args = args->cdr;

    static char buffer[BUFFER_SIZE];
    format_args(info_message, info_args, buffer);

    return pool_new_info(m->pool, buffer);
}

static value* prim_display(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);

    while (args != NULL) {
        if (args->car->type == VALUE_SYMBOL || args->car->type == VALUE_STRING) {
            printf("%s", args->car->symbol);
        } else {
            static char buffer[BUFFER_SIZE];
            value_to_str(args->car, buffer);
            printf("%s", buffer);
        }

        if (args->cdr != NULL) {
            printf(" ");
        }

        args = args->cdr;
    }

    return NULL;
}

static value* prim_newline(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 0);

    printf("\n");

    return NULL;
}

static value* prim_collect(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 0);

    size_t values_before = m->pool->size;

    pool_collect_garbage(m->pool);

    size_t values_after = m->pool->size;
    size_t collected = values_before - values_after;
    double percentage = (values_before > 0 ? (double)collected / values_before : 0) * 100;

    return pool_new_info(
        m->pool, "%zu (%.2f%%) from %zu collected",
        collected, percentage, values_before);
}

static value* prim_srand(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_NUMBER);

    srand((int)args->car->number);

    return pool_new_info(m->pool, "RNG was seeded");
}

static value* prim_random(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);
    ASSERT_ARG_TYPE(m->pool, args, 0, VALUE_NUMBER);

    int upper = (int)args->car->number;
    int result = rand() % upper;

    return pool_new_number(m->pool, result);
}

static value* prim_time(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 0);

    struct timespec t;
    timespec_get(&t, TIME_UTC);
    double millitime = t.tv_sec + t.tv_nsec / 1000000000.0;

    return pool_new_number(m->pool, millitime);
}

static value* prim_pretty(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_MAX_NUM_ARGS(m->pool, args, 2);

    value* exp = args->car;
    value* line_len = pool_new_number(m->pool, 80);

    GET_OPTIONAL_ARG(m->pool, args, 1, VALUE_NUMBER, line_len);

    static char buffer[BUFFER_SIZE];
    value_to_pretty_str(exp, buffer, (int)line_len->number);

    return value_new_symbol(buffer);
}

static value* prim_code(machine* m, const value* args) {
    ASSERT_MIN_NUM_ARGS(m->pool, args, 1);
    ASSERT_MAX_NUM_ARGS(m->pool, args, 3);

    value* exp = args->car;
    value* target = pool_new_string(m->pool, "val");
    value* linkage = pool_new_string(m->pool, "return");

    GET_OPTIONAL_ARG(m->pool, args, 1, VALUE_STRING, target);
    GET_OPTIONAL_ARG(m->pool, args, 2, VALUE_STRING, linkage);

    return compile(m->pool, exp, target->symbol, linkage->symbol);
}

static value* prim_compile(machine* m, const value* args) {
    ASSERT_NUM_ARGS(m->pool, args, 1);

    value* exp = args->car;

    // compile the expression into machine code
    // target: place the result in the val register
    // lingage: (goto (reg continue)) in the end
    value* code = compile(m->pool, exp, "val", "return");

    if (code->type == VALUE_ERROR) {
        // error in compilation -> return
        return code;
    }

    // add the compiled code to the machine
    value* start = machine_append_code(m, code);
    // and set the position to the code's start
    machine_set_code_position(m, start);

    // the return value here doesn't matter
    // as the machine will continue from the head
    return NULL;
}

static void add_primitive(char* name, machine_op fn) {
    map_add(primitive_map, name, value_new_primitive(fn, name));
}

static void add_primitives_to_map() {
    // structural
    add_primitive("car", prim_car);
    add_primitive("cdr", prim_cdr);
    add_primitive("cons", prim_cons);
    add_primitive("list", prim_list);
    add_primitive("set-car!", prim_set_car);
    add_primitive("set-cdr!", prim_set_cdr);

    // arithmetic
    add_primitive("+", prim_add);
    add_primitive("-", prim_sub);
    add_primitive("*", prim_mul);
    add_primitive("/", prim_div);
    add_primitive("remainder", prim_remainder);
    add_primitive("expt", prim_expt);
    add_primitive("min", prim_min);
    add_primitive("max", prim_max);

    // math
    add_primitive("abs", prim_abs);
    add_primitive("exp", prim_exp);
    add_primitive("log", prim_log);
    add_primitive("sin", prim_sin);
    add_primitive("cos", prim_cos);
    add_primitive("tan", prim_tan);
    add_primitive("atan", prim_atan);
    add_primitive("atan2", prim_atan2);
    add_primitive("round", prim_round);
    add_primitive("floor", prim_floor);
    add_primitive("ceiling", prim_ceiling);

    // relational
    add_primitive("=", prim_eq);
    add_primitive("<", prim_lt);
    add_primitive("<=", prim_lte);
    add_primitive(">", prim_gt);
    add_primitive(">=", prim_gte);
    add_primitive("not", prim_not);

    // predicates
    add_primitive("number?", prim_number_q);
    add_primitive("symbol?", prim_symbol_q);
    add_primitive("string?", prim_string_q);
    add_primitive("bool?", prim_bool_q);
    add_primitive("pair?", prim_pair_q);
    add_primitive("list?", prim_list_q);
    add_primitive("null?", prim_null_q);
    add_primitive("true?", prim_true_q);
    add_primitive("false?", prim_false_q);
    add_primitive("equal?", prim_equal_q);
    add_primitive("eq?", prim_eq_q);
    add_primitive("even?", prim_even_q);
    add_primitive("odd?", prim_odd_q);

    // other
    add_primitive("error", prim_error);
    add_primitive("info", prim_info);
    add_primitive("display", prim_display);
    add_primitive("newline", prim_newline);
    add_primitive("collect", prim_collect);
    add_primitive("srand", prim_srand);
    add_primitive("random", prim_random);
    add_primitive("time", prim_time);
    add_primitive("pretty", prim_pretty);

    // compilation
    add_primitive("code", prim_code);
    add_primitive("compile", prim_compile);
}

void init_primitives() {
    if (primitive_map == NULL) {
        primitive_map = map_new();
        add_primitives_to_map();
    }
}

void cleanup_primitives() {
    if (primitive_map != NULL) {
        map_dispose_values(primitive_map);
        map_dispose(primitive_map);
    }
}

int is_primitive(const char* name) {
    return map_has(primitive_map, name);
}

value* get_primitive(const char* name) {
    map_record* r = map_get(primitive_map, name);

    if (r != NULL) {
        return r->val;
    }

    return NULL;
}
