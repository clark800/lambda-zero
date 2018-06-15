#include <stdio.h>
#include "lib/tree.h"
#include "lib/util.h"
#include "scan.h"
#include "print.h"

void printLexeme(const char* lexeme, FILE* stream) {
    if (lexeme[0] < 0)
        fputs("?", stream);
    else switch (lexeme[0]) {
        case '\n': fputs("\\n", stream); break;
        case '\0': fputs("\\0", stream); break;
        default: fwrite(lexeme, sizeof(char), getLexemeLength(lexeme), stream);
    }
}

void printToken(Node* token, FILE* stream) {
    printLexeme(getLexemeByLocation(getLocation(token)), stream);
}

void fputll(long long n, FILE* stream) {
    char buffer[3 * sizeof(long long)];
    fputs(lltoa(n, buffer, 10), stream);
}
