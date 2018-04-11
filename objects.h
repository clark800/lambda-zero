#ifndef OBJECTS_H
#define OBJECTS_H

#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"

extern const char* INTERNAL_CODE;
extern Stack* INPUT_STACK;
extern Node *IDENTITY, *TRUE, *FALSE, *YCOMBINATOR, *PRINT, *INPUT;

void initObjects(Program program);
void deleteObjects(void);

Node* newNil(int location);
Node* prepend(int location, Node* item, Node* list);
Node* newUnit(int location);
Node* newSingleton(int location, Node* item);

#endif
