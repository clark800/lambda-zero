#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "lib/tag.h"
#include "scan.h"

bool isSpaceCharacter(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

bool isQuoteCharacter(char c) {
    return c == '"' || c == '\'';
}

bool isDelimiterCharacter(char c) {
    return c == '\0' || strchr(" \n,;`()[]{}", c) != NULL;
}

bool isOperandCharacter(char c) {
    // check c > 0 to ensure it is ASCII
    return c > 0 && (isalnum(c) || c == '\'' || c == '_');
}

bool isOperatorCharacter(char c) {
    // check c > 0 to ensure it is ASCII
    return c > 0 && !isDelimiterCharacter(c) && !isOperandCharacter(c)
        && !isQuoteCharacter(c) && strchr("{};@$", c) == NULL;
}

static inline bool isLineComment(const char* s) {
    return s[0] == '/' && s[1] == '/';
}

static inline bool isBlockComment(const char* s) {
    return s[0] == '/' && s[1] == '*';
}

static inline bool isNotNewline(char c) {
    return c != '\n';
}

static inline const char* skipWhile(const char* s, bool (*predicate)(char)) {
    while (s[0] != '\0' && predicate(s[0]))
        s++;
    return s;
}

static inline const char* skipBlockComment(const char* s) {
    while (s[0] != '\0' && (s[0] != '*' || s[1] != '/'))
        s++;
    return s[0] == '\0' ? s : s + 2;
}

static inline const char* skipComments(const char* s) {
    while (isLineComment(s) || isBlockComment(s))
        s = isLineComment(s) ? skipWhile(s, isNotNewline) : skipBlockComment(s);
    return s;
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

static inline const char* skipLexeme(const char* s) {
    if (isSpaceCharacter(s[0]))
        return skipWhile(s, isSpaceCharacter);
    if (isQuoteCharacter(s[0]))
        return skipQuote(s);
    if (isOperandCharacter(s[0]))
        return skipWhile(s, isOperandCharacter);
    if (isOperatorCharacter(s[0]))
        return skipWhile(s, isOperatorCharacter);
    return s[0] == '\0' ? s : s + 1;    // delimiter or illegal character
}

static inline String newLexeme(const char* start) {
    return newString(start, (unsigned int)(skipLexeme(start) - start));
}

String getFirstLexeme(const char* input) {
    return newLexeme(skipComments(input));
}

String getNextLexeme(String lastLexeme) {
    return newLexeme(skipComments(&(lastLexeme.start[lastLexeme.length])));
}
