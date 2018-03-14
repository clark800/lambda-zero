#include "libc.h"

void printError(const char* strings[]) {
    for (int i = 0; strings[i] != NULL; i++)
        fputs(strings[i], stderr);
}

void __assert(const char* filename, const char* line, const char* expression) {
    printError((const char* []){"Assertion failed: ", expression, "\n  at ",
        filename, ":", line, "\n\n", NULL});
    kill(getpid(), SIGABRT);
}
