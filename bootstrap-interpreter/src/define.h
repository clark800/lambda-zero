Node* reduceDefine(Node* operator, Node* left, Node* right);
Node* reduceADTDefinition(Node* operator, Node* left, Node* right);
Node* applyDefinition(Tag tag, Node* name, Node* value, Node* scope);
Node* applyADTDefinition(Tag tag, Node* left, Node* right, Node* scope);
