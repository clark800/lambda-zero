typedef enum {IDENTIFIER, PUNCTUATION, NUMBER, CHARACTER, STRING} TokenType;
typedef enum {TOKEN, OPERAND, OPERATOR, DEFINITION, COMMA, PIPE, ADT} ParseType;
typedef enum {NONE, APPLICATION, LAMBDA, REFERENCE, INTEGER, BUILTIN} NodeType;

// ====================================
// Functions to get a value from a node
// ====================================

static inline ParseType getParseType(Node* node) {
    return (ParseType)(getType(node) >> 4 & 0x0F);
}

static inline NodeType getNodeType(Node* node) {
    return (NodeType)(getType(node) & 0x0F);
}

static inline String getLexeme(Node* node) {return getTag(node).lexeme;}
static inline Node* getParameter(Node* lambda) {return getLeft(lambda);}
static inline Node* getBody(Node* lambda) {return getRight(lambda);}

static inline unsigned long long getDebruijnIndex(Node* reference) {
    assert(getValue(reference) > 0);
    return (unsigned long long)(getValue(reference) - 1);
}

static inline unsigned long long getGlobalIndex(Node* reference) {
    assert(getValue(reference) < 0);
    return (unsigned long long)(-getValue(reference) - 1);
}

// =============================================
// Functions to test if a node is a certain type
// =============================================

static inline bool isSameToken(Node* tokenA, Node* tokenB) {
    return isSameString(getLexeme(tokenA), getLexeme(tokenB));
}

static inline bool isThisToken(Node* token, const char* lexeme) {
    return isLeaf(token) && isThisString(getLexeme(token), lexeme);
}

static inline bool isOperand(Node* node) {return getParseType(node) == OPERAND;}
static inline bool isCommaList(Node* node) {return getParseType(node) == COMMA;}
static inline bool isPipePair(Node* node) {return getParseType(node) == PIPE;}
static inline bool isADT(Node* node) {return getParseType(node) == ADT;}

static inline bool isOperator(Node* node) {
    return getParseType(node) == OPERATOR;
}

static inline bool isDefinition(Node* node) {
    return getParseType(node) == DEFINITION;
}

static inline bool isLambda(Node* node) {return getNodeType(node) == LAMBDA;}
static inline bool isInteger(Node* node) {return getNodeType(node) == INTEGER;}
static inline bool isBuiltin(Node* node) {return getNodeType(node) == BUILTIN;}

static inline bool isApplication(Node* node) {
    return getNodeType(node) == APPLICATION;
}

static inline bool isReference(Node* node) {
    return getNodeType(node) == REFERENCE;
}

static inline bool isGlobalReference(Node* node) {
    return getNodeType(node) == REFERENCE && getValue(node) < 0;
}

static inline bool isNewline(Node* node) {return isThisToken(node, "\n");}
static inline bool isOpenParen(Node* node) {return isThisToken(node, "(");}
static inline bool isCloseParen(Node* node) {return isThisToken(node, ")");}
static inline bool isEOF(Node* node) {return isThisToken(node, "\0");}
static inline bool isComma(Node* node) {return isThisToken(node, ",");}
static inline bool isBlank(Node* node) {return isThisToken(node, "_");}

static inline bool isValidPattern(Node* node) {
    return isReference(node) || (isApplication(node) &&
        isValidPattern(getLeft(node)) && isValidPattern(getRight(node)));
}

// ================================
// Functions to construct new nodes
// ================================

static inline unsigned char makeType(ParseType parseType, NodeType nodeType) {
    return (unsigned char)(parseType << 4 | nodeType);
}

static inline Node* newToken(Tag tag, TokenType type) {
    return newLeaf(tag, makeType(TOKEN, NONE), type);
}

static inline Node* newOperator(Tag tag) {
    return newLeaf(tag, makeType(OPERATOR, NONE), 0);
}

static inline Node* newReference(Tag tag, long long value) {
    return newLeaf(tag, makeType(OPERAND, REFERENCE), value);
}

static inline Node* newName(Tag tag) {return newReference(tag, 0);}

static inline Node* newInteger(Tag tag, long long n) {
    return newLeaf(tag, makeType(OPERAND, INTEGER), n);
}

static inline Node* newBuiltin(Tag tag, long long n) {
    return newLeaf(tag, makeType(OPERAND, BUILTIN), n);
}

static inline Node* newEOF(void) {
    return newOperator(newTag(EMPTY, newLocation(0, 0)));
}

static inline Node* newLambda(Tag tag, Node* parameter, Node* body) {
    // we could make the left child of a lambda VOID, but storing a parameter
    // lets us decouple the location of the lambda from the parameter name,
    // which is useful in cases like string literals where we need to point
    // to the string literal for error messages, but we prefer not to make the
    // parameter name be the string literal
    assert(isReference(parameter) && getValue(parameter) == 0);
    return newBranch(tag, makeType(OPERAND, LAMBDA), parameter, body);
}

static inline Node* newApplication(Tag tag, Node* left, Node* right) {
    return newBranch(tag, makeType(OPERAND, APPLICATION), left, right);
}

static inline Node* newDefinition(Tag tag, Node* left, Node* right) {
    return newBranch(tag, makeType(DEFINITION, NONE), left, right);
}

static inline Node* newADT(Tag tag, Node* definitions) {
    return newBranch(tag, makeType(ADT, NONE), definitions, VOID);
}

static inline Node* newBlank(Tag tag) {return newName(renameTag(tag, "_"));}
static inline Node* newComma(Tag tag) {return newOperator(renameTag(tag, ","));}
static inline Node* newNil(Tag tag) {return newName(renameTag(tag, "[]"));}

static inline Node* newBlankReference(Tag tag, unsigned long long debruijn) {
    return newReference(renameTag(tag, "_"), (long long)debruijn);
}

static inline Node* newCommaList(Tag tag, Node* left, Node* right) {
    return newBranch(tag, makeType(COMMA, NONE), left, right);
}

static inline Node* newPipePair(Tag tag, Node* left, Node* right) {
    return newBranch(tag, makeType(PIPE, NONE), left, right);
}

static inline Node* prepend(Tag tag, Node* item, Node* list) {
    Node* operator = newName(renameTag(tag, "::"));
    return newApplication(tag, newApplication(tag, operator, item), list);
}

static inline Node* newBoolean(Tag tag, bool value) {
    return newLambda(tag, newBlank(tag), newLambda(tag, newBlank(tag),
        value ? newBlankReference(tag, 1) : newBlankReference(tag, 2)));
}

// ====================================
// Helper functions
// ====================================

static inline unsigned int getCommaListLength(Node* node) {
    return !isCommaList(node) ? 1 : 1 + getCommaListLength(getLeft(node));
}

// ====================================
// Builtins
// ====================================

enum BuiltinCode {PLUS, MINUS, TIMES, DIVIDE, MODULUS,
      EQUAL, NOTEQUAL, LESSTHAN, GREATERTHAN, LESSTHANOREQUAL,
      GREATERTHANOREQUAL, ERROR, EXIT, PUT, GET};

static inline Node* convertOperator(Node* operator) {
    // names in builtins must line up with codes in BuiltinCode, except
    // EXIT, PUT, GET which don't have accessible names
    static const char* const builtins[] =
        {"+", "-", "*", "/", "%", "=", "!=", "<", ">", "<=", ">=", "error"};
    for (unsigned int i = 0; i < sizeof(builtins)/sizeof(char*); ++i)
        if (isThisToken(operator, builtins[i]))
            return newBuiltin(getTag(operator), i);
    return newName(getTag(operator));
}

static inline Node* newPrinter(Tag tag) {
    Node* put = newBuiltin(renameTag(tag, "put"), PUT);
    Node* fold = newName(renameTag(tag, "fold"));
    Node* unit = newName(renameTag(tag, "()"));
    Node* blank = newBlankReference(tag, 1);

    Node* body = newApplication(tag, newApplication(tag, newApplication(tag,
        fold, blank), put), unit);
    return newLambda(tag, newBlank(tag), body);
}
