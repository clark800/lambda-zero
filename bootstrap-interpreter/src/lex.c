#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "lib/tree.h"
#include "scan.h"
#include "ast.h"
#include "errors.h"
#include "lex.h"

Location location;

bool isNameLexeme(char head) {
    return islower(head) || head == '_' || head == '\'' || head == '\"';
}

bool isOperatorLexeme(char head) {
    return isDelimiterCharacter(head) || isOperatorCharacter(head) ||
        isSpaceCharacter(head);
}

bool isIntegerLexeme(String lexeme) {
    for (unsigned int i = 0; i < lexeme.length; i++)
        if (!isdigit(lexeme.start[i]))
            return false;
    return lexeme.length > 0;
}

long long parseInteger(Tag tag) {
    errno = 0;
    long long result = strtoll(tag.lexeme.start, NULL, 10);
    lexerErrorIf((result == LLONG_MIN || result == LLONG_MAX) &&
        errno == ERANGE, tag, "magnitude of integer is too large");
    return result;
}

Node* createToken(String lexeme) {
    Tag tag = newTag(lexeme, location);
    char head = lexeme.length > 0 ? lexeme.start[0] : '\0';
    if (head == '\n')
        location = newLocation(location.line + 1, 1);
    else
        location = newLocation(location.line, location.column + lexeme.length);

    lexerErrorIf(isupper(head), tag, "names can't start with uppercase");

    if (isIntegerLexeme(lexeme))
        return newInteger(tag, parseInteger(tag));
    if (isNameLexeme(head))
        return newName(tag);
    if (isOperatorLexeme(head))
        return newOperator(tag);

    lexerErrorIf(true, tag, "invalid token");
    return NULL;
}

Hold* getFirstToken(const char* sourceCode) {
    location = newLocation(1, 1);
    return hold(createToken(getFirstLexeme(sourceCode)));
}

Hold* getNextToken(Hold* token) {
    return hold(createToken(getNextLexeme(getLexeme(getNode(token)))));
}
