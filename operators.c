#include <stddef.h>
#include <stdbool.h>
#include "lib/tree.h"
#include "ast.h"
#include "objects.h"
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

Node* apply(Node* operator, Node* left, Node* right) {
    return newApplication(getLocation(operator), left, right);
}

Node* infix(Node* operator, Node* left, Node* right) {
    convertOperatorToName(operator);
    return apply(operator, apply(operator, operator, left), right);
}

Node* lambda(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(!isName(left), left, "expected name but got");
    convertNameToParameter(left);
    return newLambda(getLocation(operator), left, right);
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
    syntaxErrorIf(true, operator, "missing close for");
    return left == NULL ? right : left; // suppress unused parameter warning
}

Node* constructList(int location, Node* commaTuple) {
    if (!isCommaTuple(commaTuple))
        return prepend(commaTuple, newNil(location));
    return prepend(getLeft(commaTuple),
        constructList(location, getRight(commaTuple)));
}

Node* brackets(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "["), close, "missing open for");
    if (contents == NULL)
        return newNil(getLocation(open));
    if (!isCommaTuple(contents))
        return prepend(contents, newNil(getLocation(open)));
    return constructList(getLocation(open), contents);
}

Node* parentheses(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "("), close, "missing open for");
    syntaxErrorIf(contents == NULL, open, "empty parentheses");
    if (isOperator(contents)) {
        syntaxErrorIf(isSpecialOperator(getOperator(contents, false)),
            contents, "operator cannot be parenthesized");
        convertOperatorToName(contents);
    }
    return contents;
}

Rules RULES[] = {
    {"\0", 0, 0, CLOSE, R, NULL},
    {"(", 240, 10, OPEN, L, unmatched},
    {")", 10, 240, CLOSE, R, parentheses},
    {"[", 240, 10, OPEN, L, unmatched},
    {"]", 10, 240, CLOSE, R, brackets},
    {",", 20, 20, IN, R, apply},
    {";", 30, 30, IN, R, infix},
    {"\n", 40, 40, IN, R, apply},
    {"=", 50, 50, IN, N, apply},
    {"->", 240, 60, IN, R, lambda},
    {"?|", 80, 80, IN, R, infix},
    {"?", 80, 80, IN, L, infix},
    {"?:", 80, 80, IN, R, infix},
    {"then", 80, 80, IN, L, apply},
    {"else", 80, 70, IN, L, apply},
    {"\\/", 90, 90, IN, R, infix},
    {"/\\", 100, 100, IN, R, infix},
    {"==", 120, 120, IN, N, infix},
    {"=/=", 120, 120, IN, N, infix},
    {"===", 120, 120, IN, N, infix},
    {"<=>", 120, 120, IN, N, infix},
    {"=>", 120, 120, IN, N, infix},
    {"<", 120, 120, IN, N, infix},
    {">", 120, 120, IN, N, infix},
    {"<=", 120, 120, IN, N, infix},
    {">=", 120, 120, IN, N, infix},
    {"|", 140, 140, IN, L, infix},
    {"|~", 140, 140, IN, L, infix},
    {"++", 150, 150, IN, R, infix},
    {"\\", 160, 160, IN, L, infix},
    {"\\\\", 160, 160, IN, L, infix},
    {"::", 170, 170, IN, R, infix},
    {"..", 180, 180, IN, N, infix},
    {"^^", 190, 190, IN, L, infix},
    {"+", 200, 200, IN, L, infix},
    {"-", 200, 200, IN, L, infix},
    {"*", 210, 210, IN, L, infix},
    {"/", 210, 210, IN, L, infix},
    {"^", 220, 220, IN, R, infix},
    {"<>", 230, 230, IN, R, infix},
    {"-", 235, 245, PRE, L, negate},
    {"~", 235, 245, PRE, L, prefix},
    {"!", 235, 245, PRE, L, prefix},
    {".", 250, 250, IN, L, infix}
};

Rules DEFAULT = {"", 150, 150, IN, L, infix};

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
    return operator.rules->apply != infix && operator.rules->apply != prefix;
}

bool isHigherPrecedence(Operator left, Operator right) {
    if (left.rules->rightPrecedence == right.rules->leftPrecedence) {
        const char* message = "operator is non-associative";
        syntaxErrorIf(left.rules->associativity == N, left.token, message);
        syntaxErrorIf(right.rules->associativity == N, right.token, message);
    }

    if (right.rules->associativity == R)
        return left.rules->rightPrecedence > right.rules->leftPrecedence;
    else
        return left.rules->rightPrecedence >= right.rules->leftPrecedence;
}
