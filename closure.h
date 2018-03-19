#ifndef CLOSURE_H
#define CLOSURE_H

#include "lib/tree.h"

typedef Node Closure;

static inline Closure* newClosure(Node* term, Node* locals) {
    return newBranchNode(0, term, locals);
}

static inline Node* getTerm(Closure* closure) {
    return getLeft(closure);
}

static inline Node* getLocals(Closure* closure) {
    return getRight(closure);
}

static inline void setTerm(Closure* closure, Node* term) {
    setLeft(closure, term);
}

static inline void setLocals(Closure* closure, Node* locals) {
    setRight(closure, locals);
}

static inline void setClosure(Closure* closure, Closure* update) {
    setTerm(closure, getTerm(update));
    setLocals(closure, getLocals(update));
}

static inline Closure* newUpdate(Closure* closure) {
    return newClosure(VOID, closure);
}

static inline bool isUpdate(Closure* closure) {
    return getTerm(closure) == VOID;
}

static inline Closure* getUpdateClosure(Closure* update) {
    return getRight(update);
}

#endif
