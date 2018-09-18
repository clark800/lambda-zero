Node* reduceParentheses(Node* close, Node* open, Node* contents);
Node* reduceSquareBrackets(Node* close, Node* open, Node* contents);
Node* reduceCurlyBrackets(Node* close, Node* open, Node* patterns);
Node* reduceEOF(Node* operator, Node* open, Node* contents);
Node* reduceUnmatched(Node* operator, Node* left, Node* right);
void shiftOpen(Stack* stack, Node* open);
void shiftClose(Stack* stack, Node* close);
void shiftOpenCurly(Stack* stack, Node* operator);
