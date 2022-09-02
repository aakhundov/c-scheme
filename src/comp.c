#include "comp.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "const.h"
#include "parse.h"
#include "pool.h"
#include "syntax.h"
#include "value.h"

static const char TOKEN_DELIMITER = ',';

static size_t label_counter = 0;

static value* list_copy(pool* p, value* lst) {
    value* result = NULL;
    value* tail = NULL;

    while (lst != NULL) {
        value* next = lst->car;
        lst = lst->cdr;

        if (tail == NULL) {
            result = pool_new_pair(p, next, NULL);
            tail = result;
        } else {
            tail->cdr = pool_new_pair(p, next, NULL);
            tail = tail->cdr;
        }
    }

    return result;
}

static value* list_append(pool* p, value* lst1, value* lst2) {
    value* result = NULL;
    value* tail = NULL;

    while (lst1 != NULL || lst2 != NULL) {
        value* next = NULL;
        if (lst1 != NULL) {
            next = lst1->car;
            lst1 = lst1->cdr;
        } else {
            next = lst2->car;
            lst2 = lst2->cdr;
        }

        if (tail == NULL) {
            result = pool_new_pair(p, next, NULL);
            tail = result;
        } else {
            tail->cdr = pool_new_pair(p, next, NULL);
            tail = tail->cdr;
        }
    }

    return result;
}

static value* list_union(pool* p, value* lst1, value* lst2) {
    value* result = NULL;
    value* tail = NULL;

    while (lst1 != NULL || lst2 != NULL) {
        value* next = NULL;
        if (lst1 != NULL && lst2 != NULL) {
            int cmp = strcmp(lst1->car->symbol, lst2->car->symbol);
            if (cmp < 0) {
                next = lst1->car;
                lst1 = lst1->cdr;
            } else if (cmp > 0) {
                next = lst2->car;
                lst2 = lst2->cdr;
            } else {
                next = lst1->car;
                lst1 = lst1->cdr;
                lst2 = lst2->cdr;
            }
        } else if (lst1 != NULL) {
            next = lst1->car;
            lst1 = lst1->cdr;
        } else {
            next = lst2->car;
            lst2 = lst2->cdr;
        }

        if (tail == NULL) {
            result = pool_new_pair(p, next, NULL);
            tail = result;
        } else {
            tail->cdr = pool_new_pair(p, next, NULL);
            tail = tail->cdr;
        }
    }

    return result;
}

static value* list_difference(pool* p, value* lst1, value* lst2) {
    value* result = NULL;
    value* tail = NULL;

    while (lst1 != NULL || lst2 != NULL) {
        value* next = NULL;
        if (lst1 != NULL && lst2 != NULL) {
            int cmp = strcmp(lst1->car->symbol, lst2->car->symbol);
            if (cmp < 0) {
                next = lst1->car;
                lst1 = lst1->cdr;
            } else if (cmp > 0) {
                lst2 = lst2->cdr;
                continue;
            } else {
                lst1 = lst1->cdr;
                lst2 = lst2->cdr;
                continue;
            }
        } else if (lst1 != NULL) {
            next = lst1->car;
            lst1 = lst1->cdr;
        } else {
            break;
        }

        if (tail == NULL) {
            result = pool_new_pair(p, next, NULL);
            tail = result;
        } else {
            tail->cdr = pool_new_pair(p, next, NULL);
            tail = tail->cdr;
        }
    }

    return result;
}

static int list_contains(value* list, char* name) {
    value* running = list;
    while (running != NULL) {
        if (strcmp(running->car->symbol, name) == 0) {
            return 1;
        }
        running = running->cdr;
    }

    return 0;
}

static void add_to_list(pool* p, value* list, char* name) {
    value* prev = list;
    while (prev->cdr != NULL) {
        int cmp = strcmp(prev->cdr->car->symbol, name);
        if (cmp == 0) {
            return;
        } else if (cmp > 0) {
            break;
        } else {
            prev = prev->cdr;
        }
    }

    value* new_symbol = pool_new_symbol(p, name);
    prev->cdr = pool_new_pair(p, new_symbol, prev->cdr);
}

static void split_tokens(char* tokens, char delim, char* first, char* rest) {
    char* running = tokens;
    while (*running != '\0') {
        if (*running == delim) {
            break;
        }
        running++;
    }

    if (*running != '\0') {
        strncpy(first, tokens, running - tokens);  // the one before delim
        strcpy(rest, running + 1);                 // all after delim
        first[running - tokens] = '\0';            // terminate first
    } else {
        strcpy(first, tokens);  // the only one
        rest[0] = '\0';         // nothing
    }
}

static value* make_empty_sequence(pool* p) {
    return pool_new_pair(
        p,
        pool_new_pair(
            p,
            pool_new_pair(p, NULL, NULL),   // "needed" list
            pool_new_pair(p, NULL, NULL)),  // "modified" list
        NULL);                              // code lines
}

static value* make_sequence(pool* p, value* needed, value* modified, value* code) {
    return pool_new_pair(
        p,
        pool_new_pair(
            p,
            pool_new_pair(p, NULL, needed),     // "needed" list
            pool_new_pair(p, NULL, modified)),  // "modified" list
        code);                                  // code lines
}

static value* get_needed(value* seq) {
    return seq->car->car->cdr;
}

static value* get_modified(value* seq) {
    return seq->car->cdr->cdr;
}

static value* get_code(value* seq) {
    return seq->cdr;
}

static int needs(value* seq, char* reg) {
    return list_contains(get_needed(seq), reg);
}

static int modifies(value* seq, char* reg) {
    return list_contains(get_modified(seq), reg);
}

static value* make_code_from_args(pool* p, char* line, va_list args) {
    static char buffer[BUFFER_SIZE];
    vsnprintf(buffer, sizeof(buffer), line, args);

    value* source = parse_from_str(buffer);
    value* result = pool_import(p, source);
    value_dispose(source);

    return result;
}

static value* make_code(pool* p, char* line, ...) {
    va_list args;
    va_start(args, line);
    value* result = make_code_from_args(p, line, args);
    va_end(args);

    return result;
}

static void add_needed(pool* p, value* seq, char* reg) {
    add_to_list(p, seq->car->car, reg);
}

static void add_modified(pool* p, value* seq, char* reg) {
    add_to_list(p, seq->car->cdr, reg);
}

static void add_code(pool* p, value* seq, char* line, ...) {
    va_list args;
    va_start(args, line);
    value* code = make_code_from_args(p, line, args);
    va_end(args);

    if (code->cdr == NULL && code->car->type == VALUE_SYMBOL) {
        code = code->car;  // label
    }

    value* running = seq;
    while (running->cdr != NULL) {
        running = running->cdr;
    }

    // append the line to the end of the code
    running->cdr = pool_new_pair(p, code, NULL);
}

static value* make_label(pool* p, char* name, int increment_counter) {
    if (increment_counter) {
        label_counter++;
    }

    static char buffer[BUFFER_SIZE];
    sprintf(buffer, "%s-%zu", name, label_counter);

    return pool_new_symbol(p, buffer);
}

static value* make_label_sequence(pool* p, value* label) {
    value* result = make_empty_sequence(p);

    add_code(p, result, label->symbol);

    return result;
}

static value* wrap_in_save_restore(pool* p, value* code, char* reg) {
    value* save = make_code(p, "save %s", reg);
    value* restore = make_code(p, "restore %s", reg);

    value* result = pool_new_pair(p, save, NULL);
    value* tail = result;

    while (code != NULL) {
        tail->cdr = pool_new_pair(p, code->car, NULL);
        tail = tail->cdr;
        code = code->cdr;
    }

    tail->cdr = pool_new_pair(p, restore, NULL);

    return result;
}

static value* append_sequences(pool* p, value* seq1, value* seq2) {
    return make_sequence(
        p,
        // needed = needed by #1 + (needed by #1 - modified by #1)
        list_union(
            p,
            get_needed(seq1),
            list_difference(
                p,
                get_needed(seq2),
                get_modified(seq1))),
        // modified = modified by #1 + modified by #2
        list_union(
            p,
            get_modified(seq1),
            get_modified(seq2)),
        // code = code of #1 + code of #2
        list_append(
            p,
            get_code(seq1),
            get_code(seq2)));
}

static value* preserving(pool* p, char* regs, value* seq1, value* seq2) {
    if (strlen(regs) == 0) {
        // just append the sequences normally
        return append_sequences(p, seq1, seq2);
    } else {
        char first[128];
        char rest[128];

        split_tokens(regs, TOKEN_DELIMITER, first, rest);

        if (modifies(seq1, first) && needs(seq2, first)) {
            value* singleton = pool_new_pair(
                p,
                pool_new_symbol(p, first),
                NULL);

            value* seq1_saving = make_sequence(
                p,
                // needed = needed by #1 + first register
                list_union(
                    p,
                    get_needed(seq1),
                    singleton),
                // needed = modified by #1 + first register
                list_difference(
                    p,
                    get_modified(seq1),
                    singleton),
                // code = save first + seq1 + restore first
                wrap_in_save_restore(
                    p,
                    get_code(seq1),
                    first));

            // consider the rest of registers
            return preserving(p, rest, seq1_saving, seq2);
        } else {
            // consider the rest of registers
            return preserving(p, rest, seq1, seq2);
        }
    }
}

static value* tack_on_sequence(pool* p, value* seq1, value* seq2) {
    return make_sequence(
        p,
        // needed = needed by #1
        list_copy(p, get_needed(seq1)),
        // modified = modified by #1
        list_copy(p, get_modified(seq1)),
        // code = code of #1 + code of #2
        list_append(
            p,
            get_code(seq1),
            get_code(seq2)));
}

static value* parallel_sequences(pool* p, value* seq1, value* seq2) {
    return make_sequence(
        p,
        // needed = needed by #1 + needed by #2
        list_union(
            p,
            get_needed(seq1),
            get_needed(seq2)),
        // modified = modified by #1 + modified by #2
        list_union(
            p,
            get_modified(seq1),
            get_modified(seq2)),
        // code = code of #1 + code of #2
        list_append(
            p,
            get_code(seq1),
            get_code(seq2)));
}

static value* compile_linkage(pool* p, char* linkage) {
    value* result = make_empty_sequence(p);

    if (strcmp(linkage, "return") == 0) {
        add_needed(p, result, "continue");
        add_code(p, result, "goto (reg continue)");
    } else if (strcmp(linkage, "next") != 0) {
        add_code(p, result, "goto (label %s)", linkage);
    }  // empty sequence for the "next" linkage

    return result;
}

static value* end_with_linkage(pool* p, char* linkage, value* seq) {
    return preserving(
        p, "continue",
        seq,
        compile_linkage(p, linkage));
}

static value* compile_rec(pool* p, value* exp, char* target, char* linkage);

static value* compile_self_evaluating(pool* p, value* exp, char* target, char* linkage) {
    static char self[BUFFER_SIZE];
    // the exp in string form
    value_to_str(exp, self);

    value* seq = make_empty_sequence(p);

    // return the exp
    add_modified(p, seq, target);
    add_code(p, seq, "assign %s (const %s)", target, self);

    return end_with_linkage(p, linkage, seq);
}

static value* compile_quoted(pool* p, value* exp, char* target, char* linkage) {
    static char quoted[BUFFER_SIZE];
    // the quoted exp in string form
    value_to_str(get_text_of_quotation(p, exp), quoted);

    value* seq = make_empty_sequence(p);

    // return the quoted exp
    add_modified(p, seq, target);
    add_code(p, seq, "assign %s (const %s)", target, quoted);

    return end_with_linkage(p, linkage, seq);
}

static value* compile_variable(pool* p, value* exp, char* target, char* linkage) {
    // the variable name
    char* name = exp->symbol;

    value* seq = make_empty_sequence(p);

    // lookup the variable
    // and return its value
    add_needed(p, seq, "env");
    add_modified(p, seq, target);
    add_code(
        p, seq,
        "assign %s (op lookup-variable-value) (const %s) (reg env)",
        target, name);

    return end_with_linkage(p, linkage, seq);
}

static value* compile_assignment(pool* p, value* exp, char* target, char* linkage) {
    // evaluate the value of the assignment
    value* value_seq = compile_rec(p, get_assignment_value(p, exp), "val", "next");

    // variable name to define (must be a symbol)
    char* name = get_assignment_variable(p, exp)->symbol;

    value* assign_seq = make_empty_sequence(p);

    // assignment sequence
    add_needed(p, assign_seq, "env");
    add_needed(p, assign_seq, "val");
    add_modified(p, assign_seq, target);
    add_code(
        p, assign_seq,
        "assign %s (op set-variable-value!) (const %s) (reg val) (reg env)",
        target, name);

    return end_with_linkage(
        p, linkage,
        preserving(
            p, "env",
            value_seq,     // value evaluation
            assign_seq));  // assignment
}

static value* compile_definition(pool* p, value* exp, char* target, char* linkage) {
    // evaluate the value of the definition
    value* value_seq = compile_rec(p, get_definition_value(p, exp), "val", "next");

    // variable name to define (must be a symbol)
    char* name = get_definition_variable(p, exp)->symbol;

    value* define_seq = make_empty_sequence(p);

    // definition sequence
    add_needed(p, define_seq, "env");
    add_needed(p, define_seq, "val");
    add_modified(p, define_seq, target);
    add_code(
        p, define_seq,
        "assign %s (op define-variable!) (const %s) (reg val) (reg env)",
        target, name);

    return end_with_linkage(
        p, linkage,
        preserving(
            p, "env",
            value_seq,     // value evaluation
            define_seq));  // definition
}

static value* compile_if(pool* p, value* exp, char* target, char* linkage) {
    // labels for different sections of the if
    value* true_branch = make_label(p, "true-branch", 1);
    value* false_branch = make_label(p, "false-branch", 0);
    value* after_if = make_label(p, "after-if", 0);

    // to circumvent the alternative if linkage is next
    char* cons_linkage = (strcmp(linkage, "next") == 0 ? after_if->symbol : linkage);

    // compiled predicate, consequent, and alternative of the if
    value* pred_seq = compile_rec(p, get_if_predicate(p, exp), "val", "next");
    value* cons_seq = compile_rec(p, get_if_consequent(p, exp), target, cons_linkage);
    value* alt_seq = compile_rec(p, get_if_alternative(p, exp), target, linkage);

    value* test_seq = make_empty_sequence(p);

    // test sequence
    add_needed(p, test_seq, "val");
    add_code(
        p, test_seq,
        "branch (label %s) (op false?) (reg val)",
        false_branch->symbol);

    return preserving(
        p, "env,continue",
        pred_seq,  // predicate
        append_sequences(
            p,
            append_sequences(
                p,
                test_seq,  // test
                parallel_sequences(
                    p,
                    append_sequences(
                        p,
                        make_label_sequence(p, true_branch),  // true label
                        cons_seq),                            // consequent
                    append_sequences(
                        p,
                        make_label_sequence(p, false_branch),  // false label
                        alt_seq))),                            // alternative
            make_label_sequence(p, after_if)));                // after label
}

static value* compile_sequence(pool* p, value* exp, char* target, char* linkage) {
    if (is_last_exp(p, exp)) {
        // compile the last expression with target and linkage
        return compile_rec(p, get_first_exp(p, exp), target, linkage);
    } else {
        return preserving(
            p, "env,continue",
            compile_rec(p, get_first_exp(p, exp), target, "next"),         // compile with next
            compile_sequence(p, get_rest_exps(p, exp), target, linkage));  // compile the rest
    }
}

static value* compile_lambda_body(pool* p, value* exp, value* proc_entry) {
    char params[1024];
    // the params in string form to include in the code
    value_to_str(get_lambda_parameters(p, exp), params);

    value* pre_body_seq = make_empty_sequence(p);

    // proc entry + extend the env by bounding
    // the lambda params to the arglist args
    add_needed(p, pre_body_seq, "env");
    add_needed(p, pre_body_seq, "proc");
    add_needed(p, pre_body_seq, "argl");
    add_modified(p, pre_body_seq, "env");
    add_code(p, pre_body_seq, "%s", proc_entry->symbol);
    add_code(p, pre_body_seq, "assign env (op compiled-environment) (reg proc)");
    add_code(
        p, pre_body_seq,
        "assign env (op extend-environment) (const %s) (reg argl) (reg env)",
        params);

    return append_sequences(
        p,
        pre_body_seq,                                                    // before the body
        compile_sequence(p, get_lambda_body(p, exp), "val", "return"));  // the body
}

static value* compile_lambda(pool* p, value* exp, char* target, char* linkage) {
    // labels to separate the body from the rest
    value* proc_entry = make_label(p, "proc-entry", 1);      // before the body
    value* after_lambda = make_label(p, "after-lambda", 0);  // after the body

    // to circumvent the body if linkage is next
    char* lambda_linkage = (strcmp(linkage, "next") == 0 ? after_lambda->symbol : linkage);

    value* assign_seq = make_empty_sequence(p);

    // make the compiled procedure
    // form the env and the proc entry
    add_needed(p, assign_seq, "env");
    add_modified(p, assign_seq, target);
    add_code(
        p, assign_seq,
        "assign %s (op make-compiled-procedure) (label %s) (reg env)",
        target, proc_entry->symbol);

    return append_sequences(
        p,
        tack_on_sequence(  // ignore the body's needs and modifies
            p,
            end_with_linkage(p, lambda_linkage, assign_seq),  // assign the compiled lambda
            compile_lambda_body(p, exp, proc_entry)),         // proc entry, extend the env, body
        make_label_sequence(p, after_lambda));                // after label
}

static value* compile_and(pool* p, value* exp, char* target, char* linkage) {
    value* exps = get_and_expressions(p, exp);
    if (has_no_exps(p, exps)) {
        // no exps: return true
        return compile_self_evaluating(p, pool_new_bool(p, 1), target, linkage);
    } else if (is_last_exp(p, exps)) {
        // single exp: compile as a singleton exp
        return compile_rec(p, get_first_exp(p, exps), target, linkage);
    } else {
        // collect the and exps in the reversed order
        value* rev_exp_seqs = NULL;
        while (!has_no_exps(p, exps)) {
            value* one_exp = get_first_exp(p, exps);
            value* exp_seq = compile_rec(p, one_exp, "val", "next");
            rev_exp_seqs = pool_new_pair(p, exp_seq, rev_exp_seqs);
            exps = get_rest_exps(p, exps);
        }

        // a new label to jump to the end of the and
        value* after_and = make_label(p, "after-and", 1);

        value* jump_seq = make_empty_sequence(p);

        // immediately jump to the end of the and
        // if preceeding expression evaluates to false
        add_needed(p, jump_seq, "val");
        add_code(
            p, jump_seq,
            "branch (label %s) (op false?) (reg val)",
            after_and->symbol);

        // the last expression evaluation:
        // without a conditional jump
        value* eval_seq = rev_exp_seqs->car;
        rev_exp_seqs = rev_exp_seqs->cdr;

        while (rev_exp_seqs != NULL) {
            // the next expression evaluation:
            // with a conditional jump
            eval_seq = preserving(
                p, "env",
                append_sequences(
                    p,
                    rev_exp_seqs->car,
                    jump_seq),
                eval_seq);
            rev_exp_seqs = rev_exp_seqs->cdr;
        }

        value* final_seq = make_empty_sequence(p);

        // final sequence with the label
        add_code(p, final_seq, after_and->symbol);

        if (strcmp(target, "val") != 0) {
            // return into the target if required
            add_needed(p, final_seq, "val");
            add_modified(p, final_seq, target);
            add_code(p, final_seq, "assign %s (reg val)", target);
        }

        return end_with_linkage(
            p, linkage,
            append_sequences(
                p,
                eval_seq,     // chain of evaluations
                final_seq));  // the label + return if required
    }
}

static value* compile_or(pool* p, value* exp, char* target, char* linkage) {
    value* exps = get_or_expressions(p, exp);
    if (has_no_exps(p, exps)) {
        // no exps: return false
        return compile_self_evaluating(p, pool_new_bool(p, 0), target, linkage);
    } else if (is_last_exp(p, exps)) {
        // single exp: compile as a singleton exp
        return compile_rec(p, get_first_exp(p, exps), target, linkage);
    } else {
        // collect the or exps in the reversed order
        value* rev_exp_seqs = NULL;
        while (!has_no_exps(p, exps)) {
            value* one_exp = get_first_exp(p, exps);
            value* exp_seq = compile_rec(p, one_exp, "val", "next");
            rev_exp_seqs = pool_new_pair(p, exp_seq, rev_exp_seqs);
            exps = get_rest_exps(p, exps);
        }

        // a new label to jump to the end of the or
        value* after_or = make_label(p, "after-or", 1);

        value* jump_seq = make_empty_sequence(p);

        // immediately jump to the end of the or
        // if preceeding expression evaluates to true
        add_needed(p, jump_seq, "val");
        add_code(
            p, jump_seq,
            "branch (label %s) (op true?) (reg val)",
            after_or->symbol);

        // the last expression evaluation:
        // without a conditional jump
        value* eval_seq = rev_exp_seqs->car;
        rev_exp_seqs = rev_exp_seqs->cdr;

        while (rev_exp_seqs != NULL) {
            // the next expression evaluation:
            // with a conditional jump
            eval_seq = preserving(
                p, "env",
                append_sequences(
                    p,
                    rev_exp_seqs->car,
                    jump_seq),
                eval_seq);
            rev_exp_seqs = rev_exp_seqs->cdr;
        }

        value* final_seq = make_empty_sequence(p);

        // final sequence with the label
        add_code(p, final_seq, after_or->symbol);

        if (strcmp(target, "val") != 0) {
            // return into the target if required
            add_needed(p, final_seq, "val");
            add_modified(p, final_seq, target);
            add_code(p, final_seq, "assign %s (reg val)", target);
        }

        return end_with_linkage(
            p, linkage,
            append_sequences(
                p,
                eval_seq,     // chain of evaluations
                final_seq));  // the label + return if required
    }
}

static value* compile_eval(pool* p, value* exp, char* target, char* linkage) {
    exp = get_eval_expression(p, exp);
    if (is_self_evaluating(exp)) {
        return compile_self_evaluating(p, exp, target, linkage);
    } else if (is_quoted(exp)) {
        return compile_quoted(p, exp, target, linkage);
    } else {
        // can't be target != "val" and linkage == "return" simultaneously
        assert(strcmp(target, "val") == 0 || strcmp(linkage, "return") != 0);

        // the first (internal / comiled) evaluation sequence
        value* internal_seq = compile_rec(p, exp, "exp", "next");

        // the second (external / interpreted) evaluaton sequence
        value* external_seq = make_empty_sequence(p);

        // evaluation will need the env
        add_needed(p, external_seq, "env");

        // anyting can happen during evaluation
        add_modified(p, external_seq, "env");
        add_modified(p, external_seq, "proc");
        add_modified(p, external_seq, "val");
        add_modified(p, external_seq, "argl");
        add_modified(p, external_seq, "continue");

        if (strcmp(linkage, "return") == 0) {
            // need the continue for
            // the evaluation to return to
            add_needed(p, external_seq, "continue");

            // just goto eval-dispatch which will then
            // set to the val and return to the continue
            add_code(p, external_seq, "goto (label eval-dispatch)");
        } else if (strcmp(linkage, "next") == 0) {
            // a new label to return to after the evaluation
            value* after_eval = make_label(p, "after-eval", 1);

            // assign new label to the continue and goto eval-dispatch
            add_code(p, external_seq, "assign continue (label %s)", after_eval->symbol);
            add_code(p, external_seq, "goto (label eval-dispatch)");
            add_code(p, external_seq, "%s", after_eval->symbol);

            if (strcmp(target, "val") != 0) {
                // assign val to the target if required
                add_code(p, external_seq, "assign %s (reg val)", target);
            }
        } else {
            if (strcmp(target, "val") == 0) {
                // set the continue to the linkage and goto eval-dispatch
                add_code(p, external_seq, "assign continue (label %s)", linkage);
                add_code(p, external_seq, "goto (label eval-dispatch)");
            } else {
                // a new label to return to after the evaluation
                value* after_eval = make_label(p, "after-eval", 1);

                // set the continue to the proc-return label and goto the proc
                // on return, set the val to the target and goto the linkage
                add_code(p, external_seq, "assign continue (label %s)", after_eval->symbol);
                add_code(p, external_seq, "goto (label eval-dispatch)");
                add_code(p, external_seq, "%s", after_eval->symbol);
                add_code(p, external_seq, "assign %s (reg val)", target);
                add_code(p, external_seq, "goto (label %s)", linkage);
            }
        }

        return preserving(
            p, "env",
            internal_seq,
            external_seq);
    }
}

static value* compile_rest_args(pool* p, value* rev_operand_seqs) {
    value* after_next_seq = make_empty_sequence(p);

    // add next arg to the the arglist
    add_needed(p, after_next_seq, "val");
    add_needed(p, after_next_seq, "argl");
    add_modified(p, after_next_seq, "argl");
    add_code(p, after_next_seq, "assign argl (op cons) (reg val) (reg argl)");

    // do the next arg and prepend it to arglist
    value* next_arg_seq = preserving(
        p, "argl",  // keep the argl
        rev_operand_seqs->car,
        after_next_seq);

    if (rev_operand_seqs->cdr == NULL) {
        // the next is the first arg
        return next_arg_seq;
    } else {
        return preserving(
            p, "env",                                      // keep the env
            next_arg_seq,                                  // do the next arg
            compile_rest_args(p, rev_operand_seqs->cdr));  // then do the rest
    }
}

static value* compile_arglist(pool* p, value* rev_operand_seqs) {
    if (rev_operand_seqs == NULL) {
        value* no_arg_seq = make_empty_sequence(p);

        // empty arglist
        add_modified(p, no_arg_seq, "argl");
        add_code(p, no_arg_seq, "assign argl (const ())");

        return no_arg_seq;
    } else {
        value* after_last_seq = make_empty_sequence(p);

        // the arglist with the last arg
        add_needed(p, after_last_seq, "val");
        add_modified(p, after_last_seq, "argl");
        add_code(p, after_last_seq, "assign argl (op cons) (reg val) (const ())");

        // do the last arg and make arglist from it
        value* last_arg_seq = append_sequences(
            p,
            rev_operand_seqs->car,
            after_last_seq);

        if (rev_operand_seqs->cdr == NULL) {
            // the last arg is the only arg
            return last_arg_seq;
        } else {
            return preserving(
                p, "env",                                      // keep the env
                last_arg_seq,                                  // do the first arg
                compile_rest_args(p, rev_operand_seqs->cdr));  // then do the rest
        }
    }
}

static value* compile_compiled_call(pool* p, char* target, char* linkage) {
    // can't be target != "val" and linkage == "return" simultaneously
    assert(strcmp(target, "val") == 0 || strcmp(linkage, "return") != 0);

    value* call_seq = make_empty_sequence(p);

    // need the proc to invoke
    add_needed(p, call_seq, "proc");

    // anyting can happen in the call
    add_modified(p, call_seq, "env");
    add_modified(p, call_seq, "proc");
    add_modified(p, call_seq, "val");
    add_modified(p, call_seq, "argl");
    add_modified(p, call_seq, "continue");

    if (strcmp(linkage, "return") == 0) {
        // need the continue for the call to return to
        add_needed(p, call_seq, "continue");

        // just goto the proc which will then return to the continue
        add_code(p, call_seq, "assign val (op compiled-entry) (reg proc)");
        add_code(p, call_seq, "goto (reg val)");
    } else {
        if (strcmp(target, "val") == 0) {
            // set the continue to the linkage and goto the proc
            add_code(p, call_seq, "assign continue (label %s)", linkage);
            add_code(p, call_seq, "assign val (op compiled-entry) (reg proc)");
            add_code(p, call_seq, "goto (reg val)");
        } else {
            // a new label to return to after the call
            value* proc_return = make_label(p, "proc-return", 1);

            // set the continue to the proc-return label and goto the proc
            // on return, set the val to the target and goto the linkage
            add_code(p, call_seq, "assign continue (label %s)", proc_return->symbol);
            add_code(p, call_seq, "assign val (op compiled-entry) (reg proc)");
            add_code(p, call_seq, "goto (reg val)");
            add_code(p, call_seq, "%s", proc_return->symbol);
            add_code(p, call_seq, "assign %s (reg val)", target);
            add_code(p, call_seq, "goto (label %s)", linkage);
        }
    }

    return call_seq;
}

static value* compile_procedure_call(pool* p, char* target, char* linkage) {
    // labels for the procedure type selection
    value* primitive_branch = make_label(p, "primitive-branch", 1);
    value* compiled_branch = make_label(p, "compiled-branch", 0);
    value* after_call = make_label(p, "after-call", 0);

    // to circumvent the primitive part after the compiled if linkage is next
    char* compiled_linkage = (strcmp(linkage, "next") == 0 ? after_call->symbol : linkage);

    value* test_seq = make_empty_sequence(p);

    // test for the proc type
    add_needed(p, test_seq, "proc");
    add_code(
        p, test_seq,
        "branch (label %s) (op primitive-procedure?) (reg proc)",
        primitive_branch->symbol);
    add_code(
        p, test_seq,
        "branch (label %s) (op compiled-procedure?) (reg proc)",
        compiled_branch->symbol);
    add_code(
        p, test_seq,
        "perform (op signal-error) (const \"can't apply %%s\") (reg proc)");

    value* primitive_seq = make_empty_sequence(p);

    // call the primitive proc
    add_needed(p, primitive_seq, "proc");
    add_needed(p, primitive_seq, "argl");
    add_modified(p, primitive_seq, target);
    add_code(
        p, primitive_seq,
        "assign %s (op apply-primitive-procedure) (reg proc) (reg argl)",
        target);

    return append_sequences(
        p,
        append_sequences(
            p,
            test_seq,  // test for the proc type
            parallel_sequences(
                p,
                append_sequences(
                    p,
                    make_label_sequence(p, compiled_branch),              // compiled label
                    compile_compiled_call(p, target, compiled_linkage)),  // call the compiled
                append_sequences(
                    p,
                    make_label_sequence(p, primitive_branch),        // primitive label
                    end_with_linkage(p, linkage, primitive_seq)))),  // call the primitive
        make_label_sequence(p, after_call));                         // after call label
}

static value* compile_apply(pool* p, value* exp, char* target, char* linkage) {
    // put the operator into proc with the next linkage
    value* operator_seq = compile_rec(p, get_apply_operator(p, exp), "proc", "next");

    // put the arguments into argl with the next linkage
    value* arguments_seq = compile_rec(p, get_apply_arguments(p, exp), "argl", "next");

    return preserving(
        p, "env,continue",
        operator_seq,  // first get the operator into proc
        preserving(
            p, "proc,continue",
            arguments_seq,                                 // then get the operands into argl
            compile_procedure_call(p, target, linkage)));  // then call the proc: primitive or compiled
}

static value* compile_application(pool* p, value* exp, char* target, char* linkage) {
    // put the operator into proc with the next linkage
    value* operator_seq = compile_rec(p, get_operator(p, exp), "proc", "next");

    // collect the code snippets to put each operand
    // into val with next linkage, in the reversed order
    value* rev_operand_seqs = NULL;
    value* operands = get_operands(p, exp);
    while (!has_no_operands(p, operands)) {
        value* operand = get_first_operand(p, operands);
        value* operand_seq = compile_rec(p, operand, "val", "next");
        rev_operand_seqs = pool_new_pair(p, operand_seq, rev_operand_seqs);
        operands = get_rest_operands(p, operands);
    }

    return preserving(
        p, "env,continue",
        operator_seq,  // first get the operator into proc
        preserving(
            p, "proc,continue",
            compile_arglist(p, rev_operand_seqs),          // then get the operands into argl
            compile_procedure_call(p, target, linkage)));  // then call the proc: primitive or compiled
}

static value* compile_rec(pool* p, value* exp, char* target, char* linkage) {
    if (is_self_evaluating(exp)) {
        return compile_self_evaluating(p, exp, target, linkage);
    } else if (is_variable(exp)) {
        return compile_variable(p, exp, target, linkage);
    } else if (is_quoted(exp)) {
        return compile_quoted(p, exp, target, linkage);
    } else if (is_assignment(exp)) {
        return compile_assignment(p, exp, target, linkage);
    } else if (is_definition(exp)) {
        return compile_definition(p, exp, target, linkage);
    } else if (is_if(exp)) {
        return compile_if(p, exp, target, linkage);
    } else if (is_lambda(exp)) {
        return compile_lambda(p, exp, target, linkage);
    } else if (is_let(exp)) {
        return compile_rec(p, transform_let(p, exp), target, linkage);
    } else if (is_begin(exp)) {
        return compile_sequence(p, get_begin_actions(p, exp), target, linkage);
    } else if (is_cond(exp)) {
        return compile_rec(p, transform_cond(p, exp), target, linkage);
    } else if (is_and(exp)) {
        return compile_and(p, exp, target, linkage);
    } else if (is_or(exp)) {
        return compile_or(p, exp, target, linkage);
    } else if (is_eval(exp)) {
        return compile_eval(p, exp, target, linkage);
    } else if (is_apply(exp)) {
        return compile_apply(p, exp, target, linkage);
    } else {
        // default: application
        return compile_application(p, exp, target, linkage);
    }
}

static value* check_syntax(pool* p, value* exp) {
    value* result;

    if (is_self_evaluating(exp)) {
        result = NULL;  // always ok
    } else if (is_variable(exp)) {
        result = NULL;  // always ok
    } else if (is_quoted(exp)) {
        result = check_quoted(p, exp);
    } else if (is_assignment(exp)) {
        result = check_assignment(p, exp);
        if (result == NULL) {
            result = check_syntax(p, get_assignment_value(p, exp));
        }
    } else if (is_definition(exp)) {
        result = check_definition(p, exp);
        if (result == NULL) {
            result = check_syntax(p, get_definition_value(p, exp));
        }
    } else if (is_if(exp)) {
        result = check_if(p, exp);
        if (result == NULL) {
            result = check_syntax(p, get_if_predicate(p, exp));
        }
        if (result == NULL) {
            result = check_syntax(p, get_if_consequent(p, exp));
        }
        if (result == NULL) {
            result = check_syntax(p, get_if_alternative(p, exp));
        }
    } else if (is_lambda(exp)) {
        result = check_lambda(p, exp);
        exp = get_lambda_body(p, exp);
        // recursively check body expressions
        while (result == NULL && !has_no_exps(p, exp)) {
            result = check_syntax(p, get_first_exp(p, exp));
            exp = get_rest_exps(p, exp);
        }
    } else if (is_let(exp)) {
        result = check_let(p, exp);
        if (result == NULL) {
            result = check_syntax(p, transform_let(p, exp));
        }
    } else if (is_begin(exp)) {
        result = check_begin(p, exp);
        exp = get_begin_actions(p, exp);
        // recursively check action expressions
        while (result == NULL && !has_no_exps(p, exp)) {
            result = check_syntax(p, get_first_exp(p, exp));
            exp = get_rest_exps(p, exp);
        }
    } else if (is_cond(exp)) {
        result = check_cond(p, exp);
        if (result == NULL) {
            result = check_syntax(p, transform_cond(p, exp));
        }
    } else if (is_and(exp)) {
        result = check_and(p, exp);
        exp = get_and_expressions(p, exp);
        // recursively check and expressions
        while (result == NULL && !has_no_exps(p, exp)) {
            result = check_syntax(p, get_first_exp(p, exp));
            exp = get_rest_exps(p, exp);
        }
    } else if (is_or(exp)) {
        result = check_or(p, exp);
        exp = get_or_expressions(p, exp);
        // recursively check or expressions
        while (result == NULL && !has_no_exps(p, exp)) {
            result = check_syntax(p, get_first_exp(p, exp));
            exp = get_rest_exps(p, exp);
        }
    } else if (is_eval(exp)) {
        result = check_eval(p, exp);
        if (result == NULL) {
            result = check_syntax(p, get_eval_expression(p, exp));
        }
    } else if (is_apply(exp)) {
        result = check_apply(p, exp);
        if (result == NULL) {
            result = check_syntax(p, get_apply_operator(p, exp));
        }
        if (result == NULL) {
            result = check_apply_arguments(p, get_apply_arguments(p, exp));
        }
        if (result == NULL) {
            result = check_syntax(p, get_apply_arguments(p, exp));
        }
    } else {
        // default: application
        result = check_application(p, exp);
        if (result == NULL) {
            result = check_syntax(p, get_operator(p, exp));
        }
        exp = get_operands(p, exp);
        // recursively check the operands
        while (result == NULL && !has_no_operands(p, exp)) {
            result = check_syntax(p, get_first_operand(p, exp));
            exp = get_rest_operands(p, exp);
        }
    }

    return result;
}

value* compile(pool* p, value* exp, char* target, char* linkage) {
    // check the syntax of the exp first
    value* syntax_result = check_syntax(p, exp);

    if (syntax_result != NULL) {
        // if syntax is wrong,
        // immediately return the error
        return syntax_result;
    }

    // compile the sequence
    value* seq = compile_rec(p, exp, target, linkage);

    // return the code
    return get_code(seq);
}
