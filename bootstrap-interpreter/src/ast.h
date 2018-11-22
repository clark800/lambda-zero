typedef enum
    {SYMBOL, LAMBDA, APPLICATION, DEFINITION, NATURAL, BUILTIN, SECTION}
    NodeType;
typedef enum {LEFTSECTION, RIGHTSECTION, LEFTRIGHTSECTION} SectionSide;
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
static inline bool isSection(Node* node) {return getNodeType(node) == SECTION;}

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

static inline bool isAsPattern(Node* node) {
    return isApplication(node) && isThisLexeme(node, "@");
}

static inline bool isSyntaxMarker(Node* node) {
    return isLambda(node) && isThisLexeme(node, "syntax");
}

// ================================
// Functions to construct new nodes
// ================================


static inline Node* newSymbol(Tag tag, long long value, void* rules) {
    return newLeaf(tag, SYMBOL, value, rules);
}

static inline Node* newName(Tag tag) {return newSymbol(tag, 0, NULL);}

static inline Node* newRename(Tag tag, const char* s) {
    return newName(renameTag(tag, s));
}

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
    return isSymbol(node) && getValue(node) == 0;
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

static inline Node* newNil(Tag tag) {return newRename(tag, "[]");}

static inline Node* prepend(Tag tag, Node* item, Node* list) {
    Node* operator = newRename(tag, "::");
    return newApplication(tag, newApplication(tag, operator, item), list);
}

// ====================================
// Sections
// ====================================

static inline Node* newSection(Tag tag, SectionSide side, Node* body) {
    return newBranch(tag, SECTION, newNatural(tag, (long long)side), body);
}

static inline Node* newLeftPlaceholder(Tag tag) {
    return newSection(tag, RIGHTSECTION, newName(renameTag(tag, ".*")));
}

static inline Node* newRightPlaceholder(Tag tag) {
    return newSection(tag, LEFTSECTION, newName(renameTag(tag, "*.")));
}

static inline bool isLeftPlaceholder(Node* node) {
    return isSection(node) && isThisLeaf(getRight(node), ".*");
}

static inline Node* getSectionBody(Node* node) {return getRight(node);}

static inline Node* wrapLeftSection(Tag tag, Node* body) {
    return newLambda(tag, newName(renameTag(tag, "*.")), body);
}

static inline Node* wrapRightSection(Tag tag, Node* body) {
    return newLambda(tag, newName(renameTag(tag, ".*")), body);
}

static inline Node* wrapSection(Tag tag, Node* section) {
    Node* body = getSectionBody(section);
    switch ((SectionSide)getValue(getLeft(section))) {
        case LEFTSECTION:
            return wrapLeftSection(tag, body);
        case RIGHTSECTION:
            if (isSymbol(getLeft(body)))
                return getLeft(body);   // parenthesized postfix operator
            return wrapRightSection(tag, body);
        case LEFTRIGHTSECTION:
            return wrapLeftSection(tag, wrapRightSection(tag, body));
        default: return NULL;
    }
}

// ====================================
// Builtins
// ====================================

enum BuiltinCode {PLUS, MINUS, TIMES, DIVIDE, MODULO,
      EQUAL, NOTEQUAL, LESSTHAN, GREATERTHAN, LESSTHANOREQUAL,
      GREATERTHANOREQUAL, INCREMENT, ERROR, UNDEFINED, EXIT, PUT, GET};

static inline Node* newPrinter(Tag tag) {
    Node* put = newBuiltin(renameTag(tag, "put"), PUT);
    Node* fold = newRename(tag, "fold");
    Node* unit = newRename(tag, "()");
    Node* under = newUnderscore(tag, 1);
    return newLambda(tag, newUnderscore(tag, 0), newApplication(tag,
        newApplication(tag, newApplication(tag, fold, under), put), unit));
}
