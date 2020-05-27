#include <string.h>
#include "array.h"
#include "tree.h"
#include "util.h"
#include "errors.h"
#include "operator.h"

static Array* RULES = NULL;     // note: this never gets free'd

typedef struct {
    String lexeme, alias, prior;
    char bracketType;
    Precedence leftPrecedence, rightPrecedence;
    Fixity fixity;
    Associativity associativity;
    bool special;
    Node* (*reduce)(Node* operator, Node* left, Node* right);
} Rules;

static inline Node* Operator(Tag tag, long long subprecedence, void* rules) {
    return newLeaf(tag, 0, subprecedence, rules);
}

bool isOperator(Node* node) {
    return isLeaf(node) && getType(node) == 0;
}

static inline Rules* getRules(Node* op) {
    assert(isOperator(op));
    return (Rules*)getData(op);
}

Fixity getFixity(Node* op) {return getRules(op)->fixity;}
bool isSpecialOperator(Node* op) {return getRules(op)->special;}
char getBracketType(Node* op) {return getRules(op)->bracketType;}

static Node* getPriorNode(Node* operator, Node* left, Node* right) {
    Rules* rules = getRules(operator);
    switch(rules->fixity) {
        case INFIX:
            switch(rules->associativity) {
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
    Rules* rules = getRules(operator);
    if (rules->prior.length > 0) {
        Node* node = getPriorNode(operator, left, right);
        syntaxErrorIf(node == NULL, "invalid operator with prior", operator);
        if (isLeaf(node) || !isSameString(getLexeme(node), rules->prior))
            syntaxError("invalid prior for", operator);
    }
    return rules->reduce(operator, left, right);
}

Node* reduceBracket(Node* open, Node* close, Node* before, Node* contents) {
    if (getBracketType(open) != getBracketType(close)) {
        if (getBracketType(close) == '\0')
            syntaxError("missing close for", open);
        else syntaxError("missing open for", close);
    }

    return reduce(close, before, reduce(open, before, contents));
}

bool isHigherPrecedence(Node* left, Node* right) {
    assert(isOperator(left) && isOperator(right));

    Rules* leftRules = getRules(left);
    Rules* rightRules = getRules(right);

    if (leftRules->rightPrecedence == rightRules->leftPrecedence) {
        if (leftRules->associativity != rightRules->associativity)
            syntaxError("incompatible associativity", right);
        if (rightRules->associativity == N)
            syntaxError("operator is non-associative", right);

        if (leftRules->associativity == R)
            return getValue(left) > getValue(right);
        return getValue(left) >= getValue(right);
    }

    return leftRules->rightPrecedence > rightRules->leftPrecedence;
}

static Rules* findRules(String lexeme) {
    size_t n = length(RULES);
    for (unsigned int i = 1; i <= n; ++i) {
        Rules* rules = elementAt(RULES, n - i);
        if (isSameString(lexeme, rules->lexeme))
            return rules;
    }
    return NULL;
}

Node* parseOperator(Tag tag, long long subprecedence) {
    Rules* rules = findRules(tag.lexeme);
    return rules == NULL ? NULL : Operator(setTagFixity(newTag(rules->alias,
        tag.location), (char)rules->fixity), subprecedence, rules);
}

static void appendSyntaxCopy(Rules* rules, String lexeme, String alias) {
    Rules* newRules = (Rules*)smalloc(sizeof(Rules));
    *newRules = *rules;
    newRules->lexeme = lexeme;
    newRules->alias = alias;
    append(RULES, newRules);
}

static void appendSyntax(Rules rules) {
    Rules* newRules = (Rules*)smalloc(sizeof(Rules));
    *newRules = rules;
    append(RULES, newRules);
}

void addSyntax(Tag tag, Node* prior, Precedence precedence, Fixity fixity,
        Associativity associativity, Reducer reducer) {
    if (prior != NULL) {
        // if special, override precedence with precedence of prior
        Rules* rules = findRules(getLexeme(prior));
        syntaxErrorIf(rules == NULL, "syntax not defined", prior);
        if (rules->associativity != associativity || associativity == N)
            syntaxError("invalid associativity for prior", prior);
        precedence = rules->associativity == L ?
            rules->rightPrecedence : rules->leftPrecedence;
    }
    bool special = prior != NULL;
    String priorLexeme = prior ? getLexeme(prior) : newString("", 0);
    appendSyntax((Rules){tag.lexeme, tag.lexeme, priorLexeme, '_', precedence,
        precedence, fixity, associativity, special, reducer});
}

void popSyntax(void) { unappend(RULES); }

void initSyntax(void) { RULES = newArray(1024); }

void addCoreSyntax(const char* symbol, Precedence precedence,
        Fixity fixity, Associativity associativity, Reducer reducer) {
    String lexeme = newString(symbol, (unsigned int)strlen(symbol));
    appendSyntax((Rules){lexeme, lexeme, newString("", 0), '_', precedence,
        precedence, fixity, associativity, true, reducer});
}

void addBracketSyntax(const char* symbol, char type, Precedence outerPrecedence,
    Fixity fixity, Reducer reducer) {
    size_t length = symbol[0] == 0 && fixity == CLOSEFIX ? 1 : strlen(symbol);
    String lexeme = newString(symbol, (unsigned int)length);
    Precedence leftPrecedence = fixity == OPENFIX ? outerPrecedence : 0;
    Precedence rightPrecedence = fixity == OPENFIX ? 0 : outerPrecedence;
    appendSyntax((Rules){lexeme, lexeme, newString("", 0), type,
        leftPrecedence, rightPrecedence, fixity, R, true, reducer});
}

void addCoreAlias(const char* alias, const char* name) {
    Rules* rules = findRules(toString(name));
    appendSyntaxCopy(rules, toString(alias), rules->alias);
}

void addSyntaxCopy(String lexeme, Node* name, bool alias) {
    Rules* rules = findRules(getLexeme(name));
    syntaxErrorIf(rules == NULL, "syntax not defined", name);
    appendSyntaxCopy(rules, lexeme, alias ? rules->alias : lexeme);
}
