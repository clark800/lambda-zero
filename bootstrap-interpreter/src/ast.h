typedef enum
    {SYMBOL, LAMBDA, APPLICATION, DEFINITION, NATURAL, BUILTIN} NodeType;

// ====================================
// Functions to get a value from a node
// ====================================

static inline NodeType getNodeType(Node* node) {return (NodeType)getType(node);}
static inline String getLexeme(Node* node) {return getTag(node).lexeme;}
static inline Node* getParameter(Node* lambda) {return getLeft(lambda);}
static inline Node* getBody(Node* lambda) {return getRight(lambda);}
static inline long long getBuiltinCode(Node* node) {return getValue(node);}

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

static inline bool isSameLexeme(Node* a, Node* b) {
    return isSameString(getLexeme(a), getLexeme(b));
}

static inline bool isThisLexeme(Node* node, const char* lexeme) {
    return isThisString(getLexeme(node), lexeme);
}

static inline bool isThisLeaf(Node* leaf, const char* lexeme) {
    return isLeaf(leaf) && isThisLexeme(leaf, lexeme);
}

static inline bool isSymbol(Node* node) {return getNodeType(node) == SYMBOL;}
static inline bool isLambda(Node* node) {return getNodeType(node) == LAMBDA;}
static inline bool isNatural(Node* node) {return getNodeType(node) == NATURAL;}
static inline bool isBuiltin(Node* node) {return getNodeType(node) == BUILTIN;}

static inline bool isApplication(Node* node) {
    return getNodeType(node) == APPLICATION;
}

static inline bool isDefinition(Node* node) {
    return getNodeType(node) == DEFINITION;
}

static inline bool isGlobalReference(Node* node) {
    return isSymbol(node) && getValue(node) < 0;
}

static inline bool isEOF(Node* node) {return isThisLeaf(node, "\0");}
static inline bool isUnderscore(Node* node) {return isThisLeaf(node, "_");}

static inline bool isUnused(Node* node) {
    return getLexeme(node).start[0] == '_';
}

static inline bool isIdentity(Node* node) {
    return isLambda(node) && isSymbol(getBody(node)) &&
        isSameLexeme(getParameter(node), getBody(node));
}

static inline bool isSection(Node* node) {
    return isThisLexeme(node, ".*.") ||
        isThisLexeme(node, ".*") || isThisLexeme(node, "*.");
}

static inline bool isAsPattern(Node* node) {
    return isApplication(node) && isThisLexeme(node, "@");
}

// ================================
// Functions to construct new nodes
// ================================


static inline Node* newSymbol(Tag tag, long long value, void* rules) {
    return newLeaf(tag, SYMBOL, value, rules);
}

static inline Node* newName(Tag tag) {return newSymbol(tag, 0, NULL);}

static inline Node* newUnderscore(Tag tag, unsigned long long debruijn) {
    return newSymbol(renameTag(tag, "_"), (long long)debruijn, NULL);
}

static inline Node* newNatural(Tag tag, long long n) {
    return newLeaf(tag, NATURAL, n, NULL);
}

static inline Node* newBuiltin(Tag tag, long long n) {
    return newLeaf(tag, BUILTIN, n, NULL);
}

static inline bool isValidParameter(Node* node) {
    return isNatural(node) || (isSymbol(node) && getValue(node) == 0);
}

static inline Node* newLambda(Tag tag, Node* parameter, Node* body) {
    // we could make the left child of a lambda VOID, but storing a parameter
    // lets us decouple the location of the lambda from the parameter name,
    // which is useful in cases like string literals where we need to point
    // to the string literal for error messages, but we prefer not to make the
    // parameter name be the string literal
    assert(isValidParameter(parameter));
    return newBranch(tag, LAMBDA, parameter, body);
}

static inline Node* newApplication(Tag tag, Node* left, Node* right) {
    return newBranch(tag, APPLICATION, left, right);
}

static inline Node* newDefinition(Tag tag, Node* left, Node* right) {
    return newBranch(tag, DEFINITION, left, right);
}

static inline Node* newNil(Tag tag) {return newName(renameTag(tag, "[]"));}

static inline Node* prepend(Tag tag, Node* item, Node* list) {
    Node* operator = newName(renameTag(tag, "::"));
    return newApplication(tag, newApplication(tag, operator, item), list);
}

static inline Node* newBoolean(Tag tag, bool value) {
    Node* underscore = newUnderscore(tag, 0);
    return newLambda(tag, underscore, newLambda(tag, underscore,
        value ? newUnderscore(tag, 1) : newUnderscore(tag, 2)));
}

// ====================================
// Builtins
// ====================================

enum BuiltinCode {PLUS, MINUS, TIMES, DIVIDE, MODULO,
      EQUAL, NOTEQUAL, LESSTHAN, GREATERTHAN, LESSTHANOREQUAL,
      GREATERTHANOREQUAL, INCREMENT, ERROR, UNDEFINED, EXIT, PUT, GET};

static inline Node* convertOperator(Tag tag) {
    // names in builtins must line up with codes in BuiltinCode, except
    // UNDEFINED, EXIT, PUT, GET which don't have accessible names
    static const char* const builtins[] = {"+", "-", "*", "//", "%",
        "=", "=/=", "<", ">", "<=", ">=", "up", "error"};
    for (unsigned int i = 0; i < sizeof(builtins)/sizeof(char*); ++i)
        if (isThisString(tag.lexeme, builtins[i]))
            return newBuiltin(tag, i);
    return newName(tag);
}

static inline Node* newPrinter(Tag tag) {
    Node* put = newBuiltin(renameTag(tag, "put"), PUT);
    Node* fold = newName(renameTag(tag, "fold"));
    Node* unit = newName(renameTag(tag, "()"));
    Node* under = newUnderscore(tag, 1);
    return newLambda(tag, newUnderscore(tag, 0), newApplication(tag,
        newApplication(tag, newApplication(tag, fold, under), put), unit));
}
