extern bool TRACE_EVALUATION;

Hold* evaluateClosure(Closure* closure, const Array* globals);
Hold* evaluateTerm(Node* term, const Array* globals);
