#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "shared/lib/tag.h"
#include "parse/shared/token.h"

static bool isComment(char c) {return c == '#';}
static bool isQuote(char c) {return c == '"' || c == '\'';}
static bool isLineFeed(char c) {return c == '\n';}
static bool isInvalid(char c) {return c > 0 && iscntrl(c) && !isLineFeed(c);}
static bool isRepeatable(char c) {return c > 0 && strchr(" `.,;", c) != NULL;}

static bool isDelimiter(char c) {
    return c == '\0' || isInvalid(c) || strchr(" `.,;@#$()[]{}\"\n", c) != NULL;
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

static const char* skipQuote(const char* s, char end) {
    // assumption is that s points to the opening quotation mark
    for (s += 1; s[0] != '\0' && !isLineFeed(s[0]) && s[0] != end; s += 1)
        if (s[0] == '\\' && s[1] != '\0')
            s += 1;     // skip character following slash
    return s[0] == end ? s + 1 : s;
}

static const char* skipNumeric(const char* s) {
    s = skipUntil(s, isDelimiter);
    if (s[0] == '.' && isdigit(s[1]))
        s = skipUntil(++s, isDelimiter);
    return s;
}

static const char* skipLexeme(const char* s) {
    if (isComment(s[0])) return skipUntil(s, isLineFeed);
    if (isLineFeed(s[0])) return skipRepeatable(s + 1, ' ');
    if (isQuote(s[0])) return skipQuote(s, s[0]);
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
    if (isComment(tag.lexeme.start[0]) && tag.lexeme.start[1] == '*') {
        const char* file = skipRepeatable(&(tag.lexeme.start[2]), ' ');
        if (isLineFeed(file[0]) || file[0] == '\0')
            return newLocation(NULL, 0, 0);
        return newLocation(file, 1, 0);
    }
    Location loc = tag.location;
    if (isLineFeed(tag.lexeme.start[0]))
        return newLocation(loc.file, loc.line + 1, tag.lexeme.length);
    return newLocation(loc.file, loc.line, loc.column + tag.lexeme.length);
}

Token lex(Token token) {
    if (token.tag.lexeme.start[0] == '\0')
        return (Token){token.tag, END};
    Tag tag = newTag(getNextLexeme(token.tag), advanceLocation(token.tag));

    const char *start = tag.lexeme.start;
    const char *next = start + tag.lexeme.length;
    if (start[0] == ' ') return (Token){tag, SPACE};
    if (start[0] == '\n') return (Token){tag, next[0] == '\0' ||
        isLineFeed(next[0]) || isComment(next[0]) ? VSPACE : NEWLINE};
    if (start[0] == '\'') return (Token){tag, CHARACTER};
    if (start[0] == '\"') return (Token){tag, STRING};
    if (isNumeric(start)) return (Token){tag, NUMERIC};
    if (isComment(start[0])) return (Token){tag, COMMENT};
    if (isInvalid(start[0])) return (Token){tag, INVALID};
    return (Token){tag, SYMBOLIC};
}

Token newStartToken(const char* start) {
    return (Token){newTag(newString(start, 0), newLocation(0, 1, 1)), SYMBOLIC};
}
