#include <stddef.h>
#include <stdbool.h>
#include "lib/tree.h"
#include "ast.h"
#include "lex.h"
#include "operators.h"

typedef unsigned char Precedence;
typedef enum {L, R, N, P} Associativity;
struct Rules {
    const char* symbol;
    Precedence leftPrecedence;
    Precedence rightPrecedence;
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

Node* paren(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(true, operator, "missing close parenthesis for");
    return left == NULL ? right : left; // suppress unused parameter warning
}

Rules RULES[] = {
    {"\0", 0, 0, R, NULL},
    {"(", 240, 10, P, paren},
    {")", 10, 240, R, NULL},
    {",", 20, 20, R, apply},
    {";", 30, 30, R, infix},
    {"\n", 40, 40, R, apply},
    {"=", 50, 50, N, apply},
    {"->", 240, 60, R, lambda},
    {"?|", 80, 80, R, infix},
    {"?", 80, 80, L, infix},
    {"?:", 80, 80, R, infix},
    {"then", 80, 80, L, apply},
    {"else", 80, 70, L, apply},
    {"\\/", 90, 90, R, infix},
    {"/\\", 100, 100, R, infix},
    {"==", 120, 120, N, infix},
    {"=/=", 120, 120, N, infix},
    {"===", 120, 120, N, infix},
    {"<=>", 120, 120, N, infix},
    {"=>", 120, 120, N, infix},
    {"<", 120, 120, N, infix},
    {">", 120, 120, N, infix},
    {"<=", 120, 120, N, infix},
    {">=", 120, 120, N, infix},
    {"|", 140, 140, L, infix},
    {"|~", 140, 140, L, infix},
    {"++", 150, 150, R, infix},
    {"\\", 160, 160, L, infix},
    {"\\\\", 160, 160, L, infix},
    {"::", 170, 170, R, infix},
    {"..", 180, 180, N, infix},
    {"^^", 190, 190, L, infix},
    {"+", 200, 200, L, infix},
    {"-", 200, 200, L, infix},
    {"*", 210, 210, L, infix},
    {"/", 210, 210, L, infix},
    {"^", 220, 220, R, infix},
    {"<>", 230, 230, R, infix},
    {"-", 235, 245, P, negate},
    {"~", 235, 245, P, prefix},
    {"!", 235, 245, P, prefix},
    {".", 250, 250, L, infix}
};

Rules DEFAULT = {"", 150, 150, L, infix};
Rules SPACE = {" ", 240, 240, L, apply};

Operator getOperator(Node* token, bool prefixOnly) {
    if (isSpace(token))
        return (Operator){token, &SPACE};
    for (unsigned int i = 0; i < sizeof(RULES)/sizeof(Rules); i++)
        if (isThisToken(token, RULES[i].symbol))
            if (!prefixOnly || RULES[i].associativity == P)
                return (Operator){token, &(RULES[i])};
    return (Operator){token, &DEFAULT};
}

int getOperatorArity(Operator operator) {
    return operator.rules->associativity == P ? 1 : 2;
}

Node* applyOperator(Operator operator, Node* left, Node* right) {
    return operator.rules->apply(operator.token, left, right);
}

bool isSpecialOperator(Operator operator) {
    return operator.rules->apply != infix && operator.rules->apply != prefix;
}

bool isPrefixOperator(Operator operator) {
    return operator.rules->associativity == P;
}

bool isHigherPrecedence(Operator left, Operator right) {
    if (left.rules->rightPrecedence == right.rules->leftPrecedence) {
        const char* message = "operator is non-associative";
        syntaxErrorIf(left.rules->associativity == N, left.token, message);
        syntaxErrorIf(right.rules->associativity == N, right.token, message);
    }

    if (right.rules->associativity == L || right.rules->associativity == P)
        return left.rules->rightPrecedence >= right.rules->leftPrecedence;
    else
        return left.rules->rightPrecedence > right.rules->leftPrecedence;
}
