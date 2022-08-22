#ifndef HIST_H_
#define HIST_H_

#include <stdio.h>

typedef struct hist hist;

struct hist {
    FILE* file;
};

void hist_new(hist** h, char* path_to_file);
void hist_dispose(hist** h);

void hist_add(hist* h, char* input);

#endif  // HIST_H_
