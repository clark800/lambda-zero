#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "lib/lltoa.h"
#include "lib/tree.h"
#include "lib/errors.h"
#include "scan.h"
#include "ast.h"
#include "builtins.h"
#include "lex.h"

const char* INTERNAL_INPUT = NULL;
const char* INPUT = NULL;
bool TEST = false;

struct Position {
    unsigned int line;
    unsigned int column;
};

static inline int getLexemeLocation(const char* lexeme) {
    return (int)(lexeme - INPUT + 1);
}

const char* getLexemeByLocation(int location) {
    const char* start = location == 0 ? "\0" :
        location < 0 ? INTERNAL_INPUT : INPUT;
    return &start[abs(location) - 1];
}

const char* getLexeme(Node* node) {
    return getLexemeByLocation(getLocation(node));
}

void printLexeme(const char* lexeme, FILE* stream) {
    switch (lexeme[0]) {
        case '\n': fputs("\\n", stream); break;
        case '\0': fputs("[EOF]", stream); break;
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
        if (INPUT[i] == '\n') {
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
    if (lexeme != NULL) {
        errorArray(4, (strings){type, " error: ", message, " \'"});
        printLexeme(lexeme, stderr);
        errorArray(1, (strings){"\'"});
        if (!TEST) {
            const char* location = getLocationString(getLexemeLocation(lexeme));
            errorArray(3, (strings){" at ", location, "\n"});
        }
    } else {
        errorArray(4, (strings){type, " error: ", message, "\n"});
    }
    exit(1);
}

void throwTokenError(const char* type, const char* message, Node* token) {
    throwError(type, message, token == NULL ? NULL : getLexeme(token));
}

void lexerErrorIf(bool condition, const char* lexeme, const char* message) {
    if (condition)
        throwError("Syntax", message, lexeme);
}

bool isNameCharacter(char c) {
    return isalpha(c) || c == '_' || c == '\'' || c == '$';
}

bool isOperatorCharacter(char c) {
    return c == '\n' || (
        !isspace(c) && !iscntrl(c) && !isNameCharacter(c) && !isdigit(c));
}

bool isDigitCharacter(char c) {
    return (bool)isdigit(c);
}

bool isLexemeType(const char* lexeme, bool (*predicate)(char c)) {
    size_t length = getLexemeLength(lexeme);
    for (size_t i = 0; i < length; i++)
        if (!predicate(lexeme[i]))
            return false;
    return true;
}

bool isIfSugarLexeme(const char* lexeme) {
    return isSameLexeme(lexeme, "then") || isSameLexeme(lexeme, "else");
}

bool isNameLexeme(const char* lexeme) {
    return isLexemeType(lexeme, isNameCharacter) && !isIfSugarLexeme(lexeme);
}

bool isOperatorLexeme(const char* lexeme) {
    return isLexemeType(lexeme, isOperatorCharacter) || isIfSugarLexeme(lexeme);
}

bool isIntegerLexeme(const char* lexeme) {
    const char* digits = lexeme[0] == '-' ? lexeme + 1 : lexeme;
    return isLexemeType(digits, isDigitCharacter);
}

char getUnescapedCharacter(char c, const char* lexeme) {
    switch (c) {
        case '0': return '\0';
        case 'n': return '\n';
        case '\\': return '\\';
        case '\"': return '\"';
        default:
            lexerErrorIf(true, lexeme, "invalid escape sequence");
            return 0;
    }
}

int unescape(char* dest, const char* start, const char* end) {
    int i = 0;
    for (const char* p = start; p != end; p++) {
        if (p[0] == '\\' && p + 1 != end)
            dest[i++] = getUnescapedCharacter((++p)[0], start);
        else dest[i++] = p[0];
    }
    dest[i] = '\0';
    return i;
}

Node* newString(int location, const char* start) {
    Node* nil = newLambda(location, PARAMETERX, TRUE);
    Node* string = nil;
    int length = (int)strlen(start);
    for (int i = length - 1; i >= 0; i--)
        string = newLambda(location, PARAMETERX,
            newApplication(location, newApplication(location, REFERENCEX,
                newInteger(location, (unsigned char)start[i])), string));
    return string;
}

Node* newStringLiteral(const char* open) {
    assert(open[0] == '"');
    const char* close = open + getLexemeLength(open) - 1;
    lexerErrorIf(close[0] != '"', open, "missing end of string literal");
    int location = getLexemeLocation(open);
    size_t literalLength = (size_t)(close - open);
    char* unescaped = (char*)malloc(literalLength * sizeof(char));
    unescape(unescaped, open + 1, close);
    Node* string = newString(location, unescaped);
    free(unescaped);
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
    size_t length = getLexemeLength(lexeme);
    for (size_t i = 0; i < length; i++)
        if (lexeme[i] != '\0' && strchr(characters, lexeme[i]) != NULL)
            return &(lexeme[i]);
    return NULL;
}

Node* createToken(const char* lexeme) {
    if (lexeme[0] == '\0')
        return newOperator(getLexemeLocation(lexeme));
    if (lexeme[0] == '"')
        return newStringLiteral(lexeme);

    int location = getLexemeLocation(lexeme);
    lexerErrorIf(isSameLexeme(lexeme, ":"), lexeme, "reserved operator");
    if (INTERNAL_INPUT != INPUT && findInLexeme(lexeme, "[]{}`!@#$%") != NULL)
        lexerErrorIf(true, lexeme, "reserved character in");
    if (isIntegerLexeme(lexeme))
        return newInteger(location, parseInteger(lexeme));
    if (isNameLexeme(lexeme))
        return newName(location);
    if (isOperatorLexeme(lexeme))
        return newOperator(location);
    lexerErrorIf(true, lexeme, "invalid token");
    return NULL;
}

bool isForbiddenCharacter(char c) {
    return iscntrl(c) && c != '\n';
}

Hold* getFirstToken(const char* input) {
    INTERNAL_INPUT = INPUT == NULL ? input : INPUT;
    INPUT = input;
    for (const char* p = input; p[0] != '\0'; p++)
        lexerErrorIf(isForbiddenCharacter(p[0]), p, "foridden character");
    return hold(createToken(getFirstLexeme(input)));
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

Node* newEOF() {
    return newOperator(0);
}
