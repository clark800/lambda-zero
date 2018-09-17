typedef enum {IN, PRE, POST, OPEN, CLOSE} Fixity;
typedef enum {L, R, N} Associativity;
typedef unsigned char Precedence;

bool isSpecial(Node* node);
bool isSpaceOperator(Node* node);
bool isOpenOperator(Node* node);
Fixity getFixity(Node* operator);
Node* reduceOperator(Node* operator, Node* left, Node* right);
Node* reduceBracket(Node* open, Node* close, Node* left, Node* right);
void shiftOperator(Stack* stack, Node* operator);
bool isHigherPrecedence(Node* left, Node* right);
Node* parseOperator(Tag tag, Stack* stack);
void addOperator(const char* symbol, Precedence leftPrecedence,
    Precedence rightPrecedence, Fixity fixity, Associativity associativity,
    void (*shift)(Stack*, Node*), Node* (*reduce)(Node*, Node*, Node*));
