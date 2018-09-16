#include <ctype.h>
#include "lib/tree.h"
#include "scan.h"
#include "ast.h"
#include "errors.h"
#include "lex.h"

static Location location = {1, 1};

static Token createToken(String lexeme) {
    Tag tag = newTag(lexeme, location);
    char head = lexeme.length > 0 ? lexeme.start[0] : '\0';
    if (head == '\n')
        location = newLocation(location.line + 1, 1);
    else
        location = newLocation(location.line, location.column + lexeme.length);

    if (head != '\0' && iscntrl(head) && !isspace(head))
        syntaxError("invalid character", newOperator(tag));
    if (isDelimiterCharacter(head) || isOperatorCharacter(head))
        return (Token){tag, PUNCTUATION};
    if (isdigit(head))
        return (Token){tag, NUMBER};
    if (head == '\'')
        return (Token){tag, CHARACTER};
    if (head == '\"')
        return (Token){tag, STRING};
    return (Token){tag, IDENTIFIER};
}

Token lex(const char* start) {
    return start == NULL ? (Token){0} : createToken(getNextLexeme(start));
}

const char* skip(Token token) {
    String lexeme = token.tag.lexeme;
    return lexeme.start[0] == 0 ? NULL : lexeme.start + lexeme.length;
}

Token newStartToken(void) {
    return (Token){newTag(EMPTY, newLocation(0, 0)), PUNCTUATION};
}
