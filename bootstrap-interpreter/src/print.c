#include <stdio.h>
#include "lib/tree.h"
#include "lib/util.h"
#include "ast.h"

void printLexeme(String lexeme, FILE* stream) {
    if (lexeme.start[0] < 0)
        fputs("?", stream);
    else switch (lexeme.start[0]) {
        case '\n': fputs("\\n", stream); break;
        case '\0': fputs("\\0", stream); break;
        default: fwrite(lexeme.start, sizeof(char), lexeme.length, stream);
    }
}

void printToken(Node* token, FILE* stream) {
    printLexeme(getLexeme(token), stream);
}

void fputll(long long n, FILE* stream) {
    char buffer[3 * sizeof(long long)];
    fputs(lltoa(n, buffer, 10), stream);
}
