typedef enum {IN, PRE, OPEN, OPENCALL, CLOSE} Fixity;

Node* setRules(Node* operator, bool isAfterOperator);
bool isSpecialOperator(Node* operator);
Fixity getFixity(Node* operator);
bool isHigherPrecedence(Node* left, Node* right);
Node* applyOperator(Node* operator, Node* left, Node* right);
Node* newPatternLambda(Node* operator, Node* left, Node* right);
bool isSpace(Node* token);
