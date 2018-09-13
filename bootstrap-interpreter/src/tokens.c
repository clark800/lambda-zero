#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "lex.h"
#include "errors.h"
#include "operators.h"

static void errorIf(bool condition, const char* message, Tag tag) {
    if (condition)
        syntaxError(message, newName(tag));
}

static bool isIntegerLexeme(String lexeme) {
    for (unsigned int i = 0; i < lexeme.length; ++i)
        if (!isdigit(lexeme.start[i]))
            return false;
    return lexeme.length > 0;
}

static Node* parseInteger(Tag tag) {
    errorIf(!isIntegerLexeme(tag.lexeme), "invalid token", tag);
    errno = 0;
    long long value = strtoll(tag.lexeme.start, NULL, 10);
    errorIf((value == LLONG_MIN || value == LLONG_MAX) &&
        errno == ERANGE, "magnitude of integer is too large", tag);
    return newInteger(tag, value);
}

static const char* skipQuoteCharacter(const char* start) {
    return start[0] == '\\' ? start + 2 : start + 1;
}

static char decodeCharacter(const char* start, Tag tag) {
    errorIf(start[0] <= 0, "invalid character in", tag);
    if (start[0] != '\\')
        return start[0];
    switch (start[1]) {
        case '0': return '\0';
        case 't': return '\t';
        case 'r': return '\r';
        case 'n': return '\n';
        case '\n': return '\n';
        case '\\': return '\\';
        case '\"': return '\"';
        case '\'': return '\'';
        default: syntaxError("invalid escape sequence in", newName(tag));
    }
    return 0;
}

static Node* parseCharacterLiteral(Tag tag) {
    char quote = tag.lexeme.start[0];
    const char* end = tag.lexeme.start + tag.lexeme.length - 1;
    errorIf(end[0] != quote, "missing end quote for", tag);
    const char* skip = skipQuoteCharacter(tag.lexeme.start + 1);
    errorIf(skip != end, "invalid character literal", tag);
    return newInteger(tag, decodeCharacter(tag.lexeme.start + 1, tag));
}

static Node* buildStringLiteral(Tag tag, const char* start) {
    char c = start[0];
    errorIf(c == '\n' || c == 0, "missing end quote for", tag);
    return c == tag.lexeme.start[0] ? newNil(tag) :
        prepend(tag, newInteger(tag, decodeCharacter(start, tag)),
        buildStringLiteral(tag, skipQuoteCharacter(start)));
}

static Node* parseStringLiteral(Tag tag) {
    return buildStringLiteral(tag, tag.lexeme.start + 1);
}

static Node* parseSymbol(Tag tag) {
    if (isThisString(tag.lexeme, "error")) {
        Node* print = newPrinter(tag);
        Node* exit = newBuiltin(renameTag(tag, "exit"), EXIT);
        Node* error = newBuiltin(renameTag(tag, "error"), ERROR);
        Node* blank = newBlankReference(tag, 1);
        // error(message) ~> (_ -> exit(print(error(_))))(message)
        return newLambda(tag, newBlank(tag),
            newApplication(tag, exit, newApplication(tag, print,
            newApplication(tag, error, blank))));
    }
    return newName(tag);
}

Node* parseToken(Token token, Stack* stack) {
    switch (token.type) {
        case NUMBER: return parseInteger(token.tag);
        case STRING: return parseStringLiteral(token.tag);
        case CHARACTER: return parseCharacterLiteral(token.tag);
        case PUNCTUATION: return parseOperator(token.tag, stack);
        case IDENTIFIER: return parseSymbol(token.tag);
        default: assert(false); return NULL;
    }
}
