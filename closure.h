#ifndef CLOSURE_H
#define CLOSURE_H

#include "lib/tree.h"

static inline Node* newClosure(Node* term, Node* env) {
    return newBranchNode(0, term, env);
}

static inline Node* getClosureTerm(Node* closure) {
    return getLeft(closure);
}

static inline Node* getClosureEnv(Node* closure) {
    return getRight(closure);
}

static inline Node* newUpdateClosure(Node* closure) {
    return newBranchNode(0, NULL, closure);
}

static inline bool isUpdateClosure(Node* node) {
    return getLeft(node) == NULL && getRight(node) != NULL;
}

#endif
