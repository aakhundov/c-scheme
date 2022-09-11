#include <string.h>

#include "repl.h"
#include "test.h"

int main(int argc, char** argv) {
    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        run_test();
    } else {
        run_repl();
    }

    return 0;
}
