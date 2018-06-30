#include "lib/tree.h"
#include "ast.h"
#include "scan.h"
#include "errors.h"
#include "operators.h"

typedef enum {L, R, N} Associativity;
typedef unsigned char Precedence;
struct Rules {
    const char* symbol;
    Precedence leftPrecedence, rightPrecedence;
    Fixity fixity;
    Associativity associativity;
    Node* (*apply)(Node* operator, Node* left, Node* right);
};

Node* apply(Node* operator, Node* left, Node* right) {
    return newApplication(getLexeme(operator), left, right);
}

Node* infix(Node* operator, Node* left, Node* right) {
    return apply(operator, apply(operator, operator, left), right);
}

Node* comma(Node* operator, Node* left, Node* right) {
    return (isCommaList(left) ? apply : infix)(operator, left, right);
}

int getTupleSize(Node* tuple) {
    int i = 0;
    for (Node* n = getBody(tuple); isApplication(n); i++)
        n = getLeft(n);
    return i;
}

Node* newProjection(String lexeme, int size, int index) {
    Node* projection = newReference(lexeme,
        (unsigned long long)(size - index));
    for (int i = 0; i < size; i++)
        projection = newLambda(lexeme, getParameter(IDENTITY), projection);
    return projection;
}

Node* newPatternLambda(Node* operator, Node* left, Node* right) {
    String lexeme = getLexeme(operator);
    if (isSymbol(left))
        return newLambda(lexeme, newParameter(getLexeme(left)), right);
    if (!isTuple(left) || getTupleSize(left) == 0)
        syntaxError("invalid parameter", left);
    // (x, y) -> body ---> p -> (x -> y -> body) left(p) right(p)
    Node* body = right;
    for (Node* items = getBody(left); isApplication(items);
            items = getLeft(items))
        body = newPatternLambda(operator, getRight(items), body);
    for (int i = 0, size = getTupleSize(left); i < size; i++)
        body = newApplication(lexeme, body,
            newApplication(lexeme, getBody(IDENTITY),
                newProjection(lexeme, size, i)));
    return newLambda(lexeme, getParameter(IDENTITY), body);
}

Node* prefix(Node* operator, Node* left, Node* right) {
    (void)left;     // suppress unused parameter warning
    return apply(operator, operator, right);
}

Node* negate(Node* operator, Node* left, Node* right) {
    (void)left;     // suppress unused parameter warning
    return infix(operator, newInteger(getLexeme(operator), 0), right);
}

Node* unmatched(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(true, "missing close for", operator);
    return left == NULL ? right : left; // suppress unused parameter warning
}

Node* brackets(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "["), "missing open for", close);
    if (contents == NULL)
        return newNil(getLexeme(open));
    String lexeme = getLexeme(open);
    if (!isCommaList(contents))
        return prepend(lexeme, contents, newNil(getLexeme(open)));
    Node* list = newNil(getLexeme(open));
    for(; isCommaList(getLeft(contents)); contents = getLeft(contents))
        list = prepend(lexeme, getRight(contents), list);
    return prepend(lexeme, getRight(contents), list);
}

Node* parentheses(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "("), "missing open for", close);
    if (contents == NULL)
        return newUnit(getLexeme(open));
    if (isOperator(contents)) {
        if (isSpecialOperator(getOperator(contents, false)))
            syntaxError("operator cannot be parenthesized", contents);
        return newName(getLexeme(contents));
    }
    if (isCommaList(contents))
        return newTuple(getLexeme(open), contents);
    return contents;
}

// comma must be the lowest precedence operator above parentheses/brackets
// or else a commaList, which is an invalid operand, could get buried in the
// AST without being detected, then a surrounding parentheses could apply
// a tuple abstraction, which would bind across a bracket boundary.
// if a comma is not wrapped in parentheses or brackets, it will be at the
// very top level and thus won't be defined, so bind will catch this case.
Rules RULES[] = {
    // syntactic operators
    {"\0", 0, 0, CLOSE, L, NULL},
    {"(", 22, 0, OPEN, L, unmatched},
    {")", 0, 22, CLOSE, R, parentheses},
    {"[", 22, 0, OPEN, L, unmatched},
    {"]", 0, 22, CLOSE, R, brackets},
    {",", 1, 1, IN, L, comma},
    //{";", 1, 1, IN, N, semicolon},
    {"\n", 2, 2, IN, R, apply},
    {":=", 3, 3, IN, N, apply},
    {"|", 4, 4, IN, L, infix},
    {"->", 5, 5, IN, R, newPatternLambda},

    // conditional operators
    {"||", 5, 5, IN, R, infix},
    {"?", 6, 6, IN, R, infix},
    {"?||", 6, 6, IN, R, infix},

    // monadic operators
    {">>=", 7, 7, IN, L, infix},

    // logical operators
    {"<=>", 8, 8, IN, N, infix},
    {"=>", 9, 9, IN, N, infix},
    {"\\/", 10, 10, IN, R, infix},
    {"/\\", 11, 11, IN, R, infix},

    // comparison/test operators
    {"=", 12, 12, IN, N, infix},
    {"!=", 12, 12, IN, N, infix},
    {"=*=", 12, 12, IN, N, infix},
    {"<", 12, 12, IN, N, infix},
    {">", 12, 12, IN, N, infix},
    {"<=", 12, 12, IN, N, infix},
    {">=", 12, 12, IN, N, infix},
    {"=<", 12, 12, IN, N, infix},
    {"~<", 12, 12, IN, N, infix},
    {"<:", 12, 12, IN, N, infix},
    {":", 12, 12, IN, N, infix},
    {"!:", 12, 12, IN, N, infix},
    {"~", 12, 12, IN, N, infix},

    // precedence 14: default
    // arithmetic/list operators
    {"..", 15, 15, IN, N, infix},
    {"::", 16, 16, IN, R, infix},
    {"&", 16, 16, IN, L, infix},
    {"+", 16, 16, IN, L, infix},
    {"\\./", 16, 16, IN, L, infix},
    {"++", 16, 16, IN, R, infix},
    {"-", 16, 16, IN, L, infix},
    {"\\", 16, 16, IN, L, infix},
    {"*", 17, 17, IN, L, infix},
    {"**", 17, 17, IN, R, infix},
    {"/", 17, 17, IN, L, infix},
    {"%", 17, 17, IN, L, infix},
    {"^", 18, 18, IN, R, infix},

    // functional operators
    {"<>", 19, 19, IN, R, infix},

    // precedence 20: space operator
    // prefix operators
    {"-", 21, 21, PRE, L, negate},
    {"--", 21, 21, PRE, L, prefix},
    {"!", 21, 21, PRE, L, prefix},
    {"#", 21, 21, PRE, L, prefix},

    // precedence 22: parentheses/brackets
    {"^^", 23, 23, IN, L, infix},
    {".", 24, 24, IN, L, infix},
    {"`", 25, 25, PRE, L, prefix}
};

Rules DEFAULT = {"", 14, 14, IN, L, infix};
Rules SPACE = {" ", 20, 20, IN, L, apply};

bool allowsOperatorBefore(Rules rules) {
    return rules.fixity == PRE || rules.fixity == OPEN || rules.fixity == CLOSE;
}

bool isSpace(Node* token) {
    return isLeaf(token) && isSpaceCharacter(getLexeme(token).start[0]);
}

Operator getOperator(Node* token, bool isAfterOperator) {
    if (isSpace(token))
        return (Operator){token, &SPACE};
    for (unsigned int i = 0; i < sizeof(RULES)/sizeof(Rules); i++)
        if (isThisToken(token, RULES[i].symbol))
            if (!isAfterOperator || allowsOperatorBefore(RULES[i]))
                return (Operator){token, &(RULES[i])};
    return (Operator){token, &DEFAULT};
}

Fixity getFixity(Operator operator) {
    return operator.rules->fixity;
}

Node* applyOperator(Operator operator, Node* left, Node* right) {
    return operator.rules->apply(operator.token, left, right);
}

bool isSpecialOperator(Operator operator) {
    return operator.rules->apply != infix && operator.rules->apply != prefix
        && operator.rules->apply != comma;
}

bool isHigherPrecedence(Operator left, Operator right) {
    if (left.rules->rightPrecedence == right.rules->leftPrecedence) {
        const char* message = "operator is non-associative";
        syntaxErrorIf(left.rules->associativity == N, message, left.token);
        syntaxErrorIf(right.rules->associativity == N, message, right.token);
    }

    if (right.rules->associativity == R)
        return left.rules->rightPrecedence > right.rules->leftPrecedence;
    else
        return left.rules->rightPrecedence >= right.rules->leftPrecedence;
}
