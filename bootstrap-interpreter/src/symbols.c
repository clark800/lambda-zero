#include <string.h>
#include "lib/array.h"
#include "lib/tree.h"
#include "lib/util.h"
#include "lib/stack.h"
#include "ast.h"
#include "errors.h"
#include "symbols.h"

static Array* RULES = NULL;

typedef struct {
    String lexeme;
    Precedence leftPrecedence, rightPrecedence;
    Fixity fixity;
    Associativity associativity;
    bool special;
    void (*shift)(Stack* stack, Node* operator);
    Node* (*reduce)(Node* operator, Node* left, Node* right);
} Rules;

static void shiftOperand(Stack* stack, Node* node) {
    Node* operand = reduce(node, NULL, NULL);
    if (isEmpty(stack) || isOperator(peek(stack, 0))) {
        push(stack, operand);
    } else {
        Hold* left = pop(stack);
        push(stack, newApplication(getTag(node), getNode(left), operand));
        release(left);
    }
}

static Node* reduceOperand(Node* operand, Node* left, Node* right) {
    (void)left, (void)right;
    return operand;
}

static inline Rules* getRules(Node* node) {
    static Rules operand =
        {{"", 0}, 0, 0, NOFIX, N, false, shiftOperand, reduceOperand};
    if (!isSymbol(node))
        return &operand;
    Rules* rules = (Rules*)getData(node);
    return rules == NULL ? &operand : rules;
}

Fixity getFixity(Node* operator) {
    return getRules(operator)->fixity;
}

bool isOperator(Node* node) {
    return isSymbol(node) && getFixity(node) != NOFIX;
}

bool isSpace(Node* node) {
    return isOperator(node) && isThisLeaf(node, " ");
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

static Node* propagateSection(Node* operator, Node* node, const char* name) {
    if (isSpecial(operator) || !isApplication(node))
        syntaxError("operator does not support sections", operator);
    setTag(node, renameTag(getTag(node), name));
    return node;
}

Node* reduce(Node* operator, Node* left, Node* right) {
    Node* result = getRules(operator)->reduce(operator, left, right);
    if (left != NULL && isSection(left))
        return propagateSection(operator, result,
            right != NULL && isSection(right) ?  "_._" : "_.");
    if (right != NULL && isSection(right))
        return propagateSection(operator, result, "._");
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
        const char* message = "operator is non-associative";
        syntaxErrorIf(leftRules->associativity == N, message, left);
        syntaxErrorIf(rightRules->associativity == N, message, right);
    }

    return rightRules->associativity == R ?
        leftRules->rightPrecedence > rightRules->leftPrecedence :
        leftRules->rightPrecedence >= rightRules->leftPrecedence;
}

static Rules* findRules(String lexeme) {
    for (unsigned int i = 0; i < length(RULES); ++i) {
        Rules* rules = elementAt(RULES, i);
        if (isSameString(lexeme, rules->lexeme))
            return rules;
    }
    return NULL;
}

Node* parseSymbol(Tag tag) {
    Rules* rules = findRules(tag.lexeme);
    if (rules == NULL && isThisString(tag.lexeme, " "))
        return newSymbol(tag, findRules(newString("( )", 3)));
    return newSymbol(tag, rules);
}

static void reduceTop(Stack* stack) {
    Hold* right = pop(stack);
    Hold* operator = pop(stack);
    Hold* left = getFixity(getNode(operator)) == INFIX ? pop(stack) : NULL;
    shift(stack, reduce(getNode(operator), getNode(left), getNode(right)));
    release(right);
    release(operator);
    if (left != NULL)
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

void addSyntax(Tag tag, Precedence leftPrecedence,
        Precedence rightPrecedence, Fixity fixity, Associativity associativity,
        void (*shifter)(Stack*, Node*), Node* (*reducer)(Node*, Node*, Node*)) {
    tokenErrorIf(findRules(tag.lexeme) != NULL, "syntax already defined", tag);
    appendSyntax((Rules){tag.lexeme, leftPrecedence, rightPrecedence, fixity,
        associativity, false, shifter, reducer});
}

void addBuiltinSyntax(const char* symbol, Precedence leftPrecedence,
        Precedence rightPrecedence, Fixity fixity, Associativity associativity,
        void (*shifter)(Stack*, Node*), Node* (*reducer)(Node*, Node*, Node*)) {
    if (RULES == NULL)
        RULES = newArray(1024);
    bool special = strncmp(symbol, "(+)", 4) && strncmp(symbol, "(-)", 4);
    String lexeme = newString(symbol, (unsigned int)strlen(symbol));
    appendSyntax((Rules){lexeme, leftPrecedence, rightPrecedence, fixity,
        associativity, special, shifter, reducer});
}

void addScopeMarker(void) {
    addBuiltinSyntax("\0", 0, 0, NOFIX, N, NULL, NULL);
}

void endScope(void) {
    while (((Rules*)elementAt(RULES, length(RULES) - 1))->lexeme.start[0] != 0)
        unappend(RULES);
    unappend(RULES);
    if (length(RULES) == 0)
        deleteArray(RULES);
}
