#include "repl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edit.h"
#include "parse.h"
#include "value.h"

typedef enum {
    COMMAND_EXIT = 0,
    COMMAND_CLEAR = 1,
    COMMAND_OTHER = -1
} command_type;

static const char* exit_commands[] = {"exit", "quit", "q"};
static const char* clear_commands[] = {"clear", "clr", "clrscr"};

static const char** commands[] = {
    exit_commands,
    clear_commands,
};

static const size_t command_counts[] = {
    sizeof(exit_commands) / sizeof(char*),
    sizeof(clear_commands) / sizeof(char*),
};

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

static command_type get_command_type(char* line) {
    size_t num_command_types = sizeof(commands) / sizeof(char**);
    for (size_t i = 0; i < num_command_types; i++) {
        for (size_t j = 0; j < command_counts[i]; j++) {
            if (strcmp(line, commands[i][j]) == 0) {
                return i;
            }
        }
    }

    return COMMAND_OTHER;
}

static void process_repl_command(char* input, char* output) {
    value* parsed = parse_values(input);

    if (parsed->type != VALUE_ERROR) {
        char tidy_input[16384];
        size_t tidy_len = value_to_str(parsed, tidy_input);
        if (tidy_len > 2) {
            // skip the outermost braces
            tidy_input[tidy_len - 1] = '\0';
            add_history(tidy_input + 1);
        }

        output[0] = '\0';
        value* token = parsed;
        while (!value_is_null_pair(token)) {
            output += value_to_str(token->car, output);
            output[0] = '\n';
            output[1] = '\0';
            output++;
            token = token->cdr;
        }
    } else {
        add_history(input);
        output += value_to_str(parsed, output);
        output[0] = '\n';
        output[1] = '\0';
    }

    value_dispose(parsed);
}

void run_repl() {
    printf("c-scheme version 0.0.1\n");
    printf("type in \"q\" to quit\n\n");

    int stop = 0;
    char input[65536];
    char output[65536];

    while (!stop) {
        get_input(input);
        switch (get_command_type(input)) {
            case COMMAND_EXIT:
                stop = 1;
                break;
            case COMMAND_CLEAR:
                printf("\e[1;1H\e[2J");
                break;
            default:
                process_repl_command(input, output);
                printf("%s", output);
        }
    }

    printf("\nbye!\n");
}
