void debug(const char* message);
void debugLine(void);
void debugAST(Node* node);
void debugStack(Stack* stack, Node* (*select)(Node*));
