// OPERAND, OPERATOR, DEFINTION, COMMALIST exist during parsing only
typedef enum {OPERAND, OPERATOR, DEFINITION, COMMALIST,
    APPLICATION, LAMBDA, REFERENCE, INTEGER, BUILTIN, GLOBAL} NodeType;
typedef enum {NAME, NUMERIC, CHARACTER, STRING, CONVERSION} OperandType;

// names in BUILTINS must line up with codes in BuiltinCode, except
// EXIT, PUT, GET which don't have accessible names
static const char* const BUILTINS[] =
    {"+", "-", "*", "/", "%", "=", "!=", "<", ">", "<=", ">=", "error"};

enum BuiltinCode {PLUS, MINUS, TIMES, DIVIDE, MODULUS,
      EQUAL, NOTEQUAL, LESSTHAN, GREATERTHAN, LESSTHANOREQUAL,
      GREATERTHANOREQUAL, ERROR, EXIT, PUT, GET};

// ====================================
// Functions to get a value from a node
// ====================================

static inline String getLexeme(Node* node) {return getTag(node).lexeme;}
static inline NodeType getNodeType(Node* node) {return (NodeType)getType(node);}
static inline Node* getParameter(Node* lambda) {return getLeft(lambda);}
static inline Node* getBody(Node* lambda) {return getRight(lambda);}

// =============================================
// Functions to test if a node is a certain type
// =============================================

static inline bool isSameToken(Node* tokenA, Node* tokenB) {
    return isSameString(getLexeme(tokenA), getLexeme(tokenB));
}

static inline bool isThisToken(Node* token, const char* lexeme) {
    return isLeaf(token) && isThisString(getLexeme(token), lexeme);
}

static inline bool isOperator(Node* node) {return getType(node) == OPERATOR;}
static inline bool isCommaList(Node* node) {return getType(node) == COMMALIST;}
static inline bool isLambda(Node* node) {return getType(node) == LAMBDA;}
static inline bool isReference(Node* node) {return getType(node) == REFERENCE;}
static inline bool isInteger(Node* node) {return getType(node) == INTEGER;}
static inline bool isBuiltin(Node* node) {return getType(node) == BUILTIN;}
static inline bool isGlobal(Node* node) {return getType(node) == GLOBAL;}

static inline bool isName(Node* node) {
    return getType(node) == OPERAND &&
        (getValue(node) == NAME || getValue(node) == CONVERSION);
}

static inline bool isApplication(Node* node) {
    return getType(node) == APPLICATION;
}

static inline bool isDefinition(Node* node) {
    return getType(node) == DEFINITION;
}

static inline bool isNewline(Node* node) {return isThisToken(node, "\n");}
static inline bool isOpenParen(Node* node) {return isThisToken(node, "(");}
static inline bool isCloseParen(Node* node) {return isThisToken(node, ")");}
static inline bool isEOF(Node* node) {return isThisToken(node, "\0");}
static inline bool isComma(Node* node) {return isThisToken(node, ",");}
static inline bool isBlank(Node* node) {return isThisToken(node, "_");}

// ================================
// Functions to construct new nodes
// ================================

static inline Node* newOperand(Tag tag, OperandType type) {
    return newLeaf(tag, OPERAND, type);
}

static inline Node* newName(Tag tag) {return newOperand(tag, NAME);}
static inline Node* newOperator(Tag tag) {return newLeaf(tag, OPERATOR, 0);}

static inline Node* newInteger(Tag tag, long long n) {
    return newLeaf(tag, INTEGER, n);
}

static inline Node* newBuiltin(Tag tag, long long n) {
    return newLeaf(tag, BUILTIN, n);
}

static inline Node* newEOF(void) {
    return newOperator(newTag(EMPTY, newLocation(0, 0)));
}

static inline Node* newReference(Tag tag, unsigned long long debruijn) {
    return newLeaf(tag, REFERENCE, (long long)debruijn);
}

static inline Node* newLambda(Tag tag, Node* parameter, Node* body) {
    // we could make the left child of a lambda VOID, but storing a parameter
    // lets us decouple the location of the lambda from the parameter name,
    // which is useful in cases like string literals where we need to point
    // to the string literal for error messages, but we prefer not to make the
    // parameter name be the string literal
    assert(isName(parameter));
    return newBranch(tag, LAMBDA, parameter, body);
}

static inline Node* newApplication(Tag tag, Node* left, Node* right) {
    return newBranch(tag, APPLICATION, left, right);
}

static inline Node* newDefinition(Tag tag, Node* left, Node* right) {
    return newBranch(tag, DEFINITION, left, right);
}

static inline Node* newBoolean(Tag tag, bool value) {
    Tag t = renameTag(tag, "t"), f = renameTag(tag, "f");
    return newLambda(tag, newName(f), newLambda(tag, newName(t),
        value ? newReference(t, 1) : newReference(f, 2)));
}

static inline Node* newBlank(Tag tag) {return newName(renameTag(tag, "_"));}
static inline Node* newComma(Tag tag) {return newOperator(renameTag(tag, ","));}
static inline Node* newNil(Tag tag) {return newName(renameTag(tag, "[]"));}

static inline Node* newBlankReference(Tag tag, unsigned long long debruijn) {
    return newReference(renameTag(tag, "_"), debruijn);
}

static inline Node* newCommaList(Tag tag, Node* left, Node* right) {
    return newBranch(tag, COMMALIST, left, right);
}

static inline Node* prepend(Tag tag, Node* item, Node* list) {
    Node* operator = newName(renameTag(tag, "::"));
    return newApplication(tag, newApplication(tag, operator, item), list);
}

static inline Node* newYCombinator(Tag tag) {
    Node* x = newBlankReference(tag, 1);
    Node* y = newBlankReference(tag, 2);
    Node* yxx = newApplication(tag, y, newApplication(tag, x, x));
    Node* xyxx = newLambda(tag, newBlank(tag), yxx);
    return newLambda(tag, newBlank(tag), newApplication(tag, xyxx, xyxx));
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

// =======================================
// Functions to convert between node types
// =======================================

static inline void convertSymbol(Node* node, char type, long long value) {
    assert(isName(node));
    setType(node, type);
    setValue(node, value);
}
