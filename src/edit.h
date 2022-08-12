#ifndef EDIT_H_
#define EDIT_H_

#ifdef _WIN32

char* readline(char* prompt);
void add_history(char* line);

#elif __APPLE__

#include <editline/readline.h>

#elif __linux__

#include <editline/history.h>
#include <editline/readline.h>

#endif  // _WIN32

#endif  // EDIT_H_
