#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "scan.h"

bool isComment(const char* start) {
    return start[0] == '-' && start[1] == '-';
}

bool isEscapedNewline(const char* start) {
    return start[0] == '\\' && start[1] == '\n';
}

const char* skipWhile(const char* str, bool (*predicate)(char)) {
    while (str[0] != '\0' && predicate(str[0]))
        str++;
    return str;
}

bool isNotNewlineCharacter(char c) {
    return c != '\n';
}

const char* skipToNewline(const char* str) {
    return skipWhile(str, isNotNewlineCharacter);
}

const char* skipCharacter(const char* str) {
    return str[0] == '\0' ? str : str + 1;
}

const char* skipQuote(const char* start) {
    char quote = start[0];
    // assumption is that start points to the opening quotation mark
    for (start += 1; start[0] != '\0' && start[0] != '\n'; start += 1) {
        if (start[0] == '\\' && start[1] != '\0')
            start += 1;     // skip character following slash
        else if (start[0] == quote)
            return start + 1;
    }
    return start;
}

const char* skipLexeme(const char* lexeme) {
    assert(lexeme[0] != '\0');
    if (isSpaceCharacter(lexeme[0]))
        return skipWhile(lexeme, isSpaceCharacter);
    if (isDelimiterCharacter(lexeme[0]))
        return skipCharacter(lexeme);
    if (isQuoteCharacter(lexeme[0]))
        return skipQuote(lexeme);
    if (isOperandCharacter(lexeme[0]))
        return skipWhile(lexeme, isOperandCharacter);
    if (isOperatorCharacter(lexeme[0]))
        return skipWhile(lexeme, isOperatorCharacter);
    return skipCharacter(lexeme);    // illegal character
}

const char* skipElided(const char* str) {
    while (isComment(str) || isEscapedNewline(str))
        str = isComment(str) ? skipToNewline(str) :
            skipCharacter(skipToNewline(str));
    return str;
}

const char* getFirstLexeme(const char* input) {
    return skipElided(input);
}

const char* getNextLexeme(const char* lastLexeme) {
    return skipElided(skipLexeme(lastLexeme));
}

unsigned int getLexemeLength(const char* lexeme) {
    return (unsigned int)(lexeme[0] == '\0' ? 1 : skipLexeme(lexeme) - lexeme);
}

bool isSameLexeme(const char* a, const char* b) {
    unsigned int lengthA = getLexemeLength(a);
    unsigned int lengthB = getLexemeLength(b);
    return lengthA == lengthB && strncmp(a, b, lengthA) == 0;
}
