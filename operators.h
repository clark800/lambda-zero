#ifndef OPERATORS_H
#define OPERATORS_H

#include <stdbool.h>
#include "lib/tree.h"

typedef unsigned char Precedence;
typedef enum {L, R, N, P} Associativity;
typedef struct Operator {
    const char* symbol;
    Precedence leftPrecedence;
    Precedence rightPrecedence;
    Associativity associativity;
    Node* (*collapse)(Node* operator, Node* left, Node* right);
} Operator;

Operator getOperator(Node* token, bool prefixOnly);
bool isParenthesizableOperator(Node* token);
bool isSectionableOperator(Node* token);
bool isAlwaysPrefixOperator(Node* token);
bool isAlwaysInfixOperator(Node* token);

#endif
