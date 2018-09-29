Node* getHead(Node* node);
unsigned int getArgumentCount(Node* application);
Node* newProjector(Tag tag, unsigned int size, unsigned int index);
Node* reduceLambda(Node* operator, Node* left, Node* right);
Node* reducePatternLambda(Node* operator, Node* left, Node* right);
