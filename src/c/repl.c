#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "const.h"
#include "edit.h"
#include "eval.h"
#include "hist.h"
#include "machine.h"
#include "parse.h"
#include "prim.h"
#include "value.h"

typedef enum {
    COMMAND_EXIT = 0,
    COMMAND_CLEAR = 1,
    COMMAND_TRACE = 2,
    COMMAND_RESET = 3,
    COMMAND_LOAD = 4,
    COMMAND_OTHER = -1
} command_type;

static const char* exit_commands[] = {"q", "exit", "quit"};
static const char* clear_commands[] = {"clear", "clr", "clrscr"};
static const char* trace_commands[] = {"*trace"};
static const char* reset_commands[] = {"reset"};
static const char* load_commands[] = {"*load"};

static const char** commands[] = {
    exit_commands,
    clear_commands,
    trace_commands,
    reset_commands,
    load_commands,
};

static const size_t command_counts[] = {
    sizeof(exit_commands) / sizeof(char*),
    sizeof(clear_commands) / sizeof(char*),
    sizeof(trace_commands) / sizeof(char*),
    sizeof(reset_commands) / sizeof(char*),
    sizeof(load_commands) / sizeof(char*),
};

static eval* e = NULL;
static hist* h = NULL;

static void get_input(char* input) {
    char* line = readline(">>> ");
    if (strlen(line) > 0) {
        strcpy(input, line);
        free(line);
    } else {
        free(line);
        int done = 0;
        input[0] = '\0';
        while (!done) {
            line = readline("... ");
            if (strlen(line) > 0) {
                input += sprintf(input, "%s ", line);
            } else {
                done = 1;
            }
            free(line);
        }
    }
}

static command_type get_command_type(const char* line) {
    size_t num_command_types = sizeof(commands) / sizeof(char**);
    for (size_t i = 0; i < num_command_types; i++) {
        for (size_t j = 0; j < command_counts[i]; j++) {
            const char* cmd = commands[i][j];
            if (cmd[0] == '*' && strncmp(line, cmd + 1, strlen(cmd + 1)) == 0) {
                return i;
            } else if (strcmp(line, cmd) == 0) {
                return i;
            }
        }
    }

    return COMMAND_OTHER;
}

static void process_repl_command(eval* e, hist* h, const char* input, char* output) {
    value* parsed = parse_from_str(input);

    if (parsed == NULL || parsed->type != VALUE_ERROR) {
        static char tidy[BUFFER_SIZE];
        size_t tidy_len = recover_str(parsed, tidy);
        if (tidy_len > 2) {
            // skip the outermost braces
            tidy[tidy_len - 1] = '\0';
            hist_add(h, tidy + 1);
        }

        output[0] = '\0';
        value* token = parsed;
        while (token != NULL) {
            value* result = eval_evaluate(e, token->car);
            output += value_to_str(result, output);
            value_dispose(result);

            output[0] = '\n';
            output[1] = '\0';
            output++;
            token = token->cdr;
        }
    } else {
        hist_add(h, input);
        output += value_to_str(parsed, output);
        output[0] = '\n';
        output[1] = '\0';
    }

    value_dispose(parsed);
}

value* load_from_file(eval* e, const char* path, const int verbose) {
    static char buffer[BUFFER_SIZE];
    value* content = parse_from_file(path);
    if (content != NULL && content->type == VALUE_ERROR) {
        return content;
    }

    value* running = content;
    while (running != NULL) {
        value* result = eval_evaluate(e, running->car);
        if (result != NULL && result->type == VALUE_ERROR) {
            value_dispose(content);
            return result;
        }
        if (verbose && result != NULL) {
            value_to_str(result, buffer);
            printf("%s\n", buffer);
        }
        value_dispose(result);
        running = running->cdr;
    }

    value_dispose(content);

    return NULL;
}

static int compare_str(const void* s1, const void* s2) {
    return strcmp(*((char**)s1), *((char**)s2));
}

static void load_from_path_rec(eval* e, char* path, const int verbose) {
    DIR* dir;
    if (!(dir = opendir(path))) {
        // process the file
        value* result = load_from_file(e, path, verbose);

        if (result != NULL) {
            // error while loading from the file
            static char buffer[BUFFER_SIZE];
            value_to_str(result, buffer);
            printf("error while loading from %s:\n", path);
            printf("%s\n", buffer);
            value_dispose(result);
        } else {
            printf("loaded from %s\n", path);
        }
    } else {
        size_t count = 0;
        size_t capacity = 10;
        char** item_names = malloc(capacity * sizeof(char*));

        // collect the directory item names
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            // ignore the . and .. names
            if (strcmp(entry->d_name, ".") != 0 &&
                strcmp(entry->d_name, "..") != 0) {
                // add the new item name to the list
                item_names[count] = malloc(strlen(entry->d_name) + 1);
                strcpy(item_names[count], entry->d_name);

                count++;
                if (count == capacity) {
                    // the list capacity has been
                    // reached: double the capacity
                    capacity *= 2;
                    item_names = realloc(item_names, capacity * sizeof(char*));
                }
            }
        }

        closedir(dir);

        if (count > 0) {
            // sort the directory item names in the list
            qsort(item_names, count, sizeof(char*), compare_str);

            char sub_path[1024];
            for (size_t i = 0; i < count; i++) {
                // recursively load from the sub-path: path + item name
                snprintf(sub_path, sizeof(sub_path), "%s/%s", path, item_names[i]);
                load_from_path_rec(e, sub_path, verbose);
                // free the item name
                free(item_names[i]);
            }
        }

        // free the list
        free(item_names);
    }
}

static void load_from_path(eval* e, char* path, const int verbose) {
    // trim after trailing space
    char* running = path;
    while (*running != '\0') {
        if (*running == ' ') {
            *running = '\0';
            break;
        }
        running++;
    }

    load_from_path_rec(e, path, verbose);
}

static machine_trace_level set_trace(eval* e, const char* input) {
    machine_trace_level result;
    value* tokens = parse_from_str(input);

    if (tokens == NULL ||
        tokens->type != VALUE_PAIR ||
        tokens->cdr == NULL ||
        tokens->cdr->car == NULL ||
        tokens->cdr->car->type != VALUE_NUMBER ||
        tokens->cdr->car->number < 0 ||
        tokens->cdr->car->number > TRACE_ALL ||
        tokens->cdr->cdr != NULL) {
        result = -1;
    } else {
        result = (int)tokens->cdr->car->number;
        machine_set_trace(e->machine, result);
    }

    value_dispose(tokens);

    return result;
}

static void signal_handler(int signal) {
    printf("\b\b");
    if (e != NULL) {
        machine_interrupt(e->machine);
    }
}

int main(int argc, char** argv) {
    printf("c-scheme version 0.1.0\n");
    printf("press Ctrl-C to interrupt\n");
    printf("type in \"quit\" to quit\n\n");

    int stop = 0;
    static char input[BUFFER_SIZE];
    static char output[BUFFER_SIZE];
    machine_trace_level level;

    init_primitives();
    e = eval_new(EVALUATOR_PATH);
    h = hist_new(HISTORY_PATH);

    load_from_path(e, LIBRARY_PATH, 0);
    load_from_path(e, TESTS_PATH, 0);

    printf("\n");

    while (!stop) {
        get_input(input);
        switch (get_command_type(input)) {
            case COMMAND_EXIT:
                stop = 1;
                break;
            case COMMAND_CLEAR:
                printf("\e[1;1H\e[2J");
                break;
            case COMMAND_TRACE:
                if ((level = set_trace(e, input)) != -1) {
                    printf("trace level was set to %d\n", level);
                } else {
                    printf("error setting trace level\n");
                }
                hist_add(h, input);
                break;
            case COMMAND_RESET:
                eval_reset_env(e);
                load_from_path(e, LIBRARY_PATH, 0);
                load_from_path(e, TESTS_PATH, 0);
                printf("environment was reset\n");
                hist_add(h, input);
                break;
            case COMMAND_LOAD:
                // load "x" from "load x"
                signal(SIGINT, signal_handler);
                load_from_path(e, input + 5, 1);
                signal(SIGINT, NULL);
                hist_add(h, input);
                break;
            default:
                signal(SIGINT, signal_handler);
                process_repl_command(e, h, input, output);
                signal(SIGINT, NULL);
                printf("%s", output);
        }
    }

    hist_dispose(h);
    eval_dispose(e);
    cleanup_primitives();

    printf("\nbye!\n");

    return 0;
}
