#include <string.h>
#include "array.h"
#include "tree.h"
#include "util.h"
#include "errors.h"
#include "operator.h"

static Array* SYNTAX = NULL;     // note: this never gets free'd

typedef struct Syntax Syntax;

struct Syntax {
    String lexeme, alias, prior;
    char bracketType;
    Precedence leftPrecedence, rightPrecedence;
    Fixity fixity;
    Associativity associativity;
    bool special;
    Reducer reduce;
    Syntax* infixSyntax;        // for binary prefix operators
};

static inline Node* Operator(Tag tag, long long subprecedence, void* syntax) {
    syntaxErrorIf(subprecedence >= 256, "indent too big", tag);
    return newLeaf(tag, 0, (char)subprecedence, syntax);
}

bool isOperator(Node* node) {
    return isLeaf(node) && getType(node) == 0;
}

static inline Syntax* getSyntax(Node* op) {
    assert(isOperator(op));
    return (Syntax*)getData(op);
}

Fixity getFixity(Node* op) {return getSyntax(op)->fixity;}
bool isSpecialOperator(Node* op) {return getSyntax(op)->special;}
static char getBracketType(Node* op) {return getSyntax(op)->bracketType;}
unsigned char getSubprecedence(Node* op) {return (unsigned char)getVariety(op);}

static Node* getPriorNode(Node* operator, Node* left, Node* right) {
    Syntax* syntax = getSyntax(operator);
    switch (syntax->fixity) {
        case INFIX:
            switch (syntax->associativity) {
                case L: return left;
                case R: return right;
                default: return NULL;
            }
        case PREFIX: return right;
        case POSTFIX: return left;
        default: return NULL;
    }
}

Node* reduce(Node* operator, Node* left, Node* right) {
    Syntax* syntax = getSyntax(operator);
    if (syntax->prior.length > 0) {
        Node* node = getPriorNode(operator, left, right);
        if (node == NULL)
            syntaxErrorNode("invalid operator with prior", operator);
        if (isLeaf(node) || !isSameString(getLexeme(node), syntax->prior))
            syntaxErrorNode("invalid prior for", operator);
    }
    return syntax->reduce(getTag(operator), left, right);
}

Node* reduceBracket(Node* open, Node* close, Node* before, Node* contents) {
    if (getBracketType(open) != getBracketType(close)) {
        if (getBracketType(close) == '\0')
            syntaxErrorNode("missing close for", open);
        else syntaxErrorNode("missing open for", close);
    }

    return reduce(close, before, reduce(open, before, contents));
}

bool isHigherPrecedence(Node* left, Node* right) {
    assert(isOperator(left) && isOperator(right));

    Syntax* leftSyntax = getSyntax(left);
    Syntax* rightSyntax = getSyntax(right);

    if (leftSyntax->rightPrecedence == rightSyntax->leftPrecedence) {
        if (leftSyntax->associativity != rightSyntax->associativity)
            syntaxErrorNode("incompatible associativity", right);
        if (rightSyntax->associativity == N)
            syntaxErrorNode("operator is non-associative", right);

        if (leftSyntax->associativity == R)
            return getSubprecedence(left) > getSubprecedence(right);
        return getSubprecedence(left) >= getSubprecedence(right);
    }

    return leftSyntax->rightPrecedence > rightSyntax->leftPrecedence;
}

static Syntax* findSyntax(String lexeme) {
    size_t n = length(SYNTAX);
    for (unsigned int i = 1; i <= n; ++i) {
        Syntax* syntax = elementAt(SYNTAX, n - i);
        if (isSameString(lexeme, syntax->lexeme))
            return syntax;
    }
    return NULL;
}

Node* parseOperator(Tag tag, long long subprecedence) {
    Syntax* syntax = findSyntax(tag.lexeme);
    return syntax == NULL ? NULL : Operator(setTagFixity(newTag(syntax->alias,
        tag.location), (char)syntax->fixity), subprecedence, syntax);
}

static void appendSyntaxCopy(Syntax* syntax, String lexeme, String alias) {
    Syntax* newSyntax = (Syntax*)smalloc(sizeof(Syntax));
    *newSyntax = *syntax;
    newSyntax->lexeme = lexeme;
    newSyntax->alias = alias;
    append(SYNTAX, newSyntax);
}

static void appendSyntax(Syntax syntax) {
    Syntax* newSyntax = (Syntax*)smalloc(sizeof(Syntax));
    *newSyntax = syntax;
    append(SYNTAX, newSyntax);
}

void addSyntax(Tag tag, Node* prior, Precedence precedence, Fixity fixity,
        Associativity associativity, Reducer reducer) {
    if (prior != NULL) {
        // if special, override precedence with precedence of prior
        Syntax* syntax = findSyntax(getLexeme(prior));
        syntaxErrorNodeIf(syntax == NULL, "syntax not defined", prior);
        if (syntax->associativity != associativity || associativity == N)
            syntaxErrorNode("invalid associativity for prior", prior);
        precedence = syntax->associativity == L ?
            syntax->rightPrecedence : syntax->leftPrecedence;
    }
    bool special = prior != NULL;
    String priorLexeme = prior ? getLexeme(prior) : EMPTY;
    appendSyntax((Syntax){tag.lexeme, tag.lexeme, priorLexeme, '_', precedence,
        precedence, fixity, associativity, special, reducer, NULL});
}

void popSyntax(void) { unappend(SYNTAX); }

void initSyntax(void) { SYNTAX = newArray(1024); }

void addCoreSyntax(const char* symbol, Precedence precedence,
        Fixity fixity, Associativity associativity, Reducer reducer) {
    String lexeme = toString(symbol);
    appendSyntax((Syntax){lexeme, lexeme, EMPTY, '_', precedence,
        precedence, fixity, associativity, true, reducer, NULL});
}

void addBracketSyntax(const char* symbol, char type, Precedence outerPrecedence,
        Fixity fixity, Reducer reducer) {
    size_t length = symbol[0] == 0 && fixity == CLOSEFIX ? 1 : strlen(symbol);
    String lexeme = newString(symbol, (unsigned char)length);
    Precedence leftPrecedence = fixity == OPENFIX ? outerPrecedence : 0;
    Precedence rightPrecedence = fixity == OPENFIX ? 0 : outerPrecedence;
    appendSyntax((Syntax){lexeme, lexeme, EMPTY, type,
        leftPrecedence, rightPrecedence, fixity, R, true, reducer, NULL});
}

static Node* reduceBinaryPrefix(Tag tag, Node* left, Node* right) {
    (void)left, (void)right;
    syntaxError("Internal error: reduce binary prefix", tag);
    return NULL;
}

void addBinaryPrefixSyntax(const char* symbol, Precedence precedence,
        Reducer reducer) {
    String lexeme = toString(symbol);
    appendSyntax((Syntax){lexeme, lexeme, EMPTY, '_', 99,
        precedence, INFIX, R, true, reducer, NULL});
    Syntax* infixSyntax = elementAt(SYNTAX, length(SYNTAX) - 1);
    appendSyntax((Syntax){lexeme, lexeme, EMPTY, '_', 99,
        99, PREFIX, L, true, reduceBinaryPrefix, infixSyntax});
}

Node* getBinaryPrefixInfixOperator(Node* operator) {
    Syntax* infixSyntax = getSyntax(operator)->infixSyntax;
    if (infixSyntax == NULL)
        return NULL;
    if (getFixity(operator) != PREFIX)
        syntaxErrorNode("infix syntax on non-prefix operator", operator);
    return Operator(getTag(operator), getSubprecedence(operator), infixSyntax);
}

void addCoreAlias(const char* alias, const char* name) {
    Syntax* syntax = findSyntax(toString(name));
    appendSyntaxCopy(syntax, toString(alias), syntax->alias);
}

void addSyntaxCopy(String lexeme, Node* name, bool alias) {
    Syntax* syntax = findSyntax(getLexeme(name));
    syntaxErrorNodeIf(syntax == NULL, "syntax not defined", name);
    appendSyntaxCopy(syntax, lexeme, alias ? syntax->alias : lexeme);
}
