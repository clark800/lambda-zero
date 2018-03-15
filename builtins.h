#ifndef BUILTINS_H
#define BUILTINS_H

#include "lib/tree.h"

int getArity(Node* builtin);
Node* computeBuiltin(Node* builtin, long long left, long long right);

#endif
