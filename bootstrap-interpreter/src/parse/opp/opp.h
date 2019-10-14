// operator precedence parser

typedef enum {INFIX, PREFIX, POSTFIX, OPENFIX, CLOSEFIX} Fixity;
typedef enum {L, R, N} Associativity;
typedef unsigned char Precedence;
typedef struct NodeStack NodeStack;

NodeStack* newNodeStack(void);
void deleteNodeStack(NodeStack* stack);
Node* getTop(NodeStack* stack);
bool isOperator(Node* node);
bool isThisOperator(Node* node, const char* lexeme);

void erase(NodeStack* stack, const char* lexeme);
String getPrior(Node* operator);
Precedence findPrecedence(Node* node);
bool isLeftSectionOperator(Node* op);
bool isRightSectionOperator(Node* op);
bool isOpenOperator(Node* node);
bool isCloseOperator(Node* node);

void shift(NodeStack* stack, Node* node);

Node* parseOperator(Tag tag, long long subprecedence);

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
void debugNodeStack(NodeStack* nodeStack, void debugNode(Node*));
