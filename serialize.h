#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <stdio.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "lib/array.h"

void serialize(Node* root, Node* locals, const Array* globals, FILE* stream);

void debug(const char* message);
void debugLine(void);
void debugAST(Node* node);
void debugInteger(long long n);
void debugStack(Stack* stack, Node* (*select)(Node*));

#endif
