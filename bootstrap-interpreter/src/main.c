#include <stdlib.h>
#include <stdio.h>
#include "lib/readfile.h"
#include "lib/tree.h"
#include "lib/array.h"
#include "lib/freelist.h"
#include "print.h"
#include "errors.h"
#include "term.h"
#include "parse/parse.h"
#include "evaluate/closure.h"
#include "evaluate/evaluate.h"

bool TEST = false;
extern bool isIO;

static inline String getLexeme(Node* n) {return getTag(n).lexeme;}

static void serializeNode(Node* node, Node* locals, const Array* globals,
        unsigned int depth, FILE* stream) {
    switch (getTermType(node)) {
        case VARIABLE:
            if (isGlobal(node)) {
                Node* value = elementAt(globals, getGlobalIndex(node));
                serializeNode(value, locals, globals, 0, stream);
                break;
            }
            unsigned long long debruijn = getDebruijnIndex(node);
            if (debruijn <= depth) {
                printLexeme(getLexeme(node), stream);
                break;
            }
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
            printLexeme(getLexeme(getParameter(node)), stream);
            fputs(" -> ", stream);
            serializeNode(getBody(node), locals, globals, depth + 1, stream);
            fputs(")", stream);
            break;
        case NUMERAL: fputll(getValue(node), stream); break;
        case OPERATION: printLexeme(getLexeme(node), stream); break;
        default: fputs("#ERROR#", stream); break;
    }
}

static void serialize(Closure* closure, const Array* globals) {
    serializeNode(getTerm(closure), getLocals(closure), globals, 0, stdout);
    fputs("\n", stdout);
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
