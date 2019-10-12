Node* applyDefinition(Node* definition, Node* scope);
Node* reduceApply(Node* operator, Node* left, Node* right);
Node* reducePrefix(Node* operator, Node* left, Node* right);
Node* reduceInfix(Node* operator, Node* left, Node* right);
Node* reduceDefine(Node* operator, Node* left, Node* right);
Node* reduceADTDefinition(Node* operator, Node* left, Node* right);
Node* Printer(Tag tag);
