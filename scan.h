#ifndef SCAN_H
#define SCAN_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    unsigned int line, column;
} Position;

static inline bool isQuoteCharacter(char c) {
    return c == '"' || c == '\'';
}

static inline bool isDelimiterCharacter(char c) {
    return c == ' ' || c == '\n' || c == '\0' || c == ',' || c == ';' ||
        c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}';
}

static inline bool isOperandCharacter(char c) {
    return isalnum(c) || c == '\'' || c == '_';
}

static inline bool isOperatorCharacter(char c) {
    return ispunct(c) && !isDelimiterCharacter(c) && !isOperandCharacter(c)
        && !isQuoteCharacter(c) && strchr("{};`!@$", c) == NULL;
}

const char* getFirstLexeme(const char* input);
const char* getNextLexeme(const char* lastLexeme);
unsigned int getLexemeLength(const char* lexeme);
bool isSameLexeme(const char* a, const char* b);
int getLexemeLocation(const char* lexeme);
const char* getLexemeByLocation(int location);
Position getPosition(unsigned int location);
void printLexeme(const char* lexeme, FILE* stream);

#endif
