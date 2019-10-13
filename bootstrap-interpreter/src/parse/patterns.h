bool isValidPattern(Node* node);
unsigned int getArgumentCount(Node* application);
Node* newProjector(Tag tag, unsigned int size, unsigned int index);
Node* newLazyArrow(Node* left, Node* right);
Node* newCaseArrow(Node* left, Node* right);
Node* combineCases(Tag tag, Node* left, Node* right);
