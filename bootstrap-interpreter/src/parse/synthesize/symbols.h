typedef enum {NOFIX, INFIX, PREFIX, POSTFIX, OPENFIX, CLOSEFIX} Fixity;
typedef enum {L, R, N, RV} Associativity;
typedef unsigned char Precedence;

bool isSpecial(Node* node);
bool isOpenOperator(Node* node);
void erase(Stack* stack, const char* lexeme);
Fixity getFixity(Node* operator);

Node* reduce(Node* operator, Node* left, Node* right);
Node* reduceBracket(Node* open, Node* close, Node* left, Node* right);
void reduceLeft(Stack* stack, Node* operator);
void shift(Stack* stack, Node* node);

Node* parseSymbol(Tag tag, long long value);
void addCoreSyntax(const char* symbol, Precedence leftPrecedence,
    Precedence rightPrecedence, Fixity fixity, Associativity associativity,
    void (*shift)(Stack*, Node*), Node* (*reduce)(Node*, Node*, Node*));
void addSyntax(Tag tag, Precedence leftPrecedence, Precedence rightPrecedence,
    Fixity fixity, Associativity associativity, void (*shifter)(Stack*, Node*),
    Node* (*reducer)(Node*, Node*, Node*));
void addMixfixSyntax(Tag tag, Node* prior, void (*shifter)(Stack*, Node*));
void addSyntaxCopy(String lexeme, Node* name, bool alias);
void addCoreAlias(const char* alias, const char* name);
