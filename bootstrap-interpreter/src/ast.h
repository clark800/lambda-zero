typedef enum {NONE, REFERENCE, LAMBDA, APPLICATION, INTEGER, BUILTIN} NodeType;

// ====================================
// Functions to get a value from a node
// ====================================

static inline NodeType getNodeType(Node* node) {return (NodeType)getType(node);}
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
    return isThisString(getLexeme(token), lexeme);
}

static inline bool isThisLeaf(Node* leaf, const char* lexeme) {
    return isLeaf(leaf) && isThisToken(leaf, lexeme);
}

static inline bool isOperator(Node* node) {return getNodeType(node) == NONE;}
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

static inline bool isEOF(Node* node) {return isThisLeaf(node, "\0");}
static inline bool isBlank(Node* node) {return isThisLeaf(node, "_");}

static inline bool isSection(Node* node) {
    return isThisToken(node, "_._") ||
        isThisToken(node, "_.") || isThisToken(node, "._");
}

// ================================
// Functions to construct new nodes
// ================================

static inline Node* newOperator(Tag tag) {return newLeaf(tag, NONE, 0);}

static inline Node* newReference(Tag tag, long long value) {
    return newLeaf(tag, REFERENCE, value);
}

static inline Node* newName(Tag tag) {return newReference(tag, 0);}

static inline Node* newInteger(Tag tag, long long n) {
    return newLeaf(tag, INTEGER, n);
}

static inline Node* newBuiltin(Tag tag, long long n) {
    return newLeaf(tag, BUILTIN, n);
}

static inline Node* newLambda(Tag tag, Node* parameter, Node* body) {
    // we could make the left child of a lambda VOID, but storing a parameter
    // lets us decouple the location of the lambda from the parameter name,
    // which is useful in cases like string literals where we need to point
    // to the string literal for error messages, but we prefer not to make the
    // parameter name be the string literal
    assert(isReference(parameter) && getValue(parameter) == 0);
    return newBranch(tag, LAMBDA, parameter, body);
}

static inline Node* newApplication(Tag tag, Node* left, Node* right) {
    return newBranch(tag, APPLICATION, left, right);
}

static inline Node* newNil(Tag tag) {return newName(renameTag(tag, "[]"));}
static inline Node* newBlank(Tag tag) {return newName(renameTag(tag, "_"));}

static inline Node* newBlankReference(Tag tag, unsigned long long debruijn) {
    return newReference(renameTag(tag, "_"), (long long)debruijn);
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
        if (isThisLeaf(operator, builtins[i]))
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
