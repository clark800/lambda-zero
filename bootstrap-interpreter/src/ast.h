// a symbol identifies a thing e.g. the parameter *named* x
// constructed from (x -> y) is not a symbol
// but the parameter name x is a symbol
// a parameter is not considered an atom because it is semantically
// a part of the lambda, not an object in it's own right

// node = branch | leaf
// branch = application | lambda
// during parsing:
// symbol = name | operator
// leaf = symbol | parameter
// after parsing symbols are converted to atoms:
// atom = reference | integer | builtin
// leaf = atom | parameter

extern const char OBJECTS_CODE[];
extern Node *IDENTITY, *UNIT, *TRUE, *FALSE, *YCOMBINATOR, *PRINT, *INPUT;
void initObjects(Node* internal);
bool isInternal(String lexeme);

// INTEGER must be zero and BUILTIN must be last
enum LeafType {INTEGER=0, NAME, OPERATOR, PARAMETER, BUILTIN};

// GLOBAL is not a builtin; it's a marker so that we can store global indices
// without colliding with builtin codes
enum BuiltinCode {PLUS=BUILTIN, MINUS, TIMES, DIVIDE, MODULUS,
      EQUAL, NOTEQUAL, LESSTHAN, GREATERTHAN, LESSTHANOREQUAL,
      GREATERTHANOREQUAL, ERROR, EXIT, PUT, GET, GLOBAL};

enum NodeType {
    N_INTEGER, N_BUILTIN, N_REFERENCE, N_GLOBAL, N_LAMBDA, N_APPLICATION};

// =============================================
// Functions to test if a node is a certain type
// =============================================

static inline String getLexeme(Node* node) {
    return getLabel(node);
}

static inline bool isSameToken(Node* tokenA, Node* tokenB) {
    return isSameString(getLexeme(tokenA), getLexeme(tokenB));
}

static inline bool isThisToken(Node* token, const char* lexeme) {
    return isThisString(getLexeme(token), lexeme);
}

static inline bool isInteger(Node* node) {
    return isLeaf(node) && getType(node) == INTEGER;
}

static inline bool isName(Node* node) {
    return isLeaf(node) && getType(node) == NAME;
}

static inline bool isOperator(Node* node) {
    return isLeaf(node) && getType(node) == OPERATOR;
}

static inline bool isParameter(Node* node) {
    return isLeaf(node) && getType(node) == PARAMETER;
}

static inline bool isBuiltin(Node* node) {
    return isLeaf(node) && getType(node) >= BUILTIN && getType(node) < GLOBAL;
}

static inline bool isReference(Node* node) {
    return isLeaf(node) && getType(node) < 0;
}

static inline bool isGlobal(Node* node) {
    return isLeaf(node) && getType(node) >= GLOBAL;
}

static inline bool isSymbol(Node* node) {
    return isName(node) || isOperator(node);
}

static inline bool isApplication(Node* node) {
    return isBranch(node) && !isParameter(getLeft(node));
}

static inline bool isLambda(Node* node) {
    return isBranch(node) && isParameter(getLeft(node));
}

static inline enum NodeType getNodeType(Node* node) {
    if (isBranch(node))
        return isParameter(getLeft(node)) ? N_LAMBDA : N_APPLICATION;
    long long type = getType(node);
    return type < 0 ? N_REFERENCE :
           type == 0 ? N_INTEGER :
           type < GLOBAL ? N_BUILTIN :
           N_GLOBAL;
}

// ====================================
// Functions to get a value from a node
// ====================================

static inline unsigned long long getDebruijnIndex(Node* reference) {
    assert(isReference(reference));
    return (unsigned long long)(-getType(reference));
}

static inline unsigned long long getBuiltinCode(Node* builtin) {
    assert(isBuiltin(builtin));
    return (unsigned long long)getType(builtin);
}

static inline unsigned long long getGlobalIndex(Node* global) {
    assert(isGlobal(global));
    return (unsigned long long)(getType(global) - GLOBAL);
}

static inline long long getInteger(Node* integer) {
    assert(isInteger(integer));
    return getValue(integer);
}

static inline Node* getParameter(Node* lambda) {
    assert(isLambda(lambda));
    return getLeft(lambda);
}

static inline Node* getBody(Node* lambda) {
    assert(isLambda(lambda));
    return getRight(lambda);
}

// ================================
// Functions to construct new nodes
// ================================

static inline Node* newInteger(String lexeme, long long value) {
    Node* result = newLeaf(lexeme, INTEGER);
    setValue(result, value);
    return result;
}

static inline Node* newName(String lexeme) {
    return newLeaf(lexeme, NAME);
}

static inline Node* newParameter(String lexeme) {
    return newLeaf(lexeme, PARAMETER);
}

static inline Node* newOperator(String lexeme) {
    return newLeaf(lexeme, OPERATOR);
}

static inline Node* newBuiltin(String lexeme, unsigned long long code) {
    assert(code >= BUILTIN);
    return newLeaf(lexeme, (long long)code);
}

static inline Node* newReference(String lexeme, unsigned long long debruijn) {
    return newLeaf(lexeme, -(long long)debruijn);
}

static inline Node* newLambda(String lexeme, Node* parameter, Node* body) {
    // we could make the left child of a lambda VOID, but storing a parameter
    // lets us decouple the location of the lambda from the parameter name,
    // which is useful in cases like string literals where we need to point
    // to the string literal for error messages, but we prefer not to make the
    // parameter name be the string literal
    assert(isParameter(parameter));
    return newBranch(lexeme, parameter, body);
}

static inline Node* newApplication(String lexeme, Node* left, Node* right) {
    return newBranch(lexeme, left, right);
}

static inline Node* newTuple(String lexeme, Node* items) {
    return newLambda(lexeme, newParameter(getLexeme(items)), items);
}

// =======================================
// Functions to convert between node types
// =======================================

static inline void convertToReference(Node* node, unsigned long long debruijn) {
    assert(isSymbol(node) && debruijn > 0);
    setType(node, -(long long)debruijn);
}

static inline void convertToBuiltin(Node* node, unsigned long long code) {
    assert(isSymbol(node) && code >= BUILTIN);
    setType(node, (long long)code);
}

static inline void convertToGlobal(Node* node, unsigned long long index) {
    assert(isSymbol(node));
    setType(node, (long long)(GLOBAL + index));
}

static inline bool isNewline(Node* node) {
    return isLeaf(node) && isThisToken(node, "\n");
}

static inline bool isDefinition(Node* node) {
    return isApplication(node) && isThisToken(node, ":=");
}

static inline bool isOpenParen(Node* node) {
    return isLeaf(node) && isThisToken(node, "(");
}

static inline bool isCloseParen(Node* node) {
    return isLeaf(node) && isThisToken(node, ")");
}

static inline bool isEOF(Node* node) {
    return isLeaf(node) && isThisToken(node, "\0");
}

static inline bool isComma(Node* node) {
    return isLeaf(node) && isThisToken(node, ",");
}

static inline bool isCommaList(Node* node) {
    return isApplication(node) && isThisToken(node, ",");
}

static inline bool isTuple(Node* node) {
    // must check the body to exclude the definition of ","
    return isLambda(node) && isComma(getParameter(node)) &&
           (isComma(getBody(node)) || isCommaList(getBody(node)));
}

static inline bool isList(Node* node) {
    return isLambda(node) && isThisToken(node, "[") &&
           isThisToken(getParameter(node), "_");
}

static inline Node* newEOF(void) {
    return newOperator(EMPTY);
}

static inline Node* newNil(String lexeme) {
    return newLambda(lexeme, getParameter(IDENTITY), TRUE);
}

static inline Node* prepend(String lexeme, Node* item, Node* list) {
    return newLambda(lexeme, getParameter(IDENTITY), newApplication(lexeme,
            newApplication(lexeme, getBody(IDENTITY), item), list));
}

static inline Node* newUnit(String lexeme) {
    return newLambda(lexeme, getParameter(UNIT), getBody(UNIT));
}

static inline Node* newSingleton(String lexeme, Node* item) {
    return newLambda(lexeme, getParameter(UNIT),
        newApplication(getLexeme(getBody(UNIT)), getBody(UNIT), item));
}
