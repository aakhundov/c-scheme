#include "edit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "const.h"

#ifdef _WIN32

static char buffer[BUFFER_SIZE];

char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, sizeof(buffer), stdin);

    char* result = malloc(strlen(buffer) + 1);
    strcpy(result, buffer);

    // replace the trailing '\n' by '\0'
    result[strlen(buffer) - 1] = '\0';

    return result;
}

void add_history(char* line) {
}

#endif  // _WIN32
