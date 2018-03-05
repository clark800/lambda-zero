#include "lex.h"
#include "ast.h"
#include "parse.h"
#include "operators.h"

Node* apply(Node* operator, Node* left, Node* right) {
    return newApplication(getLocation(operator), left, right);
}

Node* infix(Node* operator, Node* left, Node* right) {
    convertOperatorToName(operator);
    return apply(operator, apply(operator, operator, left), right);
}

Node* collapseLambda(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(!isName(left), left, "expected name but got");
    convertNameToParameter(left);
    return newLambda(getLocation(operator), left, right);
}

Operator DEFAULT = {"", 150, 150, L, infix};

Operator OPERATORS[] = {
    {"\0", 0, 0, R, NULL},
    {"(", 240, 10, R, NULL},
    {")", 10, 240, R, NULL},
    {",", 20, 20, R, apply},
    {";", 30, 30, R, infix},
    {"\n", 40, 40, R, apply},
    {"=", 50, 50, N, apply},
    {"->", 240, 60, R, collapseLambda},
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
    {".", 250, 250, L, infix}
};

Operator getOperator(Node* token) {
    for (unsigned int i = 0; i < sizeof(OPERATORS)/sizeof(Operator); i++)
        if (isThisToken(token, OPERATORS[i].symbol))
            return OPERATORS[i];
    return DEFAULT;
}

bool isSpecialOperator(Node* token) {
    return getOperator(token).collapse != infix;
}
