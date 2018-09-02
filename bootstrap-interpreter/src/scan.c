#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "lib/tag.h"

bool isSpaceCharacter(char c) {return c == ' ' || c == '\t' || c == '\r';}
static bool isCommaCharacter(char c) {return c == ',';}
static bool isQuoteCharacter(char c) {return c == '"' || c == '\'';}
static bool isLineComment(const char* s) {return s[0] == '/' && s[1] == '/';}
static bool isBlockComment(const char* s) {return s[0] == '/' && s[1] == '*';}
static bool isNotNewline(char c) {return c != '\n';}

bool isDelimiterCharacter(char c) {
    return c == '\0' || strchr(" \t\r\n;()[]{}", c) != NULL;
}

static bool isOperandCharacter(char c) {
    // check c > 0 to ensure it is ASCII
    return c > 0 && (isalnum(c) || c == '\'' || c == '_');
}

bool isOperatorCharacter(char c) {
    // check c > 0 to ensure it is ASCII
    return c > 0 && !isDelimiterCharacter(c) && !isOperandCharacter(c)
        && !isQuoteCharacter(c);
}

static const char* skipWhile(const char* s, bool (*predicate)(char)) {
    for (; s[0] != '\0' && predicate(s[0]); ++s);
    return s;
}

static const char* skipBlockComment(const char* s) {
    for (; s[0] != '\0' && (s[0] != '*' || s[1] != '/'); ++s);
    return s[0] == '\0' ? s : s + 2;
}

static const char* skipComments(const char* s) {
    while (isLineComment(s) || isBlockComment(s))
        s = isLineComment(s) ? skipWhile(s, isNotNewline) : skipBlockComment(s);
    return s;
}

static const char* skipQuote(const char* s) {
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

static const char* skipLexeme(const char* s) {
    if (isSpaceCharacter(s[0]))
        return skipWhile(s, isSpaceCharacter);
    if (isQuoteCharacter(s[0]))
        return skipQuote(s);
    if (isCommaCharacter(s[0]))
        return skipWhile(s, isCommaCharacter);
    if (isOperandCharacter(s[0]))
        return skipWhile(s, isOperandCharacter);
    if (isOperatorCharacter(s[0]))
        return skipWhile(s, isOperatorCharacter);
    return s[0] == '\0' ? s : s + 1;    // delimiter or invalid character
}

String getNextLexeme(const char* start) {
    start = skipComments(start);
    return newString(start, (unsigned int)(skipLexeme(start) - start));
}
