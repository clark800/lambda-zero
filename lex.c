#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "lib/lltoa.h"
#include "lib/tree.h"
#include "lib/stack.h"
#include "lib/errors.h"
#include "scan.h"
#include "ast.h"
#include "objects.h"
#include "lex.h"

const char* SOURCE_CODE = NULL;

struct Position {
    unsigned int line, column;
};

static inline int getLexemeLocation(const char* lexeme) {
    return (int)(lexeme - SOURCE_CODE + 1);
}

const char* getLexemeByLocation(int location) {
    const char* start = location < 0 ? INTERNAL_CODE : SOURCE_CODE;
    return location == 0 ? "\0" : &start[abs(location) - 1];
}

const char* getLexeme(Node* node) {
    return getLexemeByLocation(getLocation(node));
}

void printLexeme(const char* lexeme, FILE* stream) {
    switch (lexeme[0]) {
        case '\n': fputs("\\n", stream); break;
        case '\0': fputs("$EOF", stream); break;
        default: fwrite(lexeme, sizeof(char), getLexemeLength(lexeme), stream);
    }
}

void printToken(Node* token, FILE* stream) {
    printLexeme(getLexeme(token), stream);
}

struct Position getPosition(unsigned int location) {
    struct Position position = {1, 1};  // use 1-based indexing
    for (unsigned int i = 0; i < location - 1; i++) {
        position.column += 1;
        if (SOURCE_CODE[i] == '\n') {
            position.line += 1;
            position.column = 1;
        }
    }
    return position;
}

const char* getLocationString(int location) {
    if (location < 0)
        return "[INTERNAL OBJECT]";
    static char buffer[128];
    struct Position position = getPosition((unsigned int)location);
    strcpy(buffer, "line ");
    lltoa(position.line, buffer + strlen(buffer), 10);
    strcat(buffer, " column ");
    lltoa(position.column, buffer + strlen(buffer), 10);
    return buffer;
}

void throwError(const char* type, const char* message, const char* lexeme) {
    errorArray(4, (strings){type, " error: ", message, " \'"});
    printLexeme(lexeme, stderr);
    errorArray(1, (strings){"\'"});
    const char* location = getLocationString(getLexemeLocation(lexeme));
    if (VERBOSITY >= 0)
        errorArray(3, (strings){" at ", location, "\n"});
    exit(1);
}

void throwTokenError(const char* type, const char* message, Node* token) {
    throwError(type, message, getLexeme(token));
}

void lexerErrorIf(bool condition, const char* lexeme, const char* message) {
    if (condition)
        throwError("Syntax", message, lexeme);
}

void syntaxError(const char* message, Node* token) {
    throwTokenError("Syntax", message, token);
}

void syntaxErrorIf(bool condition, const char* message, Node* token) {
    if (condition)
        syntaxError(message, token);
}

bool isNameCharacter(char c) {
    return isOperandCharacter(c) && c != '"';
}

bool isDigitCharacter(char c) {
    return (bool)isdigit(c);
}

bool isForbiddenCharacter(char c) {
    return iscntrl(c) && c != '\n';
}

bool isLexemeType(const char* lexeme, bool (*predicate)(char c)) {
    unsigned int length = getLexemeLength(lexeme);
    for (unsigned int i = 0; i < length; i++)
        if (!predicate(lexeme[i]))
            return false;
    return true;
}

bool isIfSugarLexeme(const char* lexeme) {
    return isSameLexeme(lexeme, "then") || isSameLexeme(lexeme, "else");
}

bool isNameLexeme(const char* lexeme) {
    return isLexemeType(lexeme, isNameCharacter) &&
        !isIfSugarLexeme(lexeme) && !isQuoteCharacter(lexeme[0]) &&
        !isDigitCharacter(lexeme[0]);
}

bool isOperatorLexeme(const char* lexeme) {
    return isDelimiterCharacter(lexeme[0]) ||
        isLexemeType(lexeme, isOperatorCharacter) || isIfSugarLexeme(lexeme);
}

bool isIntegerLexeme(const char* lexeme) {
    return isLexemeType(lexeme, isDigitCharacter);
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
        case '\\': return '\\';
        case '\"': return '\"';
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

const char* findInLexeme(const char* lexeme, const char* characters) {
    for (unsigned int i = 0; i < getLexemeLength(lexeme); i++)
        if (lexeme[i] != '\0' && strchr(characters, lexeme[i]) != NULL)
            return &(lexeme[i]);
    return NULL;
}

Node* createToken(const char* lexeme) {
    if (lexeme[0] == '"')
        return newStringLiteral(lexeme);
    if (lexeme[0] == '\'')
        return newCharacterLiteral(lexeme);

    lexerErrorIf(isSameLexeme(lexeme, ":"), lexeme, "reserved operator");
    bool isUserCode = SOURCE_CODE != INTERNAL_CODE;
    if (isUserCode && findInLexeme(lexeme, "{}`!@#$%") != NULL)
        lexerErrorIf(true, lexeme, "reserved character in");

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
    SOURCE_CODE = sourceCode;
    for (const char* p = sourceCode; p[0] != '\0'; p++)
        lexerErrorIf(isForbiddenCharacter(p[0]), p, "foridden character");
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

bool isInternalToken(Node* token) {
    return getLexeme(token)[0] == '$';
}

Node* newEOF() {
    return newOperator(0);
}
