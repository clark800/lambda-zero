#include "libc.h"

void error(const char* strings[]) {
    for (int i = 0; strings[i] != NULL; i++)
        fputs(strings[i], stderr);
}

void __assert(const char* filename, const char* line, const char* expression) {
    error((const char* []){"Assertion failed: ", expression, "\n  at ",
        filename, ":", line, "\n\n", NULL});
    kill(getpid(), SIGABRT);
}
