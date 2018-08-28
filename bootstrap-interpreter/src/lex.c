#include <ctype.h>
#include "lib/tree.h"
#include "scan.h"
#include "ast.h"
#include "errors.h"
#include "lex.h"

static Location location = {1, 1};

static Node* createToken(String lexeme) {
    Tag tag = newTag(lexeme, location);
    char head = lexeme.length > 0 ? lexeme.start[0] : '\0';
    if (head == '\n')
        location = newLocation(location.line + 1, 1);
    else
        location = newLocation(location.line, location.column + lexeme.length);

    if (head != '\0' && iscntrl(head) && !isspace(head))
        syntaxError("invalid character", newOperator(tag));
    if (isDelimiterCharacter(head) || isOperatorCharacter(head))
        return newOperator(tag);
    if (isdigit(head))
        return newOperand(tag, NUMERIC);
    if (head == '\'')
        return newOperand(tag, CHARACTER);
    if (head == '\"')
        return newOperand(tag, STRING);
    return newOperand(tag, NAME);
}

Hold* getFirstToken(const char* sourceCode) {
    location = newLocation(1, 1);
    return hold(createToken(getNextLexeme(sourceCode)));
}

Hold* getNextToken(Hold* token) {
    String lexeme = getLexeme(getNode(token));
    return hold(createToken(getNextLexeme(lexeme.start + lexeme.length)));
}
