#ifndef BUILTINS_H
#define BUILTINS_H

#include "lib/tree.h"
#include "lex.h"
#include "closure.h"

static inline void errorIf(bool condition, Node* token, const char* message) {
    if (condition)
        throwTokenError("Evaluation", message, token);
}

unsigned int getBuiltinArity(Node* builtin);
bool isStrictArgument(Node* builtin, unsigned int i);
Hold* evaluateBuiltin(Node* builtin, Closure* left, Closure* right);

#endif
