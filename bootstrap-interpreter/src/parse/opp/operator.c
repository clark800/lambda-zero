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

bool isThisOperator(Node* node, const char* lexeme) {
    return isOperator(node) && isThisString(getLexeme(node), lexeme);
}

static Rules* getRules(Node* op) {
    assert(isOperator(op));
    return (Rules*)getData(op);
}

Node* reduce(Node* operator, Node* left, Node* right) {
    return getRules(operator)->reduce(operator, left, right);
}

Fixity getFixity(Node* op) {return getRules(op)->fixity;}
char getBracketType(Node* op) {return getRules(op)->bracketType;}
Associativity getAssociativity(Node* op) {return getRules(op)->associativity;}
String getPrior(Node* op) {return getRules(op)->prior;}

bool isLeftSectionOperator(Node* op) {
    if (!isOperator(op)) return false;
    Rules* rules = getRules(op);
    Fixity fixity = rules->fixity;
    return !rules->special && (fixity == INFIX || fixity == PREFIX);
}

bool isRightSectionOperator(Node* op) {
    if (!isOperator(op)) return false;
    Rules* rules = getRules(op);
    Fixity fixity = rules->fixity;
    return !rules->special && (fixity == INFIX || fixity == POSTFIX);
}

bool isOpenOperator(Node* node) {
    return isOperator(node) && getFixity(node) == OPENFIX;
}

bool isCloseOperator(Node* node) {
    return isOperator(node) && getFixity(node) == CLOSEFIX;
}

bool isHigherPrecedence(Node* left, Node* right) {
    assert(isOperator(left) && isOperator(right));

    Rules* leftRules = getRules(left);
    Rules* rightRules = getRules(right);

    if (leftRules->rightPrecedence == rightRules->leftPrecedence) {
        if (leftRules->associativity == N)
            syntaxError("operator is non-associative", left);
        if (rightRules->associativity == N)
            syntaxError("operator is non-associative", right);
        if (leftRules->associativity != rightRules->associativity)
            syntaxError("incompatible associativity", right);

        if (leftRules->associativity == R)
            return getValue(left) > getValue(right);
        return getValue(left) >= getValue(right);
    }

    return leftRules->rightPrecedence > rightRules->leftPrecedence;
}

static Rules* findRules(String lexeme) {
    for (unsigned int i = 0; i < length(RULES); ++i) {
        Rules* rules = elementAt(RULES, i);
        if (isSameString(lexeme, rules->lexeme))
            return rules;
    }
    return NULL;
}

Precedence findPrecedence(Node* node) {
    Rules* rules = findRules(getLexeme(node));
    syntaxErrorIf(rules == NULL, "syntax not defined", node);
    if (rules->leftPrecedence != rules->rightPrecedence)
        syntaxError("operator not supported", node);
    return rules->leftPrecedence;
}

Node* parseOperator(Tag tag, long long subprecedence) {
    Rules* rules = findRules(tag.lexeme);
    return rules == NULL ? NULL :
        Operator(newTag(rules->alias, tag.location), subprecedence, rules);
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

void addSyntax(Tag tag, String prior, Precedence leftPrecedence,
        Precedence rightPrecedence, Fixity fixity, Associativity associativity,
        Node* (*reducer)(Node*, Node*, Node*)) {
    bool special = prior.length != 0;
    if (special && associativity == N)
        tokenErrorIf(true, "expected numeric precedence", tag);
    tokenErrorIf(findRules(tag.lexeme) != NULL, "syntax already defined", tag);
    appendSyntax((Rules){tag.lexeme, tag.lexeme, prior, '_', leftPrecedence,
        rightPrecedence, fixity, associativity, special, reducer});
}

void popSyntax(void) { unappend(RULES); }

void initSyntax(void) { RULES = newArray(1024); }

void addCoreSyntax(const char* symbol, Precedence leftPrecedence,
        Precedence rightPrecedence, Fixity fixity, Associativity associativity,
        Node* (*reducer)(Node*, Node*, Node*)) {
    String lexeme = newString(symbol, (unsigned int)strlen(symbol));
    appendSyntax((Rules){lexeme, lexeme, newString("", 0), '_', leftPrecedence,
        rightPrecedence, fixity, associativity, true, reducer});
}

void addBracketSyntax(const char* symbol, char type, Precedence outerPrecedence,
    Fixity fixity, Node* (*reducer)(Node*, Node*, Node*)) {
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
