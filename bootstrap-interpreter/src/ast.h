typedef enum {VARIABLE, ABSTRACTION, APPLICATION, NUMERAL, OPERATION,
    OPERATOR, DEFINITION, SECTION} ASTType;
typedef enum {LEFTSECTION, RIGHTSECTION, LEFTRIGHTSECTION} SectionSide;

// ====================================
// Functions to get a value from a node
// ====================================

static inline ASTType getASTType(Node* n) {return (ASTType)getType(n);}
static inline String getLexeme(Node* n) {return getTag(n).lexeme;}
static inline Node* getParameter(Node* n) {return getLeft(n);}
static inline Node* getBody(Node* n) {return getRight(n);}
static inline long long getOperationCode(Node* n) {return getValue(n);}

static inline unsigned long long getDebruijnIndex(Node* n) {
    assert(getValue(n) > 0);
    return (unsigned long long)(getValue(n) - 1);
}

static inline unsigned long long getGlobalIndex(Node* n) {
    assert(getValue(n) < 0);
    return (unsigned long long)(-getValue(n) - 1);
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

static inline bool isVariable(Node* n) {return getASTType(n) == VARIABLE;}
static inline bool isAbstraction(Node* n) {return getASTType(n) == ABSTRACTION;}
static inline bool isApplication(Node* n) {return getASTType(n) == APPLICATION;}
static inline bool isNumeral(Node* n) {return getASTType(n) == NUMERAL;}
static inline bool isOperation(Node* n) {return getASTType(n) == OPERATION;}
static inline bool isOperator(Node* n) {return getASTType(n) == OPERATOR;}
static inline bool isDefinition(Node* n) {return getASTType(n) == DEFINITION;}
static inline bool isSection(Node* n) {return getASTType(n) == SECTION;}
static inline bool isGlobal(Node* n) {return isVariable(n) && getValue(n) < 0;}
static inline bool isEOF(Node* n) {return isThisLeaf(n, "\0");}
static inline bool isUnderscore(Node* n) {return isThisLeaf(n, "_");}
static inline bool isName(Node* n) {return isVariable(n) && getValue(n) == 0;}
static inline bool isUnused(Node* n) {return getLexeme(n).start[0] == '_';}

static inline bool isAsPattern(Node* n) {
    return isApplication(n) && isThisLexeme(n, "@");
}

static inline bool isSyntaxMarker(Node* n) {
    return isAbstraction(n) && isThisLexeme(n, "syntax");
}

// ================================
// Functions to construct nodes
// ================================


static inline Node* Variable(Tag tag, long long value) {
    return newLeaf(tag, VARIABLE, value, NULL);
}

static inline Node* Operator(Tag tag, long long value, void* rules) {
    return newLeaf(tag, OPERATOR, value, rules);
}

static inline Node* Name(Tag tag) {return Variable(tag, 0);}

static inline Node* Abstraction(Tag tag, Node* parameter, Node* body) {
    // we could make the left child of a lambda VOID, but storing a parameter
    // lets us decouple the location of the lambda from the parameter name,
    // which is useful in cases like string literals where we need to point
    // to the string literal for error messages, but we prefer not to make the
    // parameter name be the string literal
    assert(isName(parameter));
    return newBranch(tag, ABSTRACTION, parameter, body);
}

static inline Node* Application(Tag tag, Node* left, Node* right) {
    return newBranch(tag, APPLICATION, left, right);
}

static inline Node* Numeral(Tag tag, long long n) {
    return newLeaf(tag, NUMERAL, n, NULL);
}

static inline Node* Operation(Tag tag, long long n) {
    return newLeaf(tag, OPERATION, n, NULL);
}

static inline Node* Definition(Tag tag, Node* left, Node* right) {
    return newBranch(tag, DEFINITION, left, right);
}

static inline Node* FixedName(Tag tag, const char* s) {
    return Name(renameTag(tag, s));
}

static inline Node* Underscore(Tag tag, unsigned long long debruijn) {
    return Variable(renameTag(tag, "_"), (long long)debruijn);
}

static inline Node* Nil(Tag tag) {return FixedName(tag, "[]");}

static inline Node* prepend(Tag tag, Node* item, Node* list) {
    return Application(tag, Application(tag, FixedName(tag, "::"), item), list);
}

// ====================================
// Sections
// ====================================

static inline Node* Section(Tag tag, SectionSide side, Node* body) {
    return newBranch(tag, SECTION, Numeral(tag, (long long)side), body);
}

static inline Node* LeftPlaceholder(Tag tag) {
    return Section(tag, RIGHTSECTION, Name(renameTag(tag, ".*")));
}

static inline Node* RightPlaceholder(Tag tag) {
    return Section(tag, LEFTSECTION, Name(renameTag(tag, "*.")));
}

static inline bool isLeftPlaceholder(Node* node) {
    return isSection(node) && isThisLeaf(getRight(node), ".*");
}

static inline Node* getSectionBody(Node* node) {return getRight(node);}

static inline Node* wrapLeftSection(Tag tag, Node* body) {
    return Abstraction(tag, Name(renameTag(tag, "*.")), body);
}

static inline Node* wrapRightSection(Tag tag, Node* body) {
    return Abstraction(tag, Name(renameTag(tag, ".*")), body);
}

static inline Node* wrapSection(Tag tag, Node* section) {
    Node* body = getSectionBody(section);
    switch ((SectionSide)getValue(getLeft(section))) {
        case LEFTSECTION:
            return wrapLeftSection(tag, body);
        case RIGHTSECTION:
            if (isName(getLeft(body)))
                return getLeft(body);   // parenthesized postfix operator
            return wrapRightSection(tag, body);
        case LEFTRIGHTSECTION:
            return wrapLeftSection(tag, wrapRightSection(tag, body));
        default: return NULL;
    }
}

// ====================================
// Operations
// ====================================

enum OperationCode {PLUS, MINUS, TIMES, DIVIDE, MODULO,
      EQUAL, NOTEQUAL, LESSTHAN, GREATERTHAN, LESSTHANOREQUAL,
      GREATERTHANOREQUAL, INCREMENT, ERROR, UNDEFINED, EXIT, PUT, GET};

static inline Node* Printer(Tag tag) {
    Node* put = Operation(renameTag(tag, "put"), PUT);
    Node* fold = FixedName(tag, "fold");
    Node* unit = FixedName(tag, "()");
    Node* under = Underscore(tag, 1);
    return Abstraction(tag, Underscore(tag, 0), Application(tag,
        Application(tag, Application(tag, fold, under), put), unit));
}
