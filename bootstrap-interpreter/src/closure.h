typedef Node Closure;
extern bool TEST;

static inline Closure* newClosure(Node* term, Node* locals, Node* trace) {
    return TEST ? newPair(term, locals) : newPair(newPair(term, trace), locals);
}

static inline Node* getTerm(Closure* closure) {
    return TEST ? getLeft(closure) : getLeft(getLeft(closure));
}

static inline Node* getLocals(Closure* closure) {
    return getRight(closure);
}

static inline Node* getTrace(Closure* closure) {
    return TEST ? VOID : getRight(getLeft(closure));
}

static inline Node* getBacktrace(Closure* closure) {
    return TEST ? VOID : getLeft(closure);        // can be cast to a Stack
}

static inline void setTerm(Closure* closure, Node* term) {
    setLeft(TEST ? closure : getLeft(closure), term);
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
    if (!TEST)
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
