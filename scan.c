#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "scan.h"

static inline bool isSpaceCharacter(char c) {
    return c == ' ';
}

static inline bool isComment(const char* s) {
    return s[0] == '-' && s[1] == '-';
}

static inline bool isEscapedNewline(const char* s) {
    return s[0] == '\\' && s[1] == '\n';
}

static inline bool isNotNewlineCharacter(char c) {
    return c != '\n';
}

static inline const char* skipWhile(const char* s, bool (*predicate)(char)) {
    while (s[0] != '\0' && predicate(s[0]))
        s++;
    return s;
}

static inline const char* skipSpaces(const char* s) {
    return skipWhile(s, isSpaceCharacter);
}

static inline const char* skipToNewline(const char* s) {
    return skipWhile(s, isNotNewlineCharacter);
}

static inline const char* skipN(const char* s, unsigned int n) {
    return &(s[n]);
}

static inline const char* skipQuote(const char* s) {
    char quote = s[0];
    // assumption is that start points to the opening quotation mark
    for (s += 1; s[0] != '\0' && s[0] != '\n'; s += 1) {
        if (s[0] == '\\' && s[1] != '\0')
            s += 1;     // skip character following slash
        else if (s[0] == quote)
            return s + 1;
    }
    return s;
}

static inline const char* skipLexeme(const char* lexeme) {
    assert(lexeme[0] != '\0' && lexeme[0] != ' ');
    if (isQuoteCharacter(lexeme[0]))
        return skipQuote(lexeme);
    if (isOperandCharacter(lexeme[0]))
        return skipWhile(lexeme, isOperandCharacter);
    if (isOperatorCharacter(lexeme[0]))
        return skipWhile(lexeme, isOperatorCharacter);
    return skipN(lexeme, 1);    // delimiter
}

static inline const char* skipElided(const char* s) {
    while (isComment(s) || isEscapedNewline(s))
        s = isComment(s) ? skipToNewline(s) : skipSpaces(skipN(s, 2));
    return s;
}

const char* getFirstLexeme(const char* input) {
    return skipElided(skipSpaces(input));
}

const char* getNextLexeme(const char* lastLexeme) {
    return skipElided(skipSpaces(skipLexeme(lastLexeme)));
}

unsigned int getLexemeLength(const char* lexeme) {
    return (unsigned int)(lexeme[0] == '\0' ? 1 : skipLexeme(lexeme) - lexeme);
}

bool isSameLexeme(const char* a, const char* b) {
    unsigned int lengthA = getLexemeLength(a);
    unsigned int lengthB = getLexemeLength(b);
    return lengthA == lengthB && strncmp(a, b, lengthA) == 0;
}
