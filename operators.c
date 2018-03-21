#include <stddef.h>
#include <stdbool.h>
#include "lib/tree.h"
#include "ast.h"
#include "lex.h"
#include "operators.h"

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

Operator OPERATORS[] = {
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

Operator DEFAULT = {"", 150, 150, L, infix};
Operator SPACE = {" ", 240, 240, L, apply};

Operator getOperator(Node* token, bool prefixOnly) {
    if (isSpace(token))
        return SPACE;
    for (unsigned int i = 0; i < sizeof(OPERATORS)/sizeof(Operator); i++)
        if (isThisToken(token, OPERATORS[i].symbol))
            if (!prefixOnly || OPERATORS[i].associativity == P)
                return OPERATORS[i];
    return DEFAULT;
}

bool isParenthesizableOperator(Node* token) {
    Operator op = getOperator(token, false);
    return op.collapse == infix || op.collapse == prefix;
}

bool isAlwaysPrefixOperator(Node* token) {
    return getOperator(token, false).associativity == P;
}

bool isAlwaysInfixOperator(Node* token) {
    return getOperator(token, true).associativity != P;
}

bool isSectionableOperator(Node* token) {
    return isParenthesizableOperator(token) && !isAlwaysPrefixOperator(token);
}
