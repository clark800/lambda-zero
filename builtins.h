#ifndef BUILTINS_H
#define BUILTINS_H

#include "lib/tree.h"
#include "closure.h"

unsigned int getBuiltinArity(Node* builtin);
bool isStrictArgument(Node* builtin, unsigned int i);
Hold* evaluateBuiltin(Node* builtin, Closure* left, Closure* right);

#endif
