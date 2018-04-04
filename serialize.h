#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "lib/tree.h"
#include "lib/stack.h"
#include "lib/array.h"
#include "closure.h"

void serialize(Closure* closure, const Array* globals);

void debug(const char* message);
void debugLine(void);
void debugAST(Node* node);
void debugStack(Stack* stack, Node* (*select)(Node*));

#endif
