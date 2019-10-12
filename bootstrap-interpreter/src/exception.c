#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "tree.h"
#include "stack.h"
#include "parse/term.h"
#include "closure.h"

static void printBacktrace(Closure* closure) {
    fputs("\n\nBacktrace:\n", stderr);
    Stack* backtrace = (Stack*)getBacktrace(closure);
    for (Iterator* it = iterate(backtrace); !end(it); it = next(it)) {
        fputs("  ", stderr);
        printTag(getTag(cursor(it)), "", stderr);
        fputs("\n", stderr);
    }
}

void printRuntimeError(const char* message, Closure* closure) {
    if (!TEST && !isEmpty((Stack*)getBacktrace(closure)))
        printBacktrace(closure);
    fputs("\nRuntime error: ", stderr);
    fputs(message, stderr);
    fputs(" ", stderr);
    printTag(getTag(getTerm(closure)), "\'", stderr);
    fputs("\n", stderr);
}

void runtimeError(const char* message, Closure* closure) {
    printRuntimeError(message, closure);
    exit(1);
}

