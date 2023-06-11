typedef enum {OPERATOR=0, REFERENCE, ARROW, JUXTAPOSITION, NUMBER, LET,
    DEFINITION, ASPATTERN, COMMAPAIR, COLONPAIR, SETBUILDER} ASTType;
typedef enum {SINGLE, EXPLICITCASE, DEFAULTCASE} ArrowVariety;
typedef enum {PLAINDEFINITION, MAYBEDEFINITION, TRYDEFINITION,
    SYNTAXDEFINITION, ADTDEFINITION, BINDDEFINITION, TRYBINDDEFINITION}
    DefinitionVariety;

static inline ASTType getASTType(Node* n) {return (ASTType)getType(n);}

static inline bool isSameLexeme(Node* a, Node* b) {
    return isSameString(getLexeme(a), getLexeme(b));
}

static inline bool isReference(Node* n) {return getASTType(n) == REFERENCE;}
static inline bool isName(Node* n) {return isReference(n) && getValue(n) == 0;}
static inline bool isArrow(Node* n) {return getASTType(n) == ARROW;}
static inline bool isJuxtaposition(Node* n) {
    return getASTType(n) == JUXTAPOSITION;
}
static inline bool isNumber(Node* n) {return getASTType(n) == NUMBER;}
static inline bool isLet(Node* n) {return getASTType(n) == LET;}
static inline bool isDefinition(Node* n) {return getASTType(n) == DEFINITION;}
static inline bool isAsPattern(Node* n) {return getASTType(n) == ASPATTERN;}
static inline bool isCommaPair(Node* n) {return getASTType(n) == COMMAPAIR;}
static inline bool isColonPair(Node* n) {return getASTType(n) == COLONPAIR;}
static inline bool isSetBuilder(Node* n) {return getASTType(n) == SETBUILDER;}

static inline bool isDefaultCase(Node* n) {
    return getASTType(n) == ARROW && getVariety(n) == DEFAULTCASE;
}

static inline bool isCase(Node* n) {
    return getASTType(n) == ARROW &&
        (getVariety(n) == DEFAULTCASE || getVariety(n) == EXPLICITCASE);
}

static inline bool isUnused(Node* n) {return getLexeme(n).start[0] == '_';}

static inline bool isSyntaxDefinition(Node* n) {
    return isDefinition(n) && getVariety(n) == SYNTAXDEFINITION;
}

static inline bool isThisName(Node* node, const char* lexeme) {
    return isName(node) && isThisString(getLexeme(node), lexeme);
}

static inline bool isKeyphrase(Node* n, const char* key) {
    return isJuxtaposition(n) && isThisName(getLeft(n), key);
}

static inline bool isUnderscore(Node* n) {return isThisName(n, "_");}

static inline Node* Reference(Tag tag, long long value) {
    return newLeaf(tag, REFERENCE, 0, (void*)value);
}

static inline Node* Name(Tag tag) {return Reference(tag, 0);}

static inline Node* FixedName(Tag tag, const char* s) {
    return Name(renameTag(tag, s, 0));
}

static inline Node* ForbiddenName(Tag tag) {
    return newLeaf(tag, REFERENCE, 1, (void*)0);
}

static inline bool isForbidden(Node* name) {return getVariety(name) != 0;}

static inline Node* DefaultCaseArrow(Node* parameter, Node* body) {
    assert(isName(parameter));
    return newBranch(getTag(parameter), ARROW, DEFAULTCASE, parameter, body);
}

static inline Node* ExplicitCaseArrow(Tag tag, Node* parameter, Node* body) {
    // tag is the constructor name
    assert(isThisName(parameter, "this"));
    return newBranch(tag, ARROW, EXPLICITCASE, parameter, body);
}

static inline Node* SingleArrow(Node* parameter, Node* body) {
    assert(isName(parameter));
    return newBranch(getTag(parameter), ARROW, SINGLE, parameter, body);
}

static inline Node* Juxtaposition(Tag tag, Node* left, Node* right) {
    return newBranch(tag, JUXTAPOSITION, 0, left, right);
}

static inline Node* AsPattern(Tag tag, Node* left, Node* right) {
    return newBranch(tag, ASPATTERN, 0, left, right);
}

static inline Node* CommaPair(Tag tag, Node* left, Node* right) {
    return newBranch(tag, COMMAPAIR, 0, left, right);
}

static inline Node* ColonPair(Tag tag, Node* left, Node* right) {
    return newBranch(tag, COLONPAIR, 0, left, right);
}

static inline Node* Let(Tag tag, Node* left, Node* right) {
    return newBranch(tag, LET, 0, left, right);
}

static inline Node* Number(Tag tag, long long n) {
    return newLeaf(tag, NUMBER, 0, (void*)n);
}

static inline Node* Definition(Tag tag, DefinitionVariety variety,
        Node* left, Node* right) {
    return newBranch(tag, DEFINITION, (char)variety, left, right);
}

static inline Node* SetBuilder(Tag tag, Node* commaList) {
    return newBranch(tag, SETBUILDER, 0, VOID, commaList);
}

static inline Node* Underscore(Tag tag, unsigned long long debruijn) {
    return Reference(renameTag(tag, "_", 0), (long long)debruijn);
}

static inline Node* UnderscoreArrow(Tag tag, Node* body) {
    return SingleArrow(Underscore(tag, 0), body);
}

static inline bool isTuple(Node* node) {
    // a tuple is a spine of applications headed by a name starting with comma
    return isJuxtaposition(node) ? isTuple(getLeft(node)) :
        (isName(node) && getLexeme(node).start[0] == ',');
}
