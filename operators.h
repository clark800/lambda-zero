typedef enum {IN, PRE, OPEN, CLOSE} Fixity;
typedef struct Rules Rules;
typedef struct Operator {
    Node* token;
    Rules* rules;
} Operator;

Operator getOperator(Node* token, bool prefixOrOpen);
bool isSpecialOperator(Operator operator);
Fixity getFixity(Operator operator);
bool isHigherPrecedence(Operator left, Operator right);
Node* applyOperator(Operator operator, Node* left, Node* right);
Node* newPatternLambda(Node* operator, Node* left, Node* right);
Node* infix(Node* operator, Node* left, Node* right);
int getTupleSize(Node* tuple);
bool isSpace(Node* token);
