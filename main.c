#include <stdlib.h>
#include <stdio.h>
#include "lib/readfile.h"
#include "lib/tree.h"
#include "lib/freelist.h"
#include "lib/errors.h"
#include "objects.h"
#include "serialize.h"
#include "parse.h"
#include "evaluate.h"

void memoryError(const char* label, long long bytes) {
    errorArray(4, (strings){"MEMORY LEAK IN \"", label, "\":", " "});
    debugInteger(bytes);
    debug(" bytes\n");
}

void usageError(const char* name) {
    errorArray(3, (strings){"Usage error: ", name,
            " [-d] [-t] [-p] [-n] [FILE]"});
    exit(2);
}

void checkForMemoryLeak(const char* label, size_t expectedUsage) {
    size_t usage = getMemoryUsage();
    if (usage != expectedUsage)
        memoryError(label, (long long)(usage - expectedUsage));
}

void interpret(const char* input) {
    size_t memoryUsageAtStart = getMemoryUsage();
    Hold* parsed = parse(input);
    size_t memoryUsageBeforeEvaluate = getMemoryUsage();
    Hold* valueClosure = evaluate(getNode(parsed), NULL);
    if (!IO) {
        serialize(getClosureTerm(getNode(valueClosure)),
                getClosureEnv(getNode(valueClosure)), stdout);
        fputs("\n", stdout);
    }
    release(valueClosure);
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
    setbuf(stdout, NULL);
    initNodeAllocator();
    initObjects(parse(OBJECTS));
    char* program = argv[0];
    while (--argc > 0 && (*++argv)[0] == '-')
        for (char* flag = argv[0] + 1; *flag != '\0'; flag++)
            switch (*flag) {
                case 'd': DEBUG = true; break;    // show parse steps
                case 't': TRACE = true; break;    // show eval steps
                case 'p': PROFILE = true; break;  // show eval loop count
                case 'n': VERBOSITY = -1; break;  // hide line #s in errors
                default: usageError(program); break;
            }
    if (argc > 1)
        usageError(program);
    char* input = argc == 0 ? readfile(stdin) : readScript(argv[0]);
    interpret(input);
    free(input);
    deleteObjects();
    checkForMemoryLeak("main", 0);
    destroyNodeAllocator();
    return 0;
}
