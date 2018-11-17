Node* reduceDefine(Node* operator, Node* left, Node* right);
Node* applyDefinition(Tag tag, Node* left, Node* right, Node* scope);
Node* applyTryDefinition(Tag tag, Node* left, Node* right, Node* scope);
Node* reduceADTDefinition(Node* operator, Node* left, Node* right);
Node* applyADTDefinition(Tag tag, Node* left, Node* right, Node* scope);
