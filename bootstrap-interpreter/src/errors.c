#include <stdio.h>
#include <stdlib.h>
#include "lib/tree.h"
#include "print.h"

extern bool TEST;

static void printThree(const char* a, const char* b, const char* c) {
    fputs(a, stderr);
    fputs(b, stderr);
    fputs(c, stderr);
}

void printTagLine(Tag tag, const char* quote) {
    fputs(" ", stderr);
    fputs(quote, stderr);
    printLexeme(tag.lexeme, stderr);
    fputs(quote, stderr);
    fputs(" at line ", stderr);
    fputll(tag.location.line, stderr);
    fputs(" column ", stderr);
    fputll(tag.location.column, stderr);
    fputs("\n", stderr);
}

void printError(const char* type, const char* message, Tag tag) {
    printThree(type, " error: ", message);
    printTagLine(tag, "\'");
}

void tokenErrorIf(bool condition, const char* message, Tag tag) {
    if (condition) {
        printError("Syntax", message, tag);
        exit(1);
    }
}

void syntaxErrorIf(bool condition, const char* message, Node* node) {
    tokenErrorIf(condition, message, getTag(node));
}

void syntaxError(const char* message, Node* node) {
    syntaxErrorIf(true, message, node);
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
