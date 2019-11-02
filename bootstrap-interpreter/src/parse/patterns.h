bool isValidPattern(Node* node);
unsigned int getArgumentCount(Node* application);
Node* newLazyArrow(Node* left, Node* right);
Node* newCaseArrow(Node* left, Node* right);
Node* combineCases(Tag tag, Node* left, Node* right);
