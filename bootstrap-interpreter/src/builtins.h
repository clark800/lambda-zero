extern Stack* INPUT_STACK;
void runtimeError(const char* message, Closure* closure);
unsigned int getBuiltinArity(Node* builtin);
Hold* evaluateBuiltinNode(Closure* builtin, Closure* left, Closure* right);
