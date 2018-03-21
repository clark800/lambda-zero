#ifndef OPERATORS_H
#define OPERATORS_H

#include <stdbool.h>
#include "lib/tree.h"

typedef struct Rules Rules;
typedef struct Operator {
    Node* token;
    Rules* rules;
} Operator;

Operator getOperator(Node* token, bool prefixOnly);
bool isSpecialOperator(Operator operator);
bool isPrefixOperator(Operator operator);
bool isHigherPrecedence(Operator left, Operator right);
int getOperatorArity(Operator operator);
Node* applyOperator(Operator operator, Node* left, Node* right);

#endif
