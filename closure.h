typedef Node Closure;

static inline Closure* newClosure(Node* term, Node* locals, Node* trace) {
    return newBranch(0, newBranch(0, term, trace), locals);
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

static inline Node* getBacktrace(Closure* closure) {
    return getLeft(closure);        // can be cast to a Stack
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

static inline Closure* newUpdate(Closure* closure) {
    return newClosure(VOID, closure, VOID);
}

static inline bool isUpdate(Closure* closure) {
    return getTerm(closure) == VOID;
}

static inline Closure* getUpdateClosure(Closure* update) {
    return getLocals(update);
}
