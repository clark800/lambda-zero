#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "lib/tree.h"
#include "ast.h"
#include "errors.h"

static bool isIntegerLexeme(String lexeme) {
    for (unsigned int i = 0; i < lexeme.length; i++)
        if (!isdigit(lexeme.start[i]))
            return false;
    return lexeme.length > 0;
}

static Node* parseInteger(Node* token) {
    Tag tag = getTag(token);
    syntaxErrorIf(!isIntegerLexeme(tag.lexeme), "invalid token", token);
    errno = 0;
    long long value = strtoll(tag.lexeme.start, NULL, 10);
    syntaxErrorIf((value == LLONG_MIN || value == LLONG_MAX) &&
        errno == ERANGE, "magnitude of integer is too large", token);
    return newInteger(tag, value);
}

static const char* skipQuoteCharacter(const char* start) {
    return start[0] == '\\' ? start + 2 : start + 1;
}

static char decodeCharacter(const char* start, Node* token) {
    syntaxErrorIf(start[0] <= 0, "invalid character in", token);
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
        default: syntaxError("invalid escape sequence in", token);
    }
    return 0;
}

static Node* parseCharacterLiteral(Node* token) {
    Tag tag = getTag(token);
    char quote = tag.lexeme.start[0];
    const char* end = tag.lexeme.start + tag.lexeme.length - 1;
    syntaxErrorIf(end[0] != quote, "missing end quote for", token);
    const char* skip = skipQuoteCharacter(tag.lexeme.start + 1);
    syntaxErrorIf(skip != end, "invalid character literal", token);
    return newInteger(tag, decodeCharacter(tag.lexeme.start + 1, token));
}

static Node* buildStringLiteral(Node* token, const char* start) {
    char c = start[0];
    Tag tag = getTag(token);
    syntaxErrorIf(c == '\n' || c == 0, "missing end quote for", token);
    if (c == tag.lexeme.start[0])
        return newNil(tag);
    return prepend(tag, newInteger(tag, decodeCharacter(start, token)),
        buildStringLiteral(token, skipQuoteCharacter(start)));
}

static Node* parseStringLiteral(Node* token) {
    return buildStringLiteral(token, getTag(token).lexeme.start + 1);
}

static Node* parseSymbol(Node* token) {
    if (isThisToken(token, "error")) {
        Tag tag = getTag(token);
        Node* print = newPrinter(tag);
        Node* exit = newBuiltin(renameTag(tag, "exit"), EXIT);
        Node* error = newBuiltin(renameTag(tag, "error"), ERROR);
        Node* blank = newBlankReference(tag, 1);
        // error(message) ~> (_ -> exit(print(error(_))))(message)
        return newLambda(tag, newBlank(tag),
            newApplication(tag, exit, newApplication(tag, print,
            newApplication(tag, error, blank))));
    }
    return newName(getTag(token));
}

Node* parseOperand(Node* token) {
    switch ((OperandType)getValue(token)) {
        case NUMERIC: return parseInteger(token);
        case STRING: return parseStringLiteral(token);
        case CHARACTER: return parseCharacterLiteral(token);
        default: return parseSymbol(token);
    }
}
