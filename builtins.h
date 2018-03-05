#ifndef BUILTINS_H
#define BUILTINS_H

#include "lib/tree.h"
#include "lib/stack.h"

extern Node* NIL;
extern Node* TRUE;
extern Node* FALSE;
extern Node* YCOMBINATOR;
extern Node* PARAMETERX;
extern Node* REFERENCEX;
extern Node* PRINT;
extern Node* INPUT;

void initBuiltins(void);
void deleteBuiltins(void);

void evaluationErrorIf(bool condition, Node* token, const char* message);
unsigned long long lookupBuiltinCode(Node* token);

int getArity(Node* builtin);
Node* newNil(int location);
Node* prepend(Node* item, Node* list);
Node* evaluateBuiltin(Node* builtin, Stack* env);

#endif
