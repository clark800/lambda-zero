#ifndef OBJECTS_H
#define OBJECTS_H

#include "lib/tree.h"
#include "ast.h"

extern const char* INTERNAL_CODE;
extern Node *IDENTITY, *TRUE, *FALSE, *YCOMBINATOR, *PRINT, *INPUT;

void initObjects(Program program);
void deleteObjects(void);

Node* newNil(int location);
Node* prepend(Node* item, Node* list);

#endif
