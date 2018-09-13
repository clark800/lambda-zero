#include <stdio.h>
#include <stdlib.h>
#include "lib/tree.h"
#include "ast.h"
#include "print.h"

extern bool TEST;

static void printThree(const char* a, const char* b, const char* c) {
    fputs(a, stderr);
    fputs(b, stderr);
    fputs(c, stderr);
}

void printTagLine(Node* node, const char* quote) {
    Tag tag = getTag(node);
    if (isLeaf(node)) {
        fputs(" ", stderr);
        fputs(quote, stderr);
        printLexeme(tag.lexeme, stderr);
        fputs(quote, stderr);
    }
    fputs(" at line ", stderr);
    fputll(tag.location.line, stderr);
    fputs(" column ", stderr);
    fputll(tag.location.column, stderr);
    fputs("\n", stderr);
}

void printError(const char* type, const char* message, Node* node) {
    printThree(type, " error: ", message);
    printTagLine(node, "\'");
}

void syntaxError(const char* message, Node* token) {
    printError("Syntax", message, token);
    exit(1);
}

void syntaxErrorIf(bool condition, const char* message, Node* token) {
    if (condition)
        syntaxError(message, token);
}

void usageError(const char* name) {
    printThree("Usage error: ", name, " [-p] [-t] [FILE]\n");
    exit(2);
}

void readError(const char* filename) {
    printThree("Usage error: file '", filename, "' cannot be opened\n");
    exit(2);
}

void memoryError(const char* label, long long bytes) {
    printThree("MEMORY LEAK IN \"", label, "\": ");
    fputll(bytes, stderr);
    fputs(" bytes\n", stderr);
    exit(3);
}
