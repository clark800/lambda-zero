#include <stdlib.h>
#include <stdio.h>
#include "shared/lib/readfile.h"
#include "shared/lib/tree.h"
#include "shared/lib/array.h"
#include "shared/lib/freelist.h"
#include "shared/lib/util.h"
#include "shared/term.h"
#include "parse/parse.h"
#include "evaluate/closure.h"
#include "evaluate/evaluate.h"

bool TEST = false;
extern bool isIO;

static void serializeNode(Node* node, Node* locals, const Array* globals,
        unsigned int depth, FILE* stream) {
    switch (getTermType(node)) {
        case VARIABLE:
            if (isGlobal(node) || getDebruijnIndex(node) <= depth) {
                printString(getLexeme(node), stream);
                break;
            }
            unsigned long long debruijn = getDebruijnIndex(node);
            Closure* next = getListElement(locals, debruijn - depth - 1);
            serializeNode(getTerm(next), getLocals(next), globals, 0, stream);
            break;
        case APPLICATION:
            fputs("(", stream);
            serializeNode(getLeft(node), locals, globals, depth, stream);
            fputs(" ", stream);
            serializeNode(getRight(node), locals, globals, depth, stream);
            fputs(")", stream);
            break;
        case ABSTRACTION:
            fputs("(", stream);
            printString(getLexeme(getParameter(node)), stream);
            fputs(" \u21A6 ", stream);
            serializeNode(getBody(node), locals, globals, depth + 1, stream);
            fputs(")", stream);
            break;
        case NUMERAL: fputll(getValue(node), stream); break;
        case OPERATION: printString(getLexeme(node), stream); break;
    }
}

static void serialize(Closure* closure, const Array* globals) {
    serializeNode(getTerm(closure), getLocals(closure), globals, 0, stdout);
    fputs("\n", stdout);
}

static void print3(const char* a, const char* b, const char* c) {
    fputs(a, stderr);
    fputs(b, stderr);
    fputs(c, stderr);
}

void usageError(const char* name) {
    print3("Usage error: ", name, " [-d] [-D] [-t] [FILE]\n");
    exit(2);
}

void readError(const char* filename) {
    print3("Usage error: file '", filename, "' cannot be opened\n");
    exit(2);
}

void memoryError(const char* label, long long bytes) {
    print3("MEMORY LEAK IN \"", label, "\": ");
    fputll(bytes, stderr);
    fputs(" bytes\n", stderr);
    exit(3);
}

static void checkForMemoryLeak(const char* label, size_t expectedUsage) {
    size_t usage = getMemoryUsage();
    if (usage != expectedUsage)
        memoryError(label, (long long)(usage - expectedUsage));
}

static void interpret(const char* sourceCode) {
    initNodeAllocator();
    Program program = parse(sourceCode);
    size_t memoryUsageBeforeEvaluate = getMemoryUsage();
    Hold* valueClosure = evaluateTerm(program.entry, program.globals);
    size_t memoryUsageBeforeSerialize = getMemoryUsage();
    if (!isIO)
        serialize(getNode(valueClosure), program.globals);
    checkForMemoryLeak("serialize", memoryUsageBeforeSerialize);
    release(valueClosure);
    checkForMemoryLeak("evaluate", memoryUsageBeforeEvaluate);
    deleteProgram(program);
    checkForMemoryLeak("parse", 0);
    destroyNodeAllocator();
}

static char* readSourceCode(const char* filename) {
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
    // note: disabling buffering slows down I/O quite a bit, but it is necessary
    // for binary protocols like X Windows.
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    const char* programName = argv[0];
    while (--argc > 0 && (*++argv)[0] == '-') {
        for (const char* flag = argv[0] + 1; flag[0] != '\0'; ++flag) {
            switch (flag[0]) {
                case 'd': DEBUG = 1; break;
                case 'D': DEBUG = 2; break;
                case 't': TEST = true; break;      // hide line #s in errors
                default: usageError(programName); break;
            }
        }
    }
    if (argc > 1)
        usageError(programName);
    char* sourceCode = argc == 0 ? readfile(stdin) : readSourceCode(argv[0]);
    interpret(sourceCode);
    free(sourceCode);
    return 0;
}
