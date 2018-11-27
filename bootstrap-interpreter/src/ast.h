typedef enum {VARIABLE, ABSTRACTION, APPLICATION, NUMERAL, CASE, LET, OPERATION,
    OPERATOR, DEFINITION, SECTION, ASPATTERN, COMMAPAIR, CURLYBRACKET} ASTType;
typedef enum {LEFTSECTION, RIGHTSECTION, LEFTRIGHTSECTION} SectionVariety;
typedef enum {PLAINDEFINITION, TRYDEFINITION, SYNTAXDEFINITION, ADTDEFINITION}
    DefinitionVariety;

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
    return (unsigned long long)getValue(n);
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

static inline bool isVariable(Node* n) {return getASTType(n) == VARIABLE;}
static inline bool isAbstraction(Node* n) {return getASTType(n) == ABSTRACTION;}
static inline bool isApplication(Node* n) {return getASTType(n) == APPLICATION;}
static inline bool isNumeral(Node* n) {return getASTType(n) == NUMERAL;}
static inline bool isOperation(Node* n) {return getASTType(n) == OPERATION;}
static inline bool isCase(Node* n) {return getASTType(n) == CASE;}
static inline bool isLet(Node* n) {return getASTType(n) == LET;}
static inline bool isOperator(Node* n) {return getASTType(n) == OPERATOR;}
static inline bool isDefinition(Node* n) {return getASTType(n) == DEFINITION;}
static inline bool isSection(Node* n) {return getASTType(n) == SECTION;}
static inline bool isAsPattern(Node* n) {return getASTType(n) == ASPATTERN;}
static inline bool isCommaPair(Node* n) {return getASTType(n) == COMMAPAIR;}
static inline bool isADT(Node* n) {return getASTType(n) == CURLYBRACKET;}

static inline bool isGlobal(Node* n) {return isVariable(n) && getValue(n) < 0;}
static inline bool isName(Node* n) {return isVariable(n) && getValue(n) == 0;}
static inline bool isUnused(Node* n) {return getLexeme(n).start[0] == '_';}

static inline bool isSyntaxDefinition(Node* n) {
    return isDefinition(n) && getVariety(n) == SYNTAXDEFINITION;
}

static inline bool isThisName(Node* node, const char* lexeme) {
    return isName(node) && isThisString(getLexeme(node), lexeme);
}

static inline bool isThisOperator(Node* node, const char* lexeme) {
    return isOperator(node) && isThisString(getLexeme(node), lexeme);
}

static inline bool isKeyphrase(Node* n, const char* key) {
    return isApplication(n) && isThisName(getLeft(n), key);
}

static inline bool isEOF(Node* n) {return isThisOperator(n, "\0");}
static inline bool isUnderscore(Node* n) {return isThisName(n, "_");}

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
    return newBranch(tag, ABSTRACTION, 0, parameter, body);
}

static inline Node* Application(Tag tag, Node* left, Node* right) {
    return newBranch(tag, APPLICATION, 0, left, right);
}

static inline Node* Case(Tag tag, Node* left, Node* right) {
    return newBranch(tag, CASE, 0, left, right);
}

static inline Node* AsPattern(Tag tag, Node* left, Node* right) {
    return newBranch(tag, ASPATTERN, 0, left, right);
}

static inline Node* CommaPair(Tag tag, Node* left, Node* right) {
    return newBranch(tag, COMMAPAIR, 0, left, right);
}

static inline Node* Let(Tag tag, Node* left, Node* right) {
    return newBranch(tag, LET, 0, left, right);
}

static inline Node* Numeral(Tag tag, long long n) {
    return newLeaf(tag, NUMERAL, n, NULL);
}

static inline Node* Operation(Tag tag, long long n) {
    return newLeaf(tag, OPERATION, n, NULL);
}

static inline Node* Definition(Tag tag, DefinitionVariety variety,
        Node* left, Node* right) {
    return newBranch(tag, DEFINITION, variety, left, right);
}

static inline Node* ADT(Tag tag, Node* commaList) {
    return newBranch(tag, CURLYBRACKET, 0, VOID, commaList);
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

static inline Node* Section(Tag tag, SectionVariety variety, Node* body) {
    return newBranch(tag, SECTION, variety, VOID, body);
}

static inline Node* LeftPlaceholder(Tag tag) {
    return Section(tag, RIGHTSECTION, Name(renameTag(tag, ".*")));
}

static inline Node* RightPlaceholder(Tag tag) {
    return Section(tag, LEFTSECTION, Name(renameTag(tag, "*.")));
}

static inline bool isLeftPlaceholder(Node* node) {
    return isSection(node) && isThisName(getRight(node), ".*");
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
    switch ((SectionVariety)getVariety(section)) {
        case LEFTSECTION:
            return wrapLeftSection(tag, body);
        case RIGHTSECTION:
            if (isName(getLeft(body)))
                return getLeft(body);   // parenthesized postfix operator
            return wrapRightSection(tag, body);
        case LEFTRIGHTSECTION:
            return wrapLeftSection(tag, wrapRightSection(tag, body));
    }
    assert(false);
    return NULL;
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
