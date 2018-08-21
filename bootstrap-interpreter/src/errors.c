#include <stdio.h>
#include <stdlib.h>
#include "lib/tree.h"
#include "ast.h"
#include "print.h"
#include "errors.h"

extern bool TEST;

static void printFour(
        const char* a, const char* b, const char* c, const char* d) {
    fputs(a, stderr);
    fputs(b, stderr);
    fputs(c, stderr);
    fputs(d, stderr);
}

static void printLocationString(Location location) {
    if (location.line == 0) {
        fputs("[EOF]", stderr);
        return;
    }
    fputs("line ", stderr);
    fputll(location.line, stderr);
    fputs(" column ", stderr);
    fputll(location.column, stderr);
}

void printTagLine(Tag tag, const char* quote) {
    if (tag.lexeme.length > 0) {
        fputs(quote, stderr);
        printLexeme(tag.lexeme, stderr);
        fputs(quote, stderr);
        fputs(" ", stderr);
    }
    fputs("at ", stderr);
    printLocationString(tag.location);
    fputs("\n", stderr);
}

void printError(const char* type, const char* message, Tag tag) {
    printFour(type, " error: ", message, " ");
    printTagLine(tag, "\'");
}

void syntaxError(const char* message, Node* token) {
    printError("Syntax", message, getTag(token));
    exit(1);
}

void syntaxErrorIf(bool condition, const char* message, Node* token) {
    if (condition)
        syntaxError(message, token);
}

void usageError(const char* name) {
    printFour("Usage error: ", name, " [-p] [-e] [-t]", " [FILE]\n");
    exit(2);
}

void readError(const char* filename) {
    printFour("Usage error: ", "file '", filename, "' cannot be opened\n");
    exit(2);
}

void memoryError(const char* label, long long bytes) {
    printFour("MEMORY LEAK IN \"", label, "\":", " ");
    fputll(bytes, stderr);
    fputs(" bytes\n", stderr);
    exit(3);
}
