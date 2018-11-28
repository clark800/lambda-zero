extern Stack* INPUT_STACK;
void runtimeError(const char* message, Closure* closure);
unsigned int getArity(Node* operation);
Hold* evaluateOperationNode(Closure* operation, Closure* left, Closure* right);
