#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "scan.h"
#include "errors.h"
#include "ast.h"
#include "objects.h"
#include "lex.h"

Node* newEOF() {
    return newOperator(0);
}

const char* getLexeme(Node* node) {
    return getLexemeByLocation(getLocation(node));
}

bool isInternalToken(Node* token) {
    return isLeafNode(token) && !isInteger(token) && *getLexeme(token) == '\'';
}

void printToken(Node* token, FILE* stream) {
    const char* lexeme = getLexeme(token);
    printLexeme(isInternalToken(token) ? lexeme + 1 : lexeme, stream);
}

bool isIfSugarLexeme(const char* lexeme) {
    return isSameLexeme(lexeme, "then") || isSameLexeme(lexeme, "else");
}

bool isNameLexeme(const char* lexeme) {
    return isOperandCharacter(lexeme[0]) && !isdigit(lexeme[0]) &&
        !isIfSugarLexeme(lexeme);
}

bool isOperatorLexeme(const char* lexeme) {
    return isDelimiterCharacter(lexeme[0]) ||
        isOperatorCharacter(lexeme[0]) || isIfSugarLexeme(lexeme);
}

bool isIntegerLexeme(const char* lexeme) {
    for (unsigned int i = 0; i < getLexemeLength(lexeme); i++)
        if (!isdigit(lexeme[i]))
            return false;
    return true;
}

const char* skipQuoteCharacter(const char* start) {
    return start[0] == '\\' ? start + 2 : start + 1;
}

char decodeCharacter(const char* start) {
    if (start[0] != '\\')
        return start[0];
    switch (start[1]) {
        case '0': return '\0';
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case '\\': return '\\';
        case '\"': return '\"';
        case '\'': return '\'';
        default: lexerErrorIf(true, start, "invalid escape sequence after");
    }
    return 0;
}

Node* newCharacterLiteral(const char* lexeme) {
    char quote = lexeme[0];
    const char* end = lexeme + getLexemeLength(lexeme) - 1;
    lexerErrorIf(end[0] != quote, lexeme, "missing end quote");
    const char* skip = skipQuoteCharacter(lexeme + 1);
    lexerErrorIf(skip != end, lexeme, "invalid character literal");
    unsigned char code = (unsigned char)decodeCharacter(lexeme + 1);
    return newInteger(getLexemeLocation(lexeme), code);
}

Node* newStringLiteral(const char* lexeme) {
    char quote = lexeme[0];
    int location = getLexemeLocation(lexeme);
    const char* close = lexeme + getLexemeLength(lexeme) - 1;
    lexerErrorIf(close[0] != quote, lexeme, "missing end quote");
    Node* string = newNil(getLexemeLocation(lexeme));
    Stack* stack = newStack(VOID);
    const char* p = lexeme + 1;
    for (; p < close; p = skipQuoteCharacter(p))
        push(stack, newInteger(location, decodeCharacter(p)));
    lexerErrorIf(p != close, lexeme, "invalid string literal");
    for (Iterator* it = iterate(stack); !end(it); it = next(it))
        string = prepend(cursor(it), string);
    deleteStack(stack);
    return string;
}

long long parseInteger(const char* lexeme) {
    errno = 0;
    long long result = strtoll(lexeme, NULL, 10);
    lexerErrorIf((result == LLONG_MIN || result == LLONG_MAX) &&
        errno == ERANGE, lexeme, "magnitude of integer is too large");
    return result;
}

Node* createToken(const char* lexeme) {
    if (lexeme[0] == '"')
        return newStringLiteral(lexeme);
    // single quoted operands are internal names while parsing internal code
    if (lexeme[0] == '\'' && IDENTITY != NULL)
        return newCharacterLiteral(lexeme);

    int location = getLexemeLocation(lexeme);
    if (isIntegerLexeme(lexeme))
        return newInteger(location, parseInteger(lexeme));
    if (isNameLexeme(lexeme))
        return newName(location);
    if (isOperatorLexeme(lexeme))
        return newOperator(location);

    lexerErrorIf(true, lexeme, "invalid token");
    return NULL;
}

Hold* getFirstToken(const char* sourceCode) {
    return hold(createToken(getFirstLexeme(sourceCode)));
}

Hold* getNextToken(Hold* token) {
    return hold(createToken(getNextLexeme(getLexeme(getNode(token)))));
}

bool isSameToken(Node* tokenA, Node* tokenB) {
    return isSameLexeme(getLexeme(tokenA), getLexeme(tokenB));
}

bool isThisToken(Node* token, const char* lexeme) {
    return isSameLexeme(getLexeme(token), lexeme);
}
