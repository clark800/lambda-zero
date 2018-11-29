typedef enum {NAME, REFERENCE, ARROW, JUXTAPOSITION, NUMBER, CASE, LET,
    OPERATOR, DEFINITION, SECTION, ASPATTERN, COMMAPAIR, CURLYBRACKET} ASTType;
typedef enum {LEFTSECTION, RIGHTSECTION, LEFTRIGHTSECTION} SectionVariety;
typedef enum {PLAINDEFINITION, TRYDEFINITION, SYNTAXDEFINITION, ADTDEFINITION}
    DefinitionVariety;

static inline ASTType getASTType(Node* n) {return (ASTType)getType(n);}
static inline String getLexeme(Node* n) {return getTag(n).lexeme;}

static inline bool isSameLexeme(Node* a, Node* b) {
    return isSameString(getLexeme(a), getLexeme(b));
}

static inline bool isName(Node* n) {return getASTType(n) == NAME;}
static inline bool isReference(Node* n) {return getASTType(n) == REFERENCE;}
static inline bool isArrow(Node* n) {return getASTType(n) == ARROW;}
static inline bool isJuxtaposition(Node* n) {
    return getASTType(n) == JUXTAPOSITION;
}
static inline bool isNumber(Node* n) {return getASTType(n) == NUMBER;}
static inline bool isCase(Node* n) {return getASTType(n) == CASE;}
static inline bool isLet(Node* n) {return getASTType(n) == LET;}
static inline bool isOperator(Node* n) {return getASTType(n) == OPERATOR;}
static inline bool isDefinition(Node* n) {return getASTType(n) == DEFINITION;}
static inline bool isSection(Node* n) {return getASTType(n) == SECTION;}
static inline bool isAsPattern(Node* n) {return getASTType(n) == ASPATTERN;}
static inline bool isCommaPair(Node* n) {return getASTType(n) == COMMAPAIR;}
static inline bool isADT(Node* n) {return getASTType(n) == CURLYBRACKET;}

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
    return isJuxtaposition(n) && isThisName(getLeft(n), key);
}

static inline bool isEOF(Node* n) {return isThisOperator(n, "\0");}
static inline bool isUnderscore(Node* n) {return isThisName(n, "_");}

static inline Node* Reference(Tag tag, long long value) {
    return newLeaf(tag, REFERENCE, value, NULL);
}

static inline Node* Name(Tag tag) {
    return newLeaf(tag, NAME, 0, NULL);
}

static inline Node* Operator(Tag tag, long long value, void* rules) {
    return newLeaf(tag, OPERATOR, value, rules);
}

static inline Node* Arrow(Tag tag, Node* parameter, Node* body) {
    assert(isName(parameter));
    return newBranch(tag, ARROW, 0, parameter, body);
}

static inline Node* Juxtaposition(Tag tag, Node* left, Node* right) {
    return newBranch(tag, JUXTAPOSITION, 0, left, right);
}

static inline Node* Case(Tag tag, Node* body) {
    return newBranch(tag, CASE, 0, Name(renameTag(tag, "_")), body);
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

static inline Node* Number(Tag tag, long long n) {
    return newLeaf(tag, NUMBER, n, NULL);
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
    return Reference(renameTag(tag, "_"), (long long)debruijn);
}

static inline Node* UnderscoreArrow(Tag tag, Node* body) {
    return Arrow(tag, Name(renameTag(tag, "_")), body);
}

static inline Node* Nil(Tag tag) {return FixedName(tag, "[]");}

static inline Node* prepend(Tag tag, Node* item, Node* list) {
    return Juxtaposition(tag, Juxtaposition(tag, FixedName(tag, "::"),
        item), list);
}

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
