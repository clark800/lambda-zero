#ifndef OPERATORS_H
#define OPERATORS_H

#include <stdbool.h>
#include "lib/tree.h"

typedef enum {IN, PRE, OPEN, CLOSE} Fixity;
typedef struct Rules Rules;
typedef struct Operator {
    Node* token;
    Rules* rules;
} Operator;

Operator getOperator(Node* token, bool prefixOrOpen);
bool isSpecialOperator(Operator operator);
Fixity getFixity(Operator operator);
bool isHigherPrecedence(Operator left, Operator right);
Node* applyOperator(Operator operator, Node* left, Node* right);

#endif
