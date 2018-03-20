#ifndef BUILTINS_H
#define BUILTINS_H

#include "lib/tree.h"
#include "lex.h"
#include "closure.h"

static inline void errorIf(bool condition, Node* token, const char* message) {
    if (condition)
        throwTokenError("Evaluation", message, token);
}

int getArity(Node* builtin);
Node* evaluateBuiltin(Node* builtin, Closure* left, Closure* right);

#endif
