// operator precedence parser

typedef struct NodeStack NodeStack;
NodeStack* newNodeStack(void);
void deleteNodeStack(NodeStack* stack);
void shift(NodeStack* stack, Node* node);
void erase(NodeStack* stack, const char* lexeme);
Node* getTop(NodeStack* stack);
void debugNodeStack(NodeStack* nodeStack, void debugNode(Node*));
