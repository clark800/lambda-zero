#ifndef SCAN_H
#define SCAN_H

#include <stdbool.h>
#include <ctype.h>

static inline bool isQuoteCharacter(char c) {
    return c == '"' || c == '\'';
}

static inline bool isDelimiterCharacter(char c) {
    return c == ' ' || c == '\n' || c == '\0' || c == ',' || c == ';' ||
        c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}';
}

static inline bool isOperandCharacter(char c) {
    return isalnum(c) || isQuoteCharacter(c) || c == '_' || c == '$';
}

static inline bool isOperatorCharacter(char c) {
    return ispunct(c) && !isDelimiterCharacter(c) && !isOperandCharacter(c);
}

const char* getFirstLexeme(const char* input);
const char* getNextLexeme(const char* lastLexeme);
unsigned int getLexemeLength(const char* lexeme);
bool isSameLexeme(const char* a, const char* b);

#endif
