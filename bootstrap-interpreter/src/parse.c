#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "lib/tree.h"
#include "lib/array.h"
#include "lib/stack.h"
#include "lex.h"
#include "ast.h"
#include "errors.h"
#include "symbols.h"
#include "syntax.h"
#include "bind.h"
#include "debug.h"
#include "parse.h"

bool TRACE_PARSING = false;

static bool isNaturalLexeme(String lexeme) {
    for (unsigned int i = 0; i < lexeme.length; ++i)
        if (!isdigit(lexeme.start[i]))
            return false;
    return lexeme.length > 0;
}

static bool isIntegerLexeme(String lexeme) {
    if (lexeme.start[0] != '-')
        return isNaturalLexeme(lexeme);
    return isNaturalLexeme(newString(lexeme.start + 1, lexeme.length - 1));
}

static Node* parseInteger(Tag tag) {
    tokenErrorIf(!isIntegerLexeme(tag.lexeme), "invalid token", tag);
    errno = 0;
    long long value = strtoll(tag.lexeme.start, NULL, 10);
    tokenErrorIf((value == LLONG_MIN || value == LLONG_MAX) &&
        errno == ERANGE, "magnitude of integer is too large", tag);
    return newInteger(tag, value);
}

static const char* skipQuoteCharacter(const char* start) {
    return start[0] == '\\' ? start + 2 : start + 1;
}

static char decodeCharacter(const char* start, Tag tag) {
    tokenErrorIf(start[0] <= 0, "invalid character in", tag);
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
        default: tokenErrorIf(true, "invalid escape sequence in", tag);
    }
    return 0;
}

static Node* parseCharacterLiteral(Tag tag) {
    char quote = tag.lexeme.start[0];
    const char* end = tag.lexeme.start + tag.lexeme.length - 1;
    tokenErrorIf(end[0] != quote, "missing end quote for", tag);
    const char* skip = skipQuoteCharacter(tag.lexeme.start + 1);
    tokenErrorIf(skip != end, "invalid character literal", tag);
    return newInteger(tag, decodeCharacter(tag.lexeme.start + 1, tag));
}

static Node* buildStringLiteral(Tag tag, const char* start) {
    char c = start[0];
    tokenErrorIf(c == '\n' || c == 0, "missing end quote for", tag);
    return c == tag.lexeme.start[0] ? newNil(tag) :
        prepend(tag, newInteger(tag, decodeCharacter(start, tag)),
        buildStringLiteral(tag, skipQuoteCharacter(start)));
}

static Node* parseStringLiteral(Tag tag) {
    return buildStringLiteral(tag, tag.lexeme.start + 1);
}

static Node* parseToken(Token token, Stack* stack) {
    switch (token.type) {
        case NUMERIC: return parseInteger(token.tag);
        case STRING: return parseStringLiteral(token.tag);
        case CHARACTER: return parseCharacterLiteral(token.tag);
        case INVALID: tokenErrorIf(true, "invalid character", token.tag);
            return NULL;
        case SPACE: return parseSymbol(renameTag(token.tag, " "), stack);
        default: return parseSymbol(token.tag, stack);
    }
}

Program parse(const char* input) {
    initSymbols();
    Stack* stack = newStack();
    Token start = newStartToken(input);
    push(stack, parseToken(start, stack));
    for (Token token = lex(start); token.type != END; token = lex(token)) {
        debugParseState(token.tag, stack, TRACE_PARSING);
        if (token.type == COMMENT)
            continue;
        Node* node = parseToken(token, stack);
        shift(stack, node);
        release(hold(node));
    }
    Hold* result = pop(stack);
    deleteStack(stack);
    debugParseStage("parse", getNode(result), TRACE_PARSING);
    Array* globals = bind(result);
    debugParseStage("bind", getNode(result), TRACE_PARSING);
    Node* entry = elementAt(globals, length(globals) - 1);
    return (Program){result, entry, globals};
}

void deleteProgram(Program program) {
    release(program.root);
    deleteArray(program.globals);
}
