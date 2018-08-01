#include "lib/tree.h"
#include "errors.h"
#include "closure.h"

Closure* newClosure(Node* term, Node* locals, Node* trace) {
    if (TEST)
        return newBranch(EMPTY, term, locals);
    return newBranch(EMPTY, newBranch(EMPTY, term, trace), locals);
}

Node* getTerm(Closure* closure) {
    return TEST ? getLeft(closure) : getLeft(getLeft(closure));
}

Node* getLocals(Closure* closure) {
    return getRight(closure);
}

Node* getTrace(Closure* closure) {
    return TEST ? VOID : getRight(getLeft(closure));
}

Node* getBacktrace(Closure* closure) {
    return TEST ? VOID : getLeft(closure);        // can be cast to a Stack
}

void setTerm(Closure* closure, Node* term) {
    if (TEST)
        setLeft(closure, term);
    else
        setLeft(getLeft(closure), term);
}

void setLocals(Closure* closure, Node* locals) {
    setRight(closure, locals);
}

void updateClosure(Closure* closure, Closure* update) {
    setTerm(closure, getTerm(update));
    setLocals(closure, getLocals(update));
}

void setClosure(Closure* closure, Closure* update) {
    setTerm(closure, getTerm(update));
    if (!TEST)
        setRight(getLeft(closure), getTrace(update));
    // warning: closures are stored in locals, so if you update locals
    // first, the trace may be erased before it can be copied!
    setLocals(closure, getLocals(update));
}

Closure* newUpdate(Closure* closure) {
    return newClosure(VOID, closure, VOID);
}

bool isUpdate(Closure* closure) {
    return getTerm(closure) == VOID;
}

Closure* getUpdateClosure(Closure* update) {
    return getLocals(update);
}
