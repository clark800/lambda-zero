#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "lib/tag.h"
#include "lex.h"

static bool isQuote(char c) {return c == '"' || c == '\'';}
static bool isNewline(char c) {return c == '\n';}
static bool isInvalid(char c) {return c > 0 && iscntrl(c) && !isNewline(c);}
static bool isRepeatable(char c) {return c > 0 && strchr(" `.,;", c) != NULL;}

static bool isDelimiter(char c) {
    return c == '\0' || isInvalid(c) || strchr(" `.,;@$()[]{}\"\n", c) != NULL;
}

static bool isLineComment(const char* s) {return s[0] == '-' && s[1] == '-';}
static bool isBlockComment(const char* s) {return s[0] == '{' && s[1] == '=';}

static bool isComment(const char* s) {
    return isLineComment(s) || isBlockComment(s);
}

static bool isNumeric(const char* s) {
    return (bool)isdigit(s[0] == '+' || s[0] == '-' ? s[1] : s[0]);
}

static const char* skipRepeatable(const char* s, char c) {
    for (; s[0] == c; ++s);
    return s;
}

static const char* skipUntil(const char* s, bool (*predicate)(char)) {
    for (; s[0] != '\0' && !predicate(s[0]); ++s);
    return s;
}

static const char* skipBlockComment(const char* s) {
    for (s += 2; s[0] != '\0' && (s[0] != '=' || s[1] != '}'); ++s);
    return s[0] == '\0' ? s : s + 2;
}

static const char* skipQuote(const char* s) {
    char quote = s[0];
    // assumption is that s points to the opening quotation mark
    for (s += 1; s[0] != '\0' && !isNewline(s[0]); s += 1) {
        if (s[0] == '\\' && s[1] != '\0')
            s += 1;     // skip character following slash
        else if (s[0] == quote)
            return s + 1;
    }
    return s;
}

static const char* skipNumeric(const char* s) {
    s = skipUntil(s, isDelimiter);
    if (s[0] == '.' && isdigit(s[1])) s = skipUntil(++s, isDelimiter);
    return s;
}

static const char* skipLexeme(const char* s) {
    if (isLineComment(s)) return skipUntil(s, isNewline);
    if (isBlockComment(s)) return skipBlockComment(s);
    if (isNewline(s[0])) return skipRepeatable(s + 1, ' ');
    if (isQuote(s[0])) return skipQuote(s);
    if (isNumeric(s)) return skipNumeric(s);
    if (!isDelimiter(s[0])) return skipUntil(s, isDelimiter);
    if (isRepeatable(s[0])) return skipRepeatable(s, s[0]);
    return s[0] == '\0' ? s : s + 1;
}

static String getNextLexeme(Tag tag) {
    const char* start = tag.lexeme.start + tag.lexeme.length;
    return newString(start, (unsigned int)(skipLexeme(start) - start));
}

static Location advanceLocation(Tag tag) {
    if (isLineComment(tag.lexeme.start) && tag.lexeme.start[2] == '*') {
        const char* file = skipRepeatable(&(tag.lexeme.start[3]), ' ');
        if (isNewline(file[0]) || file[0] == '\0')
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
        return (Token){token.tag, END};
    Tag tag = newTag(getNextLexeme(token.tag), advanceLocation(token.tag));

    char head = tag.lexeme.start[0];
    if (head == ' ') return (Token){tag, SPACE};
    if (head == '\n') return (Token){tag, NEWLINE};
    if (head == '\'') return (Token){tag, CHARACTER};
    if (head == '\"') return (Token){tag, STRING};
    if (isNumeric(tag.lexeme.start)) return (Token){tag, NUMERIC};
    if (isComment(tag.lexeme.start)) return (Token){tag, COMMENT};
    if (isInvalid(head)) return (Token){tag, INVALID};
    return (Token){tag, SYMBOLIC};
}

Token newStartToken(const char* start) {
    return (Token){newTag(newString(start, 0), newLocation(0, 1, 1)), SYMBOLIC};
}
