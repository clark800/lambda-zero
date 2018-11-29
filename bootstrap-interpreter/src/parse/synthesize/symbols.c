#include <string.h>
#include "shared/lib/array.h"
#include "shared/lib/tree.h"
#include "shared/lib/util.h"
#include "shared/lib/stack.h"
#include "parse/shared/errors.h"
#include "parse/shared/ast.h"
#include "symbols.h"

static Array* RULES = NULL;     // note: this never gets free'd

typedef struct {
    String lexeme, prior;
    Precedence leftPrecedence, rightPrecedence;
    Fixity fixity;
    Associativity associativity;
    bool special;
    void (*shift)(Stack* stack, Node* operator);
    Node* (*reduce)(Node* operator, Node* left, Node* right);
} Rules;

static void shiftOperand(Stack* stack, Node* node) {
    if (!isEmpty(stack) && !isOperator(peek(stack, 0)))
        syntaxError("missing operator before", node);
    push(stack, reduce(node, VOID, VOID));
}

static Node* reduceOperand(Node* operand, Node* left, Node* right) {
    (void)left, (void)right;
    return operand;
}

static inline Rules* getRules(Node* node) {
    static Rules operand =
        {{"", 0}, {"", 0}, 0, 0, NOFIX, N, false, shiftOperand, reduceOperand};
    if (!isOperator(node))
        return &operand;
    Rules* rules = (Rules*)getData(node);
    return rules == NULL ? &operand : rules;
}

Fixity getFixity(Node* operator) {
    return getRules(operator)->fixity;
}

void erase(Stack* stack, const char* lexeme) {
    if (!isEmpty(stack) && isThisOperator(peek(stack, 0), lexeme))
        release(pop(stack));
}

bool isSpecial(Node* node) {
    return isOperator(node) && ((Rules*)getRules(node))->special;
}

bool isOpenOperator(Node* node) {
    return isOperator(node) && getFixity(node) == OPENFIX;
}

Node* reduceBracket(Node* open, Node* close, Node* left, Node* right) {
    return getRules(close)->reduce(open, left, right);
}

static Node* propagateSection(Node* operator, SectionVariety side, Node* body) {
    if (isSpecial(operator) || !isJuxtaposition(body))
        syntaxError("operator does not support sections", operator);
    return Section(getTag(operator), side, body);
}

Node* reduce(Node* operator, Node* left, Node* right) {
    Node* leftOp = isSection(left) ? getSectionBody(left) : left;
    Node* rightOp = isSection(right) ? getSectionBody(right) : right;
    Node* result = getRules(operator)->reduce(operator, leftOp, rightOp);
    if (isSection(left))
        return propagateSection(operator, isSection(right) ?
            LEFTRIGHTSECTION : RIGHTSECTION, result);
    if (isSection(right))
        return propagateSection(operator, LEFTSECTION, result);
    return result;
}

void shift(Stack* stack, Node* node) {
    getRules(node)->shift(stack, node);
}

bool isHigherPrecedence(Node* left, Node* right) {
    assert(isOperator(left) && isOperator(right));
    if (isEOF(left))
        return false;

    Rules* leftRules = getRules(left);
    Rules* rightRules = getRules(right);

    if (leftRules->rightPrecedence == rightRules->leftPrecedence) {
        static const char* message = "operator is non-associative";
        syntaxErrorIf(leftRules->associativity == N, message, left);
        syntaxErrorIf(rightRules->associativity == N, message, right);

        if (leftRules->associativity != rightRules->associativity)
            syntaxError("incompatible associativity", right);

        if (leftRules->associativity == RV)
            return getValue(left) > getValue(right);

        return leftRules->associativity == L;
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

Node* parseSymbol(Tag tag, long long value) {
    Rules* rules = findRules(tag.lexeme);
    if (rules == NULL && isThisString(tag.lexeme, " "))
        return Operator(tag, value, findRules(newString("( )", 3)));
    return rules == NULL ? Name(tag) : Operator(tag, value, rules);
}

static void reduceTop(Stack* stack) {
    Hold* right = pop(stack);
    Hold* operator = pop(stack);
    Hold* left = getFixity(getNode(operator)) == INFIX ?
        pop(stack) : hold(VOID);
    // if a syntax declaration marker is about to be an argument to a reducer
    // then the scope of the syntax declaration has ended
    if (isSyntaxDefinition(getNode(left)))
        unappend(RULES);
    shift(stack, reduce(getNode(operator), getNode(left), getNode(right)));
    release(right);
    release(operator);
    release(left);
}

void reduceLeft(Stack* stack, Node* operator) {
    while (!isOperator(peek(stack, 0)) &&
            isHigherPrecedence(peek(stack, 1), operator))
        reduceTop(stack);
}

static void appendSyntax(Rules rules) {
    Rules* new = (Rules*)smalloc(sizeof(Rules));
    *new = rules;
    append(RULES, new);
}

void addSyntax(Tag tag, Precedence leftPrecedence, Precedence rightPrecedence,
        Fixity fixity, Associativity associativity,
        void (*shifter)(Stack*, Node*), Node* (*reducer)(Node*, Node*, Node*)) {
    tokenErrorIf(findRules(tag.lexeme) != NULL, "syntax already defined", tag);
    appendSyntax((Rules){tag.lexeme, {"", 0}, leftPrecedence, rightPrecedence,
        fixity, associativity, false, shifter, reducer});
}

void addCoreSyntax(const char* symbol, Precedence leftPrecedence,
        Precedence rightPrecedence, Fixity fixity, Associativity associativity,
        void (*shifter)(Stack*, Node*), Node* (*reducer)(Node*, Node*, Node*)) {
    if (RULES == NULL)
        RULES = newArray(1024);
    bool special = strncmp(symbol, "(+)", 4) && strncmp(symbol, "(-)", 4);
    String lexeme = newString(symbol, (unsigned int)strlen(symbol));
    appendSyntax((Rules){lexeme, {"", 0}, leftPrecedence, rightPrecedence,
        fixity, associativity, special, shifter, reducer});
}

static Node* reduceMixfix(Node* operator, Node* left, Node* right) {
    Tag tag = getTag(operator);
    String prior = getRules(operator)->prior;
    if (!isJuxtaposition(left) || !isSameString(getLexeme(left), prior))
        syntaxError("mixfix syntax error", operator);
    return Juxtaposition(tag, Juxtaposition(tag, Name(tag), left), right);
}

void addMixfixSyntax(Tag tag, Node* prior, void (*shifter)(Stack*, Node*)) {
    tokenErrorIf(findRules(tag.lexeme) != NULL, "syntax already defined", tag);
    Rules* rules = findRules(getLexeme(prior));
    syntaxErrorIf(rules == NULL, "syntax not defined", prior);
    Precedence p = rules->rightPrecedence;
    appendSyntax((Rules){tag.lexeme, getLexeme(prior), p, p, INFIX, L, false,
        shifter, reduceMixfix});
}
