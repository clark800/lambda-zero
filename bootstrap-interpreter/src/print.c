#include <stdio.h>
#include <stdbool.h>
#include "lib/tag.h"
#include "lib/util.h"

void printLine(const char* line, FILE* stream) {
    size_t length = 0;
    for (; line[length] != '\0' && line[length] != '\n'; ++length);
    fwrite(line, sizeof(char), length, stream);
}

void printLexeme(String lexeme, FILE* stream) {
    if (lexeme.length == 0 || lexeme.start[0] == '\0')
        fputs("\\0", stream);
    else switch (lexeme.start[0]) {
        case '\n': fputs("\\n", stream); break;
        default: fwrite(lexeme.start, sizeof(char), lexeme.length, stream);
    }
}

void fputll(long long n, FILE* stream) {
    char buffer[3 * sizeof(long long)];
    fputs(lltoa(n, buffer, 10), stream);
}
