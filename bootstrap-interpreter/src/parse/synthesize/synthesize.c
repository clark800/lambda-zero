#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "shared/lib/tree.h"
#include "shared/lib/stack.h"
#include "parse/shared/errors.h"
#include "parse/shared/token.h"
#include "parse/shared/ast.h"
#include "parse/shared/debug.h"
#include "symbols.h"
#include "syntax.h"

int DEBUG = 0;

static bool isNumberLexeme(String lexeme) {
    for (unsigned int i = 0; i < lexeme.length; ++i)
        if (!isdigit(lexeme.start[i]))
            return false;
    return lexeme.length > 0;
}

static Node* parseNumber(Tag tag) {
    tokenErrorIf(!isNumberLexeme(tag.lexeme), "invalid token", tag);
    errno = 0;
    long long value = strtoll(tag.lexeme.start, NULL, 10);
    tokenErrorIf((value == LLONG_MIN || value == LLONG_MAX) &&
        errno == ERANGE, "magnitude of numeral is too large", tag);
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
        default: tokenErrorIf(true, "invalid escape sequence in", tag);
    }
    return 0;
}

static Node* parseCharacterLiteral(Tag tag) {
    char quote = tag.lexeme.start[0];
    const char* end = tag.lexeme.start + tag.lexeme.length - 1;
    tokenErrorIf(end[0] != quote, "missing end quote for", tag);
    const char* skip = skipQuoteCharacter(tag.lexeme.start + 1);
    tokenErrorIf(skip != end, "invalid character literal", tag);
    return Number(tag, decodeCharacter(tag.lexeme.start + 1, tag));
}

static Node* buildStringLiteral(Tag tag, const char* start) {
    char c = start[0];
    tokenErrorIf(c == '\n' || c == 0, "missing end quote for", tag);
    return c == tag.lexeme.start[0] ? Nil(tag) :
        prepend(tag, Number(tag, decodeCharacter(start, tag)),
        buildStringLiteral(tag, skipQuoteCharacter(start)));
}

static Node* parseStringLiteral(Tag tag) {
    return buildStringLiteral(tag, tag.lexeme.start + 1);
}

static Node* parseToken(Token token) {
    switch (token.type) {
        case NUMERIC: return parseNumber(token.tag);
        case STRING: return parseStringLiteral(token.tag);
        case CHARACTER: return parseCharacterLiteral(token.tag);
        case INVALID: tokenErrorIf(true, "invalid character", token.tag);
            return NULL;
        case SPACE: return parseSymbol(renameTag(token.tag, " "), 0);
        case NEWLINE: return parseSymbol(renameTag(token.tag, "\n"),
            token.tag.lexeme.length - 1);
        default: return parseSymbol(token.tag, 0);
    }
}

Hold* synthesize(Token (*lexer)(Token), Token start) {
    initSymbols();
    Stack* stack = newStack();
    push(stack, parseToken(start));
    for (Token token = lexer(start); token.type != END; token = lexer(token)) {
        debugParseState(token.tag, stack, DEBUG >= 2);
        if (token.type != COMMENT && token.type != VSPACE) {
            Node* node = parseToken(token);
            shift(stack, node);
            release(hold(node));
        }
    }
    Hold* ast = pop(stack);
    syntaxErrorIf(isEOF(getNode(ast)), "no input", getNode(ast));
    deleteStack(stack);
    return ast;
}

