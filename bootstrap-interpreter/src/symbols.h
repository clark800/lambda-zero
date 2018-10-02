typedef enum {NOFIX, INFIX, PREFIX, POSTFIX, OPENFIX, CLOSEFIX} Fixity;
typedef enum {L, R, N} Associativity;
typedef unsigned char Precedence;

bool isOperator(Node* node);
bool isSpecial(Node* node);
bool isOpenOperator(Node* node);
void erase(Stack* stack, const char* lexeme);
void eraseWhitespace(Stack* stack);
Fixity getFixity(Node* operator);

Node* reduce(Node* operator, Node* left, Node* right);
Node* reduceBracket(Node* open, Node* close, Node* left, Node* right);
void reduceLeft(Stack* stack, Node* operator);
void shift(Stack* stack, Node* node);

Node* parseSymbol(Tag tag);
void addBuiltinSyntax(const char* symbol, Precedence leftPrecedence,
    Precedence rightPrecedence, Fixity fixity, Associativity associativity,
    void (*shift)(Stack*, Node*), Node* (*reduce)(Node*, Node*, Node*));
void addSyntax(Tag tag, Precedence leftPrecedence,
        Precedence rightPrecedence, Fixity fixity, Associativity associativity,
        void (*shifter)(Stack*, Node*), Node* (*reducer)(Node*, Node*, Node*));
void addScopeMarker(void);
void endScope(void);
