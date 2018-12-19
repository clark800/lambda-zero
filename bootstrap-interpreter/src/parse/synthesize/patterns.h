unsigned int getArgumentCount(Node* application);
Node* newProjector(Tag tag, unsigned int size, unsigned int index);
Node* newLazyArrow(Tag tag, Node* left, Node* right);
Node* newStrictArrow(Tag tag, Node* left, Node* right);
Node* combineCases(Tag tag, Node* left, Node* right);
