#include "lib/tree.h"
#include "ast.h"
#include "errors.h"

const char* skipQuoteCharacter(const char* start) {
    return start[0] == '\\' ? start + 2 : start + 1;
}

char decodeCharacter(const char* start, Tag tag) {
    lexerErrorIf(start[0] <= 0, tag, "illegal character in");
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
        default: lexerErrorIf(true, tag, "invalid escape sequence in");
    }
    return 0;
}

Node* newCharacterLiteral(Tag tag) {
    char quote = tag.lexeme.start[0];
    const char* end = tag.lexeme.start + tag.lexeme.length - 1;
    lexerErrorIf(end[0] != quote, tag, "missing end quote for");
    const char* skip = skipQuoteCharacter(tag.lexeme.start + 1);
    lexerErrorIf(skip != end, tag, "invalid character literal");
    return newInteger(tag, decodeCharacter(tag.lexeme.start + 1, tag));
}

Node* buildStringLiteral(Tag tag, const char* start) {
    char c = start[0];
    lexerErrorIf(c == '\n' || c == 0, tag, "missing end quote for");
    if (c == tag.lexeme.start[0])
        return newNil(tag);
    return prepend(tag, newInteger(tag, decodeCharacter(start,
        tag)), buildStringLiteral(tag, skipQuoteCharacter(start)));
}

Node* newStringLiteral(Tag tag) {
    return buildStringLiteral(tag, tag.lexeme.start + 1);
}
