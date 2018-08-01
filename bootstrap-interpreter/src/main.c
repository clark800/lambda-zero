#include <stdlib.h>
#include <stdio.h>
#include "lib/readfile.h"
#include "lib/tree.h"
#include "lib/array.h"
#include "lib/freelist.h"
#include "closure.h"
#include "errors.h"
#include "parse.h"
#include "evaluate.h"
#include "serialize.h"

void checkForMemoryLeak(const char* label, size_t expectedUsage) {
    size_t usage = getMemoryUsage();
    if (usage != expectedUsage)
        printMemoryError(label, (long long)(usage - expectedUsage));
}

void interpret(const char* sourceCode) {
    initNodeAllocator();
    Program internal = parse(NULL);
    Program program = parse(sourceCode);
    Hold* valueClosure = evaluateTerm(program.entry, program.globals);
    if (!isIO(program))
        serialize(getNode(valueClosure), program.globals);
    release(valueClosure);
    deleteProgram(program);
    deleteProgram(internal);
    destroyNodeAllocator();
    checkForMemoryLeak("interpret", 0);
}

char* readSourceCode(const char* filename) {
    FILE* stream = fopen(filename, "r");
    if (stream == NULL || stream == (FILE*)(-1))
        readError(filename);
    char* sourceCode = readfile(stream);
    fclose(stream);
    return sourceCode;
}

int main(int argc, char* argv[]) {
    // note: setbuf(stdin, NULL) will leave unread input in stdin on exit
    // causing the shell to execute it, which is dangerous
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    char* programName = argv[0];
    while (--argc > 0 && (*++argv)[0] == '-')
        for (char* flag = argv[0] + 1; *flag != '\0'; flag++)
            switch (*flag) {
                case 'p': TRACE_PARSING = true; break;
                case 'e': TRACE_EVALUATION = true; break;
                case 't': TEST = true; break;      // hide line #s in errors
                default: usageError(programName); break;
            }
    if (argc > 1)
        usageError(programName);
    char* sourceCode = argc == 0 ? readfile(stdin) : readSourceCode(argv[0]);
    interpret(sourceCode);
    free(sourceCode);
    return 0;
}
