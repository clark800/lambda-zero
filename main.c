#include <stdlib.h>
#include <stdio.h>
#include "lib/readfile.h"
#include "lib/tree.h"
#include "lib/freelist.h"
#include "ast.h"
#include "objects.h"
#include "errors.h"
#include "serialize.h"
#include "parse.h"
#include "evaluate.h"

void checkForMemoryLeak(const char* label, size_t expectedUsage) {
    size_t usage = getMemoryUsage();
    if (usage != expectedUsage)
        memoryError(label, (long long)(usage - expectedUsage));
}

void interpret(const char* input, bool showDebug) {
    initNodeAllocator();
    initObjects(parse(INTERNAL_CODE, false, false));
    size_t memoryUsageBeforeParse = getMemoryUsage();
    Program program = parse(input, true, showDebug);
    size_t memoryUsageBeforeEvaluate = getMemoryUsage();
    Hold* valueClosure = evaluate(program.entry, VOID, program.globals);
    if (!program.IO) {
        serialize(getNode(valueClosure), program.globals);
        fputs("\n", stdout);
    }
    release(valueClosure);
    checkForMemoryLeak("evaluate", memoryUsageBeforeEvaluate);
    deleteProgram(program);
    checkForMemoryLeak("parse", memoryUsageBeforeParse);
    deleteObjects();
    destroyNodeAllocator();
    checkForMemoryLeak("interpret", 0);
}

char* readScript(const char* filename) {
    FILE* stream = fopen(filename, "r");
    if (stream == NULL || stream == (FILE*)(-1))
        readError(filename);
    char* script = readfile(stream);
    fclose(stream);
    return script;
}

int main(int argc, char* argv[]) {
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    bool showDebug = false;
    while (--argc > 0 && (*++argv)[0] == '-')
        for (char* flag = argv[0] + 1; *flag != '\0'; flag++)
            switch (*flag) {
                case 'd': showDebug = true; break;    // show parse steps
                case 't': TRACE = true; break;        // show eval steps
                case 'p': PROFILE = true; break;      // show eval loop count
                case 'n': VERBOSITY = -1; break;      // hide line #s in errors
                default: usageError(argv[0]); break;
            }
    if (argc > 1)
        usageError(argv[0]);
    char* input = argc == 0 ? readfile(stdin) : readScript(argv[0]);
    interpret(input, showDebug);
    free(input);
    return 0;
}
