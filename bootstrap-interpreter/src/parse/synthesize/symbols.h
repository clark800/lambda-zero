typedef enum {INFIX, PREFIX, POSTFIX, OPENFIX, CLOSEFIX, SPACEFIX} Fixity;
typedef enum {L, R, N, RV} Associativity;
typedef unsigned char Precedence;
typedef struct NodeStack NodeStack;

NodeStack* newNodeStack(void);
void deleteNodeStack(NodeStack* stack);
Node* getTop(NodeStack* stack);

void erase(NodeStack* stack, const char* lexeme);
String getPrior(Node* operator);
Precedence findPrecedence(Node* node);

void shift(NodeStack* stack, Node* node);

Node* parseSymbol(Tag tag, long long value);
void addCoreSyntax(const char* symbol, Precedence leftPrecedence,
    Precedence rightPrecedence, Fixity fixity, Associativity associativity,
    Node* (*reduce)(Node*, Node*, Node*));
void addSyntax(Tag tag, String prior, Precedence leftPrecedence,
    Precedence rightPrecedence, Fixity fixity, Associativity associativity,
    Node* (*reducer)(Node*, Node*, Node*));
void addSyntaxCopy(String lexeme, Node* name, bool alias);
void addCoreAlias(const char* alias, const char* name);
void popSyntax(void);
void debugNodeStack(NodeStack* nodeStack, void debugNode(Node*));
