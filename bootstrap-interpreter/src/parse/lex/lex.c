#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h> // exit
#include "lexeme.h"
#include "token.h"
#include "lex.h"

static bool isComment(char c) {return c == '#';}
static bool isQuote(char c) {return c == '"' || c == '\'';}
static bool isLineFeed(char c) {return c == '\n';}
static bool isInvalid(char c) {return c > 0 && iscntrl(c) && !isLineFeed(c);}
static bool isRepeatable(char c) {return c > 0 && strchr(" `.,;", c) != NULL;}

static void lexErrorIf(bool condition, const char* message, Location loc) {
    if (condition) {
        fputs("Lexical error: ", stderr);
        fputs(message, stderr);
        fputs(" at ", stderr);
        printLocation(loc, stderr);
        fputs("\n", stderr);
        exit(1);
    }
}

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

static Location advance(Lexeme lexeme) {
    if (isComment(lexeme.start[0]) && lexeme.start[1] == '@') {
        const char* filename = skipRepeatable(&(lexeme.start[2]), ' ');
        if (isLineFeed(filename[0]) || filename[0] == '\0')
            return newLocation(0, 0, 0);
        unsigned short file = newFilename(filename);
        lexErrorIf(file == 0, "too many files", lexeme.location);
        return newLocation(file, 1, 0);
    }
    Location loc = lexeme.location;
    if (isLineFeed(lexeme.start[0])) {
        lexErrorIf(loc.line >= MAX_LINE, "too many lines in file", loc);
        return newLocation(loc.file, loc.line + 1, lexeme.length);
    }
    unsigned long column = (unsigned long)(loc.column + lexeme.length);
    lexErrorIf(column > MAX_COLUMN, "column too wide", loc);
    return newLocation(loc.file, loc.line, (unsigned short)column);
}

static Lexeme getNextLexeme(Lexeme lexeme) {
    const char* start = lexeme.start + lexeme.length;
    long length = start[0] == '\0' ? 1 : skipLexeme(start) - start;
    lexErrorIf(length > MAX_LEXEME_LENGTH, "lexeme too long", lexeme.location);
    return newLexeme(start, (unsigned short)length, advance(lexeme));
}

Token lex(Token token) {
    if (token.lexeme.start[0] == '\0')
        return (Token){token.lexeme, END};
    Lexeme lexeme = getNextLexeme(token.lexeme);

    const char *start = lexeme.start;
    const char *next = start + lexeme.length;
    lexErrorIf(isInvalid(start[0]), "invalid character", lexeme.location);
    if (start[0] == ' ') return (Token){lexeme, SPACE};
    if (start[0] == '\n') return (Token){lexeme, next[0] == '\0' ||
        isLineFeed(next[0]) || isComment(next[0]) ? VSPACE : NEWLINE};
    if (start[0] == '\'') return (Token){lexeme, CHARACTER};
    if (start[0] == '\"') return (Token){lexeme, STRING};
    if (isNumeric(start)) return (Token){lexeme, NUMERIC};
    if (isComment(start[0])) return (Token){lexeme, COMMENT};
    return (Token){lexeme, SYMBOLIC};
}

Token newStartToken(const char* start) {
    return (Token){newLexeme(start, 0, newLocation(0, 1, 1)), SYMBOLIC};
}
