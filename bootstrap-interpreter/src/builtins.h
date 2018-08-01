extern Stack* INPUT_STACK;
void runtimeError(const char* message, Closure* closure);
unsigned int getBuiltinArity(Node* builtin);
bool isStrictArgument(Node* builtin, unsigned int i);
Hold* evaluateBuiltin(Closure* builtin, Closure* left, Closure* right);
