#include <stdlib.h>
#include <stdio.h>
#include "readfile.h"
#include "tree.h"
#include "array.h"
#include "freelist.h"
#include "util.h"
#include "parse/opp/operator.h"
#include "parse/term.h"
#include "parse/parse.h"
#include "closure.h"
#include "evaluate.h"

bool TEST = false;
extern bool isIO;

static void showTag(Tag tag, FILE* stream) {
    if (getTagFixity(tag) == NOFIX) {
        printTag(tag, stream);
    } else {
        fputs("(", stream);
        printTag(tag, stream);
        fputs(")", stream);
    }
}

static Hold* resolveLocals(Term* term, Node* locals, unsigned int depth) {
    switch (getTermType(term)) {
        case VARIABLE: {
            if (isGlobal(term) || getDebruijnIndex(term) <= depth)
                return hold(term);
            Closure* closure = getListElement(locals,
                getDebruijnIndex(term) - depth - 1);
            return resolveLocals(getTerm(closure), getLocals(closure), 0);
        } case APPLICATION: {
            Hold* left = resolveLocals(getLeft(term), locals, depth);
            Hold* right = resolveLocals(getRight(term), locals, depth);
            Term* ap = Application(getTag(term), getNode(left), getNode(right));
            release(left);
            release(right);
            return hold(ap);
        } case ABSTRACTION: {
            Hold* body = resolveLocals(getBody(term), locals, depth + 1);
            Term* abstraction = Abstraction(getTag(term), getNode(body));
            release(body);
            return hold(abstraction);
        } default: return hold(term);
    }
}

static void showTerm(Term* term, FILE* stream) {
    switch (getTermType(term)) {
        case APPLICATION:
            if (isAbstraction(getLeft(term)))
                fputs("(", stream);
            showTerm(getLeft(term), stream);
            if (isAbstraction(getLeft(term)))
                fputs(")", stream);
            fputs("(", stream);
            showTerm(getRight(term), stream);
            fputs(")", stream);
            break;
        case ABSTRACTION:
            showTag(getTag(term), stream);
            fputs(" \xE2\x86\xA6 ", stream); // u21A6
            showTerm(getBody(term), stream);
            break;
        case NUMERAL: fputll(getValue(term), stream); break;
        default: showTag(getTag(term), stream); break;
    }
}

static void showClosure(Closure* closure, FILE* stream) {
    Hold* term = resolveLocals(getTerm(closure), getLocals(closure), 0);
    showTerm(getNode(term), stream);
    release(term);
    fputs("\n", stream);
}

static void print3(const char* a, const char* b, const char* c) {
    fputs(a, stderr);
    fputs(b, stderr);
    fputs(c, stderr);
}

static void usageError(const char* name) {
    print3("Usage error: ", name, " [-d] [-D] [-t] [FILE]\n");
    exit(2);
}

static void readError(const char* filename) {
    print3("Usage error: file '", filename, "' cannot be opened\n");
    exit(2);
}

static void memoryError(const char* label, long long bytes) {
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
        showClosure(getNode(valueClosure), stdout);
    checkForMemoryLeak("serialize", memoryUsageBeforeSerialize);
    release(valueClosure);
    checkForMemoryLeak("evaluate", memoryUsageBeforeEvaluate);
    deleteProgram(program);
    checkForMemoryLeak("parse", 0);
    destroyNodeAllocator();
}

static char* readSourceCode(const char* filename) {
    FILE* stream = fopen(filename, "r");
    if (stream == NULL)
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
