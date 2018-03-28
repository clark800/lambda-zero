#include <string.h>
#include <stdlib.h>
#include "lib/tree.h"
#include "lib/lltoa.h"
#include "scan.h"
#include "errors.h"

int VERBOSITY = 0;

typedef const char* strings[];

void errorArray(int count, const char* strs[]) {
    for (int i = 0; i < count; i++)
        fputs(strs[i], stderr);
}

const char* getLocationString(int location) {
    if (location < 0)
        return "[INTERNAL OBJECT]";
    static char buffer[128];
    Position position = getPosition((unsigned int)location);
    strcpy(buffer, "line ");
    lltoa(position.line, buffer + strlen(buffer), 10);
    strcat(buffer, " column ");
    lltoa(position.column, buffer + strlen(buffer), 10);
    return buffer;
}

void throwError(const char* type, const char* message, const char* lexeme) {
    errorArray(4, (strings){type, " error: ", message, " \'"});
    printLexeme(lexeme, stderr);
    errorArray(1, (strings){"\'"});
    const char* location = getLocationString(getLexemeLocation(lexeme));
    if (VERBOSITY >= 0)
        errorArray(3, (strings){" at ", location, "\n"});
    exit(1);
}

void throwTokenError(const char* type, const char* message, Node* token) {
    throwError(type, message, getLexemeByLocation(getLocation(token)));
}

void lexerErrorIf(bool condition, const char* lexeme, const char* message) {
    if (condition)
        throwError("Syntax", message, lexeme);
}

void syntaxError(const char* message, Node* token) {
    throwTokenError("Syntax", message, token);
}

void syntaxErrorIf(bool condition, const char* message, Node* token) {
    if (condition)
        syntaxError(message, token);
}

void runtimeError(const char* message, Node* token) {
    throwTokenError("\nRuntime", message, token);
}

void memoryError(const char* label, long long bytes) {
    errorArray(3, (strings){"MEMORY LEAK IN \"", label, "\": "});
    fputll(bytes, stderr);
    fputs(" bytes\n", stderr);
}

void usageError(const char* name) {
    errorArray(3, (strings){"Usage error: ", name, " [-dtpn] [FILE]"});
    exit(2);
}

void readError(const char* filename) {
    errorArray(3, (strings){
        "Usage error: file '", filename, "' cannot be opened"});
    exit(2);
}
