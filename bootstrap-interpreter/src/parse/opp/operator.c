#include <string.h>
#include "array.h"
#include "tree.h"
#include "util.h"
#include "errors.h"
#include "operator.h"

static Array* SYNTAX = NULL;     // note: this never gets free'd

typedef struct Syntax Syntax;

struct Syntax {
    Lexeme lexeme, alias, prior;
    char bracketType;
    Precedence leftPrecedence, rightPrecedence;
    Fixity fixity;
    Associativity associativity;
    bool special;
    Reducer reduce;
};

static inline Node* Operator(Tag tag, long long subprecedence, void* syntax) {
    syntaxErrorIf(subprecedence >= 256, "indent too big", tag);
    return newLeaf(tag, 0, (char)subprecedence, syntax);
}

bool isOperator(Node* node) {
    return getType(node) == 0;
}

static inline Syntax* getSyntax(Node* op) {
    assert(isOperator(op));
    return (Syntax*)getData(op);
}

Fixity getFixity(Node* op) {return getSyntax(op)->fixity;}
Lexeme getPrior(Node* op) {return getSyntax(op)->prior;}
bool isSpecialOperator(Node* op) {return getSyntax(op)->special;}
static char getBracketType(Node* op) {return getSyntax(op)->bracketType;}
unsigned char getSubprecedence(Node* op) {return (unsigned char)getVariety(op);}

Node* reduce(Node* operator, Node* left, Node* right) {
    return getSyntax(operator)->reduce(getTag(operator), left, right);
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

static Syntax* findSyntax(Lexeme lexeme) {
    size_t n = length(SYNTAX);
    for (unsigned int i = 1; i <= n; ++i) {
        Syntax* syntax = elementAt(SYNTAX, n - i);
        if (isSameLexeme(lexeme, syntax->lexeme))
            return syntax;
    }
    return NULL;
}

Node* parseOperator(Lexeme lexeme, long long subprecedence) {
    Syntax* syntax = findSyntax(lexeme);
    return syntax == NULL ? NULL : Operator(newTag(newLexeme(
        syntax->alias.start, syntax->alias.length, lexeme.location),
        (char)syntax->fixity), subprecedence, syntax);
}

static void appendSyntaxCopy(Syntax* syntax, Lexeme lexeme, Lexeme alias) {
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
        Syntax* syntax = findSyntax(getLexeme(getTag(prior)));
        syntaxErrorNodeIf(syntax == NULL, "prior syntax not defined", prior);
        precedence = syntax->rightPrecedence;  // precedence comes in as 0
        if (fixity != INFIX && fixity != POSTFIX)
            syntaxError("invalid fixity for operator with prior", tag);
        if (associativity != L || syntax->associativity != L)
            syntaxError("invalid associativity for operator or prior", tag);
    }
    bool special = prior != NULL;
    Lexeme lexeme = getLexeme(tag);
    Lexeme priorLexeme = prior ? getLexeme(getTag(prior)) : EMPTY;
    appendSyntax((Syntax){lexeme, lexeme, priorLexeme, '_', precedence,
        precedence, fixity, associativity, special, reducer});
}

void popSyntax(void) { unappend(SYNTAX); }

void initSyntax(void) { SYNTAX = newArray(1024); }

void addCoreSyntax(const char* symbol, Precedence precedence,
        Fixity fixity, Associativity associativity, Reducer reducer) {
    Lexeme lexeme = newLiteralLexeme(symbol, newLocation(0, 0, 0));
    appendSyntax((Syntax){lexeme, lexeme, EMPTY, '_', precedence,
        precedence, fixity, associativity, true, reducer});
}

void addBracketSyntax(const char* symbol, char type, Precedence outerPrecedence,
        Fixity fixity, Reducer reducer) {
    size_t length = symbol[0] == 0 && fixity == CLOSEFIX ? 1 : strlen(symbol);
    Lexeme lexeme = newLexeme(symbol, (unsigned short)length, (Location){0});
    Precedence leftPrecedence = fixity == OPENFIX ? outerPrecedence : 0;
    Precedence rightPrecedence = fixity == OPENFIX ? 0 : outerPrecedence;
    appendSyntax((Syntax){lexeme, lexeme, EMPTY, type,
        leftPrecedence, rightPrecedence, fixity, R, true, reducer});
}

void addCoreAlias(const char* alias, const char* name) {
    Syntax* syntax = findSyntax(newLiteralLexeme(name, newLocation(0, 0, 0)));
    Lexeme lexeme = newLiteralLexeme(alias, newLocation(0, 0, 0));
    appendSyntaxCopy(syntax, lexeme, syntax->alias);
}

void addSyntaxCopy(Lexeme lexeme, Node* name, bool alias) {
    Syntax* syntax = findSyntax(getLexeme(getTag(name)));
    syntaxErrorNodeIf(syntax == NULL, "syntax not defined", name);
    appendSyntaxCopy(syntax, lexeme, alias ? syntax->alias : lexeme);
}
