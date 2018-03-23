#ifndef OPERATORS_H
#define OPERATORS_H

#include <stdbool.h>
#include "lib/tree.h"

typedef struct Rules Rules;
typedef struct Operator {
    Node* token;
    Rules* rules;
} Operator;

static inline bool isCommaTuple(Node* node) {
    return isBranchNode(node) && isThisToken(node, ",");
}

Operator getOperator(Node* token, bool prefixOrOpen);
bool isSpecialOperator(Operator operator);
bool isPrefixOperator(Operator operator);
bool isOpenOperator(Operator operator);
bool isCloseOperator(Operator operator);
bool isInfixOperator(Operator operator);
bool isHigherPrecedence(Operator left, Operator right);
int getOperatorArity(Operator operator);
Node* applyOperator(Operator operator, Node* left, Node* right);

#endif
