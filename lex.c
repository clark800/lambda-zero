#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "lib/tree.h"
#include "scan.h"
#include "ast.h"
#include "errors.h"
#include "lex.h"

bool isNameLexeme(char head) {
    // note: the quote case is only for internal code
    return islower(head) || head == '_' || head == '\'';
}

bool isOperatorLexeme(char head) {
    return isDelimiterCharacter(head) || isOperatorCharacter(head) ||
        isSpaceCharacter(head);
}

bool isIntegerLexeme(String lexeme) {
    for (unsigned int i = 0; i < lexeme.length; i++)
        if (!isdigit(lexeme.start[i]))
            return false;
    return true;
}

const char* skipQuoteCharacter(const char* start) {
    return start[0] == '\\' ? start + 2 : start + 1;
}

char decodeCharacter(const char* start, String lexeme) {
    lexerErrorIf(start[0] <= 0, lexeme, "illegal character in");
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
        default: lexerErrorIf(true, lexeme, "invalid escape sequence in");
    }
    return 0;
}

Node* newCharacterLiteral(String lexeme) {
    char quote = lexeme.start[0];
    const char* end = lexeme.start + lexeme.length - 1;
    lexerErrorIf(end[0] != quote, lexeme, "missing end quote for");
    const char* skip = skipQuoteCharacter(lexeme.start + 1);
    lexerErrorIf(skip != end, lexeme, "invalid character literal");
    return newInteger(lexeme, decodeCharacter(lexeme.start + 1, lexeme));
}

Node* buildStringLiteral(String lexeme, const char* start) {
    char c = start[0];
    lexerErrorIf(c == '\n' || c == 0, lexeme, "missing end quote for");
    if (c == lexeme.start[0])
        return newNil(lexeme);
    return prepend(lexeme, newInteger(lexeme, decodeCharacter(start,
        lexeme)), buildStringLiteral(lexeme, skipQuoteCharacter(start)));
}

Node* newStringLiteral(String lexeme) {
    return buildStringLiteral(lexeme, lexeme.start + 1);
}

long long parseInteger(String lexeme) {
    errno = 0;
    long long result = strtoll(lexeme.start, NULL, 10);
    lexerErrorIf((result == LLONG_MIN || result == LLONG_MAX) &&
        errno == ERANGE, lexeme, "magnitude of integer is too large");
    return result;
}

Node* createToken(String lexeme) {
    char head = lexeme.start[0];
    lexerErrorIf(isupper(head), lexeme, "names can't start with uppercase");
    if (head == '"')
        return newStringLiteral(lexeme);
    // single quoted operands are internal names while parsing internal code
    if (head == '\'')
        return newCharacterLiteral(lexeme);

    if (isIntegerLexeme(lexeme))
        return newInteger(lexeme, parseInteger(lexeme));
    if (isNameLexeme(head))
        return newName(lexeme);
    if (isOperatorLexeme(head))
        return newOperator(lexeme);

    lexerErrorIf(true, lexeme, "invalid token");
    return NULL;
}

Hold* getFirstToken(const char* sourceCode) {
    return hold(createToken(getFirstLexeme(sourceCode)));
}

Hold* getNextToken(Hold* token) {
    return hold(createToken(getNextLexeme(getLexeme(getNode(token)))));
}
