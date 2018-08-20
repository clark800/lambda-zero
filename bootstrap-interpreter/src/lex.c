#include <ctype.h>
#include "lib/tree.h"
#include "scan.h"
#include "ast.h"
#include "lex.h"

static Location location;

static Node* createToken(String lexeme) {
    Tag tag = newTag(lexeme, location);
    char head = lexeme.length > 0 ? lexeme.start[0] : '\0';
    if (head == '\n')
        location = newLocation(location.line + 1, 1);
    else
        location = newLocation(location.line, location.column + lexeme.length);

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
    return hold(createToken(getFirstLexeme(sourceCode)));
}

Hold* getNextToken(Hold* token) {
    return hold(createToken(getNextLexeme(getLexeme(getNode(token)))));
}
