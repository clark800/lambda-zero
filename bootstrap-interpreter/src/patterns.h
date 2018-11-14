bool isCase(Node* node);
unsigned int getArgumentCount(Node* application);
Node* newProjector(Tag tag, unsigned int size, unsigned int index);
Node* newPatternLambda(Tag tag, Node* left, Node* right);
Node* newCase(Tag tag, Node* left, Node* right);
Node* combineCases(Tag tag, Node* left, Node* right);
