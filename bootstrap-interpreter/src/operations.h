extern Stack* INPUT_STACK;
unsigned int getArity(Term* operation);
Hold* evaluateOperationTerm(Closure* operation, Closure* left, Closure* right);
