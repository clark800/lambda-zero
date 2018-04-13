#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "objects.h"
#include "scan.h"

const char* SOURCE_CODE = NULL;

static inline bool isComment(const char* s) {
    return s[0] == '/' && s[1] == '/';
}

static inline bool isNotNewlineCharacter(char c) {
    return c != '\n';
}

static inline const char* skipWhile(const char* s, bool (*predicate)(char)) {
    while (s[0] != '\0' && predicate(s[0]))
        s++;
    return s;
}

static inline const char* skipComment(const char* s) {
    return isComment(s) ? skipWhile(s, isNotNewlineCharacter) : s;
}

static inline const char* skipQuote(const char* s) {
    char quote = s[0];
    // assumption is that s points to the opening quotation mark
    for (s += 1; s[0] != '\0' && s[0] != '\n'; s += 1) {
        if (s[0] == '\\' && s[1] != '\0')
            s += 1;     // skip character following slash
        else if (s[0] == quote)
            return s + 1;
    }
    return s;
}

static inline const char* skipLexeme(const char* lexeme) {
    assert(lexeme[0] != '\0');
    if (isSpaceCharacter(lexeme[0]))
        return skipWhile(lexeme, isSpaceCharacter);
    if (isQuoteCharacter(lexeme[0]))
        return skipQuote(lexeme);
    if (isOperandCharacter(lexeme[0]))
        return skipWhile(lexeme, isOperandCharacter);
    if (isOperatorCharacter(lexeme[0]))
        return skipWhile(lexeme, isOperatorCharacter);
    return lexeme + 1;    // delimiter or illegal character
}

const char* getFirstLexeme(const char* input) {
    SOURCE_CODE = input;
    return skipComment(input);
}

const char* getNextLexeme(const char* lastLexeme) {
    return skipComment(skipLexeme(lastLexeme));
}

unsigned int getLexemeLength(const char* lexeme) {
    return (unsigned int)(lexeme[0] == '\0' ? 1 : skipLexeme(lexeme) - lexeme);
}

bool isSameLexeme(const char* a, const char* b) {
    unsigned int lengthA = getLexemeLength(a);
    return getLexemeLength(b) == lengthA && strncmp(a, b, lengthA) == 0;
}

int getLexemeLocation(const char* lexeme) {
    return (int)(lexeme - SOURCE_CODE + 1);
}

const char* getLexemeByLocation(int location) {
    const char* start = location < 0 ? INTERNAL_CODE : SOURCE_CODE;
    return location == 0 ? "\0" : &start[abs(location) - 1];
}

Position getPosition(unsigned int location) {
    Position position = {1, 1};  // use 1-based indexing
    for (unsigned int i = 0; i < location - 1; i++) {
        position.column += 1;
        if (SOURCE_CODE[i] == '\n') {
            position.line += 1;
            position.column = 1;
        }
    }
    return position;
}

void printLexeme(const char* lexeme, FILE* stream) {
    switch (lexeme[0]) {
        case '\n': fputs("\\n", stream); break;
        case '\0': fputs("\\0", stream); break;
        default: fwrite(lexeme, sizeof(char), getLexemeLength(lexeme), stream);
    }
}
