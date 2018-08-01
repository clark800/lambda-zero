typedef Node Closure;

Closure* newClosure(Node* term, Node* locals, Node* trace);
Node* getTerm(Closure* closure);
Node* getLocals(Closure* closure);
Node* getTrace(Closure* closure);
Node* getBacktrace(Closure* closure);
void setTerm(Closure* closure, Node* term);
void setLocals(Closure* closure, Node* locals);
void updateClosure(Closure* closure, Closure* update);
void setClosure(Closure* closure, Closure* update);
Closure* newUpdate(Closure* closure);
bool isUpdate(Closure* closure);
Closure* getUpdateClosure(Closure* update);
