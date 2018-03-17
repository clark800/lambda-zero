#ifndef CLOSURE_H
#define CLOSURE_H

#include "lib/tree.h"

static inline Node* newClosure(Node* term, Node* locals) {
    return newBranchNode(0, term, locals);
}

static inline Node* getTerm(Node* closure) {
    return getLeft(closure);
}

static inline Node* getLocals(Node* closure) {
    return getRight(closure);
}

static inline void updateClosure(Node* closure, Node* term, Node* locals) {
    setLeft(closure, term);
    setRight(closure, locals);
}

#endif
