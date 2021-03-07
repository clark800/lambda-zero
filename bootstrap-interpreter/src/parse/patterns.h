bool isValidPattern(Node* node);
unsigned int getArgumentCount(Node* application);
Node* newArrow(Node* left, Node* right);
Node* newCase(Node* left, Node* right);
Node* combineCases(Tag tag, Node* left, Node* right);
