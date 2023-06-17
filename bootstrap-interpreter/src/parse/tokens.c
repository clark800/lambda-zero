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

static bool isNumberLexeme(Lexeme lexeme) {
    for (unsigned int i = 0; i < lexeme.length; ++i)
        if (!isdigit(lexeme.start[i]))
            return false;
    return lexeme.length > 0;
}

static Node* parseNumber(Lexeme lexeme) {
    Tag tag = newTag(lexeme, NOFIX);
    syntaxErrorIf(!isNumberLexeme(lexeme), "invalid token", tag);
    errno = 0;
    long long value = strtoll(lexeme.start, NULL, 10);
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

static Node* parseCharacterLiteral(Lexeme lexeme) {
    Tag tag = newTag(lexeme, NOFIX);
    const char* start = lexeme.start;
    const char* end = start + lexeme.length - 1;
    syntaxErrorIf(end[0] != start[0], "missing end quote for", tag);
    long long n = 0, c = 0;
    for (const char* p = ++start; p < end; p = skipQuoteCharacter(p), c++)
        n = n * 256 + decodeCharacter(p, tag);
    syntaxErrorIf(c == 0 || c > 4, "invalid character literal", tag);
    return Number(tag, n);
}

static Node* buildStringLiteral(Tag tag, const char* start) {
    char c = start[0];
    syntaxErrorIf(c == '\n' || c == '\0', "missing end quote for", tag);
    return c == getLexeme(tag).start[0] ? Nil(tag) :
        prepend(tag, Number(tag, decodeCharacter(start, tag)),
        buildStringLiteral(tag, skipQuoteCharacter(start)));
}

static Node* parseStringLiteral(Lexeme lexeme) {
    Hold* name = hold(Name(newTag(lexeme, NOFIX)));
    Node* node = buildStringLiteral(getTag(name), lexeme.start + 1);
    release(name);
    return node;
}

Node* parseSymbol(Lexeme lexeme, long long subprecedence) {
    Node* operator = parseOperator(lexeme, subprecedence);
    return operator == NULL ? Name(newTag(lexeme, NOFIX)) : operator;
}

Node* parseToken(Token token) {
    Lexeme lexeme = token.lexeme;
    switch (token.type) {
        case NUMERIC: return parseNumber(lexeme);
        case STRING: return parseStringLiteral(lexeme);
        case CHARACTER: return parseCharacterLiteral(lexeme);
        case NEWLINE: return parseSymbol(newLiteralLexeme(
            "\n", lexeme.location), (long long)(lexeme.length - 1));
        case INVALID: syntaxError("invalid character", newTag(lexeme, NOFIX));
        default: return parseSymbol(lexeme, 0);
    }
}
