#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/readfile.h"
#include "lib/tree.h"
#include "lib/stack.h"
#include "lib/freelist.h"
#include "lib/errors.h"
#include "lib/lltoa.h"
#include "lex.h"
#include "parse.h"
#include "builtins.h"
#include "evaluate.h"
#include "serialize.h"

static inline size_t getTotalAllocatedMemory(void) {
    #ifdef LIBC_H
    return mstats();
    #else
    return 0;
    #endif
}

void memoryError(const char* label, long long bytes) {
    errorArray(4, (strings){"MEMORY LEAK IN \"", label, "\":", " "});
    serializeInteger(bytes, stderr);
    fputs(" bytes\n", stderr);
}

void usageError(const char* name) {
    errorArray(3, (strings){"Usage error: ", name, " [-d] [-p] [FILE]"});
    exit(2);
}

void checkForMemoryLeak(const char* label, size_t expectedUsage) {
    size_t usage = getMemoryUsage();
    if (usage != expectedUsage)
        memoryError(label, (long long)(usage - expectedUsage));
}

void checkForMallocMemoryLeak(void) {
    size_t memory = getTotalAllocatedMemory();
    if (memory != 0)
        memoryError("malloc", (long long)memory);
}

void interpret(const char* input) {
    size_t memoryUsageAtStart = getMemoryUsage();
    Hold* parsed = parse(input);
    size_t memoryUsageBeforeEvaluate = getMemoryUsage();
    Hold* closure = evaluate(getNode(parsed));
    if (closure != NULL) {
        serializeClosure(getNode(closure), stdout);
        fputs("\n", stdout);
        release(closure);
    }
    checkForMemoryLeak("evaluate", memoryUsageBeforeEvaluate);
    release(parsed);
    checkForMemoryLeak("interpret", memoryUsageAtStart);
}

char* readScript(const char* filename) {
    FILE* stream = fopen(filename, "r");
    if (stream == NULL || stream == (FILE*)(-1)) {
        errorArray(3, (strings){
            "Usage error: file '", filename, "' cannot be opened"});
        exit(2);
    }
    char* script = readfile(stream);
    fclose(stream);
    return script;
}

int main(int argc, char* argv[]) {
    initNodeAllocator();
    initBuiltins();
    char* program = argv[0];
    while (--argc > 0 && (*++argv)[0] == '-')
        for (char* flag = argv[0] + 1; *flag != '\0'; flag++)
            switch (*flag) {
                case 'd': DEBUG = true; break;  // show parse and eval steps
                case 'p': PROFILE = true; break;  // show eval loop count
                case 't': TEST = true; break;  // omit line numbers from errors
                default: usageError(program); break;
            }
    if (argc > 1)
        usageError(program);
    char* input = argc == 0 ? readfile(stdin) : readScript(argv[0]);
    interpret(input);
    free(input);
    deleteBuiltins();
    checkForMemoryLeak("main", 0);
    destroyNodeAllocator();
    checkForMallocMemoryLeak();
    return 0;
}
