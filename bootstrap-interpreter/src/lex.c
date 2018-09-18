#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "lib/tag.h"
#include "lex.h"

static bool isQuote(char c) {return c == '"' || c == '\'';}
static bool isNotNewline(char c) {return c != '\n';}
static bool isLineComment(const char* s) {return s[0] == '/' && s[1] == '/';}
static bool isBlockComment(const char* s) {return s[0] == '/' && s[1] == '*';}
static bool isSpace(char c) {return c > 0 && isspace(c) && c != '\n';}

static bool isInvalid(char c) {
    return c < 0 || (iscntrl(c) && !isspace(c) && c != '\0');
}

static bool isOperandCharacter(char c) {
    // check c > 0 to ensure it is ASCII
    return c > 0 && (isalnum(c) || c == '\'' || c == '_');
}

static bool isDelimiter(char c) {
    return c == '\0' || isSpace(c) || strchr("\n,;()[]{}", c) != NULL;
}

static bool isOperatorCharacter(char c) {
    // check c > 0 to ensure it is ASCII
    return c > 0 && !isDelimiter(c) && !isOperandCharacter(c)
        && !isQuote(c) && !isInvalid(c);
}

static const char* skipWhile(const char* s, bool (*predicate)(char)) {
    for (; s[0] != '\0' && predicate(s[0]); ++s);
    return s;
}

static const char* skipBlockComment(const char* s) {
    for (; s[0] != '\0' && (s[0] != '*' || s[1] != '/'); ++s);
    return s[0] == '\0' ? s : s + 2;
}

static const char* skipQuote(const char* s) {
    char quote = s[0];
    // assumption is that s points to the opening quotation mark
    for (s += 1; s[0] != '\0' && s[0] != '\n'; s += 1) {
        if (s[0] == '\\' && s[1] != '\0')
            s += 1;     // skip character following slash
        else if (s[0] == quote)
            return s + 1;
    }
    return s;
}

static const char* skipLexeme(const char* s) {
    if (isLineComment(s))
        return skipWhile(s, isNotNewline);
    if (isBlockComment(s))
        return skipBlockComment(s);
    if (isSpace(s[0]))
        return skipWhile(s, isSpace);
    if (isQuote(s[0]))
        return skipQuote(s);
    if (isOperandCharacter(s[0]))
        return skipWhile(s, isOperandCharacter);
    if (isOperatorCharacter(s[0]))
        return skipWhile(s, isOperatorCharacter);
    return s[0] == '\0' ? s : s + 1;    // delimiter or invalid character
}

static String getNextLexeme(Tag tag) {
    const char* start = tag.lexeme.start + tag.lexeme.length;
    return newString(start, (unsigned int)(skipLexeme(start) - start));
}

static Location advanceLocation(Tag tag) {
    if (isLineComment(tag.lexeme.start) && tag.lexeme.start[2] == '$') {
        const char* file = skipWhile(&(tag.lexeme.start[3]), isSpace);
        if (file[0] == '\n' || file[0] == '\0')
            return newLocation(NULL, 0, 0);
        return newLocation(file, 1, 0);
    }
    // must scan the whole lexeme for newlines because of block comments
    for (unsigned int i = 0; i < tag.lexeme.length; ++i)
        tag.location = (tag.lexeme.start[i] == '\n') ?
            newLocation(tag.location.file, tag.location.line + 1, 1) :
            newLocation(tag.location.file,
                tag.location.line, tag.location.column + 1);
    return tag.location;
}

Token lex(Token token) {
    if (token.tag.lexeme.start[0] == '\0')
        return (Token){newTag(EMPTY, token.tag.location), END};
    Tag tag = newTag(getNextLexeme(token.tag), advanceLocation(token.tag));

    char head = tag.lexeme.start[0];
    if (isInvalid(head))
        return (Token){tag, INVALID};
    if (isSpace(head))
        return (Token){tag, SPACE};
    if (isLineComment(tag.lexeme.start) || isBlockComment(tag.lexeme.start))
        return (Token){tag, COMMENT};
    if (isdigit(head))
        return (Token){tag, NUMERIC};
    if (head == '\'')
        return (Token){tag, CHARACTER};
    if (head == '\"')
        return (Token){tag, STRING};
    return (Token){tag, SYMBOLIC};
}

Token newStartToken(const char* start) {
    return (Token){newTag(newString(start, 0), newLocation(0, 1, 1)), SYMBOLIC};
}
