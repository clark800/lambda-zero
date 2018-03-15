#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <stdio.h>
#include "lib/tree.h"
#include "lib/stack.h"

void serialize(Node* root, Node* env, FILE* stream);

void debug(const char* message);
void debugLine(void);
void debugAST(Node* node);
void debugInteger(long long n);
void debugStack(Stack* stack, Node* (*select)(Node*));

#endif
