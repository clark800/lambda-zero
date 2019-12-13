typedef enum {VARIABLE, ABSTRACTION, APPLICATION, NUMERAL, OPERATION} TermType;

// names in Operations must line up with codes in OperationCode
static const char* const Operations[] = {"+", "--", "*", "//", "%",
    "=", "=/=", "<", ">", "<=", ">=", "abort",
    "(increment)", "(exit)", "(put)", "(get)"};
typedef enum {PLUS, MONUS, TIMES, DIVIDE, MODULO, EQUAL, NOTEQUAL,
      LESSTHAN, GREATERTHAN, LESSTHANOREQUAL, GREATERTHANOREQUAL,
      ABORT, INCREMENT, EXIT, PUT, GET} OperationCode;

typedef Node Term;
static inline TermType getTermType(Term* t) {return (TermType)getType(t);}
static inline bool isVariable(Term* t) {return getType(t) == VARIABLE;}
static inline bool isAbstraction(Term* t) {return getType(t) == ABSTRACTION;}
static inline bool isApplication(Term* t) {return getType(t) == APPLICATION;}
static inline bool isNumeral(Term* t) {return getType(t) == NUMERAL;}
static inline bool isOperation(Term* t) {return getType(t) == OPERATION;}
static inline bool isGlobal(Term* t) {return isVariable(t) && getValue(t) < 0;}

static inline Term* Variable(Tag tag, long long value) {
    return newLeaf(tag, VARIABLE, value, NULL);
}

static inline Term* Abstraction(Tag tag, Term* body) {
    return newBranch(tag, ABSTRACTION, 0, VOID, body);
}

static inline Term* Application(Tag tag, Term* left, Term* right) {
    return newBranch(tag, APPLICATION, 0, left, right);
}

static inline Term* Numeral(Tag tag, long long n) {
    return newLeaf(tag, NUMERAL, n, NULL);
}

static inline Term* Operation(Tag tag, long long n) {
    return newLeaf(tag, OPERATION, n, NULL);
}

static inline unsigned long long getDebruijnIndex(Term* t) {
    assert(getValue(t) > 0);
    return (unsigned long long)getValue(t);
}

static inline unsigned long long getGlobalIndex(Term* t) {
    assert(getValue(t) < 0);
    return (unsigned long long)(-getValue(t) - 1);
}


static inline Term* getParameter(Term* t) {return getLeft(t);}
static inline Term* getBody(Term* t) {return getRight(t);}
static inline OperationCode getOperationCode(Term* t) {
  return (OperationCode)getValue(t);
}
