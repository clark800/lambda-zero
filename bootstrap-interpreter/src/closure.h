typedef Node Closure;
extern bool TRACE;

static inline Closure* newClosure(Term* term, Node* locals, Node* trace) {
    return !TRACE ? newPair(term, locals) : newPair(newPair(term, trace), locals);
}

static inline Term* getTerm(Closure* closure) {
    return !TRACE ? getLeft(closure) : getLeft(getLeft(closure));
}

static inline Node* getLocals(Closure* closure) {
    return getRight(closure);
}

static inline Node* getTrace(Closure* closure) {
    return !TRACE ? VOID : getRight(getLeft(closure));
}

static inline Node* getBacktrace(Closure* closure) {
    return !TRACE ? VOID : getLeft(closure);        // can be cast to a Stack
}

static inline void setTerm(Closure* closure, Term* term) {
    setLeft(!TRACE ? closure : getLeft(closure), term);
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
    if (TRACE)
        setRight(getLeft(closure), getTrace(update));
    // warning: closures are stored in locals, so if you update locals
    // first, the trace may be erased before it can be copied!
    setLocals(closure, getLocals(update));
}
