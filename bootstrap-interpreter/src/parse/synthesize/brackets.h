Node* reduceParentheses(Node* close, Node* open, Node* contents);
Node* reduceSquareBrackets(Node* close, Node* open, Node* contents);
Node* reduceCurlyBrackets(Node* close, Node* open, Node* patterns);
Node* reduceEOF(Node* operator, Node* open, Node* contents);
Node* reduceUnmatched(Node* operator, Node* left, Node* right);
