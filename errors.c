#include <stdio.h>
#include <stdlib.h>
#include "lib/tree.h"
#include "scan.h"
#include "print.h"
#include "errors.h"

bool TEST = false;

static inline const char* getLexeme(Node* node) {
    return getLexemeByLocation(getLocation(node));
}

void printFour(const char* a, const char* b, const char* c, const char* d) {
    fputs(a, stderr);
    fputs(b, stderr);
    fputs(c, stderr);
    fputs(d, stderr);
}

void printLocationString(int location) {
    if (location < 0) {
        fputs("[INTERNAL OBJECT]", stderr);
        return;
    }
    Position position = getPosition((unsigned int)location);
    fputs("line ", stderr);
    fputll(position.line, stderr);
    fputs(" column ", stderr);
    fputll(position.column, stderr);
}

void printLexemeAndLocationLine(const char* lexeme, const char* quote) {
    fputs(quote, stderr);
    printLexeme(lexeme, stderr);
    fputs(quote, stderr);
    fputs(" at ", stderr);
    printLocationString(getLexemeLocation(lexeme));
    fputs("\n", stderr);
}

void printTokenAndLocationLine(Node* token, const char* quote) {
    printLexemeAndLocationLine(getLexeme(token), quote);
}

void printError(const char* type, const char* message, const char* lexeme) {
    printFour(type, " error: ", message, " ");
    printLexemeAndLocationLine(lexeme, "\'");
}

void printTokenError(const char* type, const char* message, Node* token) {
    printError(type, message, getLexeme(token));
}

void lexerErrorIf(bool condition, const char* lexeme, const char* message) {
    if (condition) {
        printError("Syntax", message, lexeme);
        exit(1);
    }
}

void syntaxError(const char* message, Node* token) {
    printTokenError("Syntax", message, token);
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

void printMemoryError(const char* label, long long bytes) {
    printFour("MEMORY LEAK IN \"", label, "\":", " ");
    fputll(bytes, stderr);
    fputs(" bytes\n", stderr);
}
