#ifndef CLOSURE_H
#define CLOSURE_H

#include "lib/tree.h"
#include "lib/stack.h"

typedef Node Closure;

static inline Closure* newClosure(Node* term, Node* locals, Node* trace) {
    Node* left = newBranchNode(0, term, trace);
    return newBranchNode(0, left, locals);
}

static inline Node* getTerm(Closure* closure) {
    return getLeft(getLeft(closure));
}

static inline Node* getLocals(Closure* closure) {
    return getRight(closure);
}

static inline Node* getTrace(Closure* closure) {
    return getRight(getLeft(closure));
}

static inline Stack* getBacktrace(Closure* closure) {
    return (Stack*)getLeft(closure);
}

static inline void setTerm(Closure* closure, Node* term) {
    setLeft(getLeft(closure), term);
}

static inline void setLocals(Closure* closure, Node* locals) {
    setRight(closure, locals);
}

static inline void updateClosure(Closure* closure, Closure* update) {
    setTerm(closure, getTerm(update));
    setLocals(closure, getLocals(update));
}

static inline void setClosure(Closure* closure, Closure* update) {
    setTerm(closure, getTerm(update));
    setRight(getLeft(closure), getTrace(update));
    // warning: closures are stored in locals, so if you update locals
    // first, the trace may be erased before it can be copied!
    setLocals(closure, getLocals(update));
}

static inline void pushLocal(Closure* closure, Closure* local) {
    push((Stack*)closure, local);
}

static inline Closure* newUpdate(Closure* closure) {
    return newClosure(VOID, closure, VOID);
}

static inline bool isUpdate(Closure* closure) {
    return getTerm(closure) == VOID;
}

static inline Closure* getUpdateClosure(Closure* update) {
    return getLocals(update);
}

#endif
