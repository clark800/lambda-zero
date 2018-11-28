unsigned int getArgumentCount(Node* application);
Node* newProjector(Tag tag, unsigned int size, unsigned int index);
Node* newPatternLambda(Tag tag, Node* left, Node* right);
Node* reduceCase(Node* operator, Node* left, Node* right);
Node* combineCases(Node* left, Node* right);
