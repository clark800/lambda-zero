typedef enum {INFIX, PREFIX, POSTFIX, OPENFIX, CLOSEFIX} Fixity;
typedef enum {L, R, N} Associativity;
typedef unsigned char Precedence;

Fixity getFixity(Node* op);
char getBracketType(Node* op);

bool isOperator(Node* node);
bool isLeftSectionOperator(Node* op);
bool isRightSectionOperator(Node* op);
bool isOpenOperator(Node* op);
bool isCloseOperator(Node* op);
bool isHigherPrecedence(Node* left, Node* right);

Node* parseOperator(Tag tag, long long subprecedence);
Node* reduce(Node* op, Node* left, Node* right);


void initSyntax(void);
void addCoreSyntax(const char* symbol, Precedence leftPrecedence,
    Precedence rightPrecedence, Fixity fixity, Associativity associativity,
    Node* (*reduce)(Node*, Node*, Node*));
void addBracketSyntax(const char* symbol, char type, Precedence outerPrecedence,
    Fixity fixity, Node* (*reducer)(Node*, Node*, Node*));
void addSyntax(Tag tag, String prior, Precedence leftPrecedence,
    Precedence rightPrecedence, Fixity fixity, Associativity associativity,
    Node* (*reducer)(Node*, Node*, Node*));
void addSyntaxCopy(String lexeme, Node* name, bool alias);
void addCoreAlias(const char* alias, const char* name);
void popSyntax(void);
