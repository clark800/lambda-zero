#include "lex.h"
#include "ast.h"
#include "parse.h"
#include "desugar.h"
#include "operators.h"

void infix(Stack* stack, Node* operator, Node* left, Node* right) {
    int location = getLocation(operator);
    convertOperatorToName(operator);
    push(stack, newBranchNode(location,
        newBranchNode(location, operator, left), right));
}

void apply(Stack* stack, Node* operator, Node* left, Node* right) {
    push(stack, newBranchNode(getLocation(operator), left, right));
}

void lambda(Stack* stack, Node* operator, Node* left, Node* right) {
    push(stack, transformLambdaSugar(operator, left, right));
}

void split(Stack* stack, Node* operator, Node* left, Node* right) {
    push(stack, left);
    int openLocation = collapseParentheses(stack, operator);
    push(stack, newOperator(openLocation));
    push(stack, right); // leave right on stack so parens won't be empty
}

Operator DEFAULT = {"", 150, 150, L, infix};

Operator OPERATORS[] = {
    {"(", 10, 10, R, NULL},
    {")", 10, 10, R, NULL},
    {"\0", 10, 10, R, NULL},
    {",", 20, 20, L, split},
    {"\n", 30, 30, R, apply},
    {"=", 40, 40, N, apply},
    {"|>", 50, 50, L, infix},
    {"<|", 50, 50, R, infix},
    {"!", 50, 50, R, infix},
    {"->", 240, 60, R, lambda},
    {"?", 70, 70, L, infix},
    {"then", 70, 70, L, apply},
    {"else", 70, 70, L, apply},
    {"\\/", 80, 80, R, infix},
    {"/\\", 90, 90, R, infix},
    {"==", 100, 100, N, infix},
    {"=/=", 100, 100, N, infix},
    {"===", 100, 100, N, infix},
    {"<=>", 100, 100, N, infix},
    {"<", 100, 100, N, infix},
    {">", 100, 100, N, infix},
    {"<=", 100, 100, N, infix},
    {">=", 100, 100, N, infix},
    {"<:", 100, 100, N, infix},
    {"in", 100, 100, N, infix},
    {"|", 120, 120, L, infix},
    {"<:>", 130, 130, R, infix},
    {"<+>", 140, 140, L, infix},
    {"++", 160, 160, R, infix},
    {"\\", 170, 170, L, infix},
    {"::", 180, 180, R, infix},
    {"~", 190, 190, N, infix},
    {"+", 200, 200, L, infix},
    {"-", 200, 200, L, infix},
    {"*", 210, 210, L, infix},
    {"/", 210, 210, L, infix},
    {"mod", 210, 210, L, infix},
    {"^", 220, 220, R, infix},
    {"<>", 230, 230, R, infix},
    {"@", 230, 230, L, infix},
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
