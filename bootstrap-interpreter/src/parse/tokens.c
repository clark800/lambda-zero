#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "tree.h"
#include "lex/token.h"
#include "opp/errors.h"
#include "opp/operator.h"
#include "ast.h"
#include "brackets.h"  // Nil and prepend
#include "tokens.h"

static bool isNumberLexeme(String lexeme) {
    for (unsigned int i = 0; i < lexeme.length; ++i)
        if (!isdigit(lexeme.start[i]))
            return false;
    return lexeme.length > 0;
}

static Node* parseNumber(Tag tag) {
    syntaxErrorIf(!isNumberLexeme(tag.lexeme), "invalid token", tag);
    errno = 0;
    long long value = strtoll(tag.lexeme.start, NULL, 10);
    if ((value == LLONG_MIN || value == LLONG_MAX) && errno == ERANGE)
       syntaxError("magnitude of numeral is too large", tag);
    return Number(tag, value);
}

static const char* skipQuoteCharacter(const char* start) {
    return start[0] == '\\' ? start + 2 : start + 1;
}

static unsigned char decodeCharacter(const char* start, Tag tag) {
    if (start[0] != '\\')
        return (unsigned char)start[0];
    switch (start[1]) {
        case '0': return '\0';
        case 't': return '\t';
        case 'r': return '\r';
        case 'n': return '\n';
        case '\n': return '\n';
        case '\\': return '\\';
        case '\"': return '\"';
        case '\'': return '\'';
        default: syntaxError("invalid escape sequence in", tag);
    }
    return '\0';
}

static Node* parseCharacterLiteral(Tag tag) {
    char quote = tag.lexeme.start[0];
    const char* end = tag.lexeme.start + tag.lexeme.length - 1;
    syntaxErrorIf(end[0] != quote, "missing end quote for", tag);
    const char* skip = skipQuoteCharacter(tag.lexeme.start + 1);
    syntaxErrorIf(skip != end, "invalid character literal", tag);
    return Number(tag, decodeCharacter(tag.lexeme.start + 1, tag));
}

static Node* buildStringLiteral(Tag tag, const char* start) {
    char c = start[0];
    syntaxErrorIf(c == '\n' || c == '\0', "missing end quote for", tag);
    return c == tag.lexeme.start[0] ? Nil(tag) :
        prepend(tag, Number(tag, decodeCharacter(start, tag)),
        buildStringLiteral(tag, skipQuoteCharacter(start)));
}

static Node* parseStringLiteral(Tag tag) {
    return buildStringLiteral(tag, tag.lexeme.start + 1);
}

Node* parseSymbol(Tag tag, long long subprecedence) {
    Node* operator = parseOperator(tag, subprecedence);
    return operator == NULL ? Name(tag) : operator;
}

Node* parseToken(Token token) {
    switch (token.type) {
        case NUMERIC: return parseNumber(token.tag);
        case STRING: return parseStringLiteral(token.tag);
        case CHARACTER: return parseCharacterLiteral(token.tag);
        case NEWLINE: return parseSymbol(renameTag(token.tag, "\n"),
            (long long)(token.tag.lexeme.length - 1));
        case INVALID: syntaxError("invalid character", token.tag); return NULL;
        default: return parseSymbol(token.tag, 0);
    }
}
