#include "hist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edit.h"

static void recover_history(char* path_to_file) {
    FILE* fp = fopen(path_to_file, "r");
    if (fp != NULL) {
        static char* line = NULL;
        size_t len = 0;
        ssize_t read = 0;

        while ((read = getline(&line, &len, fp)) != -1) {
            line[read - 1] = '\0';  // remove trailing \n
            add_history(line);
        }

        if (line) {
            free(line);
        }
        fclose(fp);
    }
}

void hist_new(hist** h, char* path_to_file) {
    *h = malloc(sizeof(hist));

    recover_history(path_to_file);

    if (path_to_file != NULL) {
        (*h)->file = fopen(path_to_file, "a");
    } else {
        (*h)->file = NULL;
    }
}

void hist_dispose(hist** h) {
    if ((*h)->file) {
        fclose((*h)->file);
        (*h)->file = NULL;
    }

    free(*h);
    *h = NULL;
}

void hist_add(hist* h, char* input) {
    HIST_ENTRY* curr = history_get(history_length);
    if (curr == NULL || strcmp(curr->line, input) != 0) {
        // add to history
        add_history(input);

        if (h->file) {
            // write to the history file
            fprintf(h->file, "%s\n", input);
            fflush(h->file);
        }
    }
}
