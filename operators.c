#include <stddef.h>
#include <stdbool.h>
#include "lib/tree.h"
#include "ast.h"
#include "objects.h"
#include "errors.h"
#include "lex.h"
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

int getTupleSize(Node* tuple) {
    int i = 0;
    for (Node* n = getBody(tuple); isApplication(n); i++)
        n = getLeft(n);
    return i;
}

Node* apply(Node* operator, Node* left, Node* right) {
    return newApplication(getLocation(operator), left, right);
}

Node* infix(Node* operator, Node* left, Node* right) {
    return apply(operator, apply(operator, operator, left), right);
}

Node* comma(Node* operator, Node* left, Node* right) {
    return (isCommaList(left) ? apply : infix)(operator, left, right);
}

Node* newArrow(Node* operator, Node* left, Node* right) {
    int location = getLocation(operator);
    if (isSymbol(left))
        return newLambda(location, newParameter(getLocation(left)), right);
    if (!(isTuple(left) && getTupleSize(left) == 2))
        syntaxError("invalid parameter", left);
    // (x, y) -> body ---> p -> (x -> y -> body) left(p) right(p)
    Node* leftName = getRight(getLeft(getBody(left)));
    Node* rightName = getRight(getBody(left));
    Node* leftComponent = newApplication(location, getBody(IDENTITY), TRUE);
    Node* rightComponent = newApplication(location, getBody(IDENTITY), FALSE);
    Node* lambda = newLambda(location, newParameter(getLocation(leftName)),
        newLambda(location, newParameter(getLocation(rightName)), right));
    Node* application = newApplication(location,
        newApplication(location, lambda, leftComponent), rightComponent);
    return newLambda(location, getParameter(IDENTITY), application);
}

Node* prefix(Node* operator, Node* left, Node* right) {
    (void)left;     // suppress unused parameter warning
    return apply(operator, operator, right);
}

Node* negate(Node* operator, Node* left, Node* right) {
    (void)left;     // suppress unused parameter warning
    return infix(operator, newInteger(getLocation(operator), 0), right);
}

Node* unmatched(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(true, "missing close for", operator);
    return left == NULL ? right : left; // suppress unused parameter warning
}

Node* brackets(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "["), "missing open for", close);
    if (contents == NULL)
        return newNil(getLocation(open));
    if (!isCommaList(contents))
        return prepend(contents, newNil(getLocation(open)));
    Node* list = newNil(getLocation(open));
    for(; isCommaList(getLeft(contents)); contents = getLeft(contents))
        list = prepend(getRight(contents), list);
    return prepend(getRight(contents), list);
}

Node* parentheses(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "("), "missing open for", close);
    if (contents == NULL)
        return newLambda(getLocation(open),
            getParameter(IDENTITY), getBody(IDENTITY));
    if (isOperator(contents)) {
        if (isSpecialOperator(getOperator(contents, false)))
            syntaxError("operator cannot be parenthesized", contents);
        return newName(getLocation(contents));
    }
    return contents;
}

// comma must be the lowest precedence operator above parentheses/brackets
// or else a commaList, which is an invalid operand, could get buried in the
// AST without being detected, then a surrounding parentheses could apply
// a tuple abstraction, which would bind across a bracket boundary.
// if a comma is not wrapped in parentheses or brackets, it will be at the
// very top level and thus won't be defined, so bind will catch this case.
Rules RULES[] = {
    {"\0", 0, 0, CLOSE, L, NULL},
    {"(", 14, 0, OPEN, L, unmatched},
    {")", 0, 14, CLOSE, R, parentheses},
    {"[", 14, 0, OPEN, L, unmatched},
    {"]", 0, 14, CLOSE, R, brackets},
    {",", 1, 1, IN, L, comma},
    {"\n", 2, 2, IN, R, apply},
    {"=", 3, 3, IN, N, apply},
    {"|", 4, 4, IN, L, infix},
    {"|~", 4, 4, IN, L, infix},
    {"->", 5, 5, IN, R, newArrow},
    {"||", 5, 5, IN, R, infix},
    {"::?", 5, 5, IN, R, infix},
    {"?", 6, 6, IN, R, infix},
    {"\\/", 7, 7, IN, R, infix},
    {"/\\", 8, 8, IN, R, infix},
    {"==", 9, 9, IN, N, infix},
    {"=/=", 9, 9, IN, N, infix},
    {"===", 9, 9, IN, N, infix},
    {"<=>", 9, 9, IN, N, infix},
    {"=>", 9, 9, IN, N, infix},
    {"<", 9, 9, IN, N, infix},
    {">", 9, 9, IN, N, infix},
    {"<=", 9, 9, IN, N, infix},
    {">=", 9, 9, IN, N, infix},
    {">=<", 9, 9, IN, N, infix},
    {"<:", 9, 9, IN, N, infix},
    {":", 9, 9, IN, N, infix},
    {"::", 10, 10, IN, R, infix},
    {"..", 10, 10, IN, N, infix},
    {"++", 10, 10, IN, R, infix},
    {"--", 10, 10, IN, N, infix},
    {"**", 10, 10, IN, R, infix},
    {"^^", 10, 10, IN, L, infix},
    {"?:", 10, 10, IN, R, infix},
    {"+", 11, 11, IN, L, infix},
    {"-", 11, 11, IN, L, infix},
    {"-", 11, 11, PRE, L, negate},
    {"~", 11, 11, PRE, L, prefix},
    {"#", 11, 11, PRE, L, prefix},
    {"*", 12, 12, IN, L, infix},
    {"/", 12, 12, IN, L, infix},
    {"%", 12, 12, IN, L, infix},
    {"\\", 12, 12, IN, L, infix},
    {"^", 13, 13, IN, R, infix},
    {"<>", 13, 13, IN, R, infix},
    {"?.", 15, 15, IN, L, infix},
    {".", 15, 15, IN, L, infix},
    {"`", 15, 15, PRE, L, prefix}
};

Rules DEFAULT = {"", 10, 10, IN, L, infix};

bool allowsOperatorBefore(Rules rules) {
    return rules.fixity == PRE || rules.fixity == OPEN || rules.fixity == CLOSE;
}

Operator getOperator(Node* token, bool isAfterOperator) {
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
