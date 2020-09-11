Node* applyDefinition(Node* definition, Node* scope);
Node* reducePrefix(Tag tag, Node* left, Node* right);
Node* reduceInfix(Tag tag, Node* left, Node* right);
Node* reduceDefine(Tag tag, Node* left, Node* right);
Node* reduceADTDefinition(Tag tag, Node* left, Node* right);
Node* Printer(Tag tag);
