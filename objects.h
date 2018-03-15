#ifndef OBJECTS_H
#define OBJECTS_H

#include <stdbool.h>
#include "lib/tree.h"

extern bool IO;
extern const char* OBJECTS;

extern Node* IDENTITY;
extern Node* NIL;
extern Node* TRUE;
extern Node* FALSE;
extern Node* YCOMBINATOR;
extern Node* PARAMETERX;
extern Node* REFERENCEX;
extern Node* PRINT;
extern Node* INPUT;
extern Node* GET_BUILTIN;

void initObjects(Hold* objectsHold);
void deleteObjects(void);

Node* newNil(int location);
Node* prepend(Node* item, Node* list);

#endif
