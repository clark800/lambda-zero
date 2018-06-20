#include <stdio.h>
#include <stdlib.h>
#include "lib/tree.h"
#include "ast.h"
#include "print.h"
#include "errors.h"

typedef struct {
    unsigned int line, column;
} Location;

bool TEST = false;
const char* SOURCE_CODE = NULL;

Location getLocation(String lexeme) {
    unsigned int index = (unsigned int)(lexeme.start - SOURCE_CODE + 1);
    Location location = {1, 1};  // use 1-based indexing
    for (unsigned int i = 0; i < index - 1; i++) {
        location.column += 1;
        if (SOURCE_CODE[i] == '\n') {
            location.line += 1;
            location.column = 1;
        }
    }
    return location;
}

void printFour(const char* a, const char* b, const char* c, const char* d) {
    fputs(a, stderr);
    fputs(b, stderr);
    fputs(c, stderr);
    fputs(d, stderr);
}

void printLocationString(String lexeme) {
    if (isInternal(lexeme)) {
        fputs("[INTERNAL]", stderr);
        return;
    }
    Location location = getLocation(lexeme);
    fputs("line ", stderr);
    fputll(location.line, stderr);
    fputs(" column ", stderr);
    fputll(location.column, stderr);
}

void printLexemeAndLocationLine(String lexeme, const char* quote) {
    fputs(quote, stderr);
    printLexeme(lexeme, stderr);
    fputs(quote, stderr);
    fputs(" at ", stderr);
    printLocationString(lexeme);
    fputs("\n", stderr);
}

void printTokenAndLocationLine(Node* token, const char* quote) {
    printLexemeAndLocationLine(getLexeme(token), quote);
}

void printError(const char* type, const char* message, String lexeme) {
    printFour(type, " error: ", message, " ");
    printLexemeAndLocationLine(lexeme, "\'");
}

void printTokenError(const char* type, const char* message, Node* token) {
    printError(type, message, getLexeme(token));
}

void lexerErrorIf(bool condition, String lexeme, const char* message) {
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
