typedef enum {NOFIX=0, INFIX, PREFIX, POSTFIX, OPENFIX, CLOSEFIX} Fixity;
typedef enum {L, R, N} Associativity;
typedef unsigned char Precedence;
typedef Node* (*Reducer)(Tag tag, Node* left, Node* right);

Fixity getFixity(Node* op);
bool isOperator(Node* node);
bool isSpecialOperator(Node* op);
bool isHigherPrecedence(Node* left, Node* right);
unsigned char getSubprecedence(Node* op);
Node* reduce(Node* op, Node* left, Node* right);
Node* reduceBracket(Node* open, Node* close, Node* before, Node* contents);
Node* parseOperator(Tag tag, long long subprecedence);
Node* getBinaryPrefixInfixOperator(Node* op);


void initSyntax(void);
void addCoreSyntax(const char*, Precedence, Fixity, Associativity, Reducer);
void addSyntax(Tag, Node* prior, Precedence, Fixity, Associativity, Reducer);
void addBracketSyntax(const char*, char type, Precedence, Fixity, Reducer);
void addBinaryPrefixSyntax(const char*, Precedence, Reducer);
void addSyntaxCopy(String lexeme, Node* name, bool alias);
void addCoreAlias(const char* alias, const char* name);
void popSyntax(void);
