#include <stdlib.h>
#include <string.h>
#include "lib/array.h"
#include "lib/tree.h"
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
        {{"", 0}, 0, 0, NOFIX, N, shiftOperand, reduceOperand};
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

bool isSpaceOperator(Node* node) {
    return isOperator(node) && getRules(node)->lexeme.start[0] == ' ';
}

bool isSpecial(Node* node) {
    if (!isOperator(node))
        return false;
    Rules* rules = getRules(node);
    return (rules->leftPrecedence <= 5 || rules->rightPrecedence <= 5);
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
    return setTag(node, renameTag(getTag(node), name));
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

    if (rightRules->associativity == R)
        return leftRules->rightPrecedence > rightRules->leftPrecedence;
    else
        return leftRules->rightPrecedence >= rightRules->leftPrecedence;
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
    return newSymbol(tag, findRules(tag.lexeme));
}

static void reduceTop(Stack* stack) {
    Hold* right = pop(stack);
    Hold* op = pop(stack);
    Node* operator = getNode(op);
    Hold* left = getFixity(operator) == INFIX ? pop(stack) : NULL;
    shift(stack, reduce(operator, getNode(left), getNode(right)));
    release(right);
    release(op);
    if (left != NULL)
        release(left);
}

void reduceLeft(Stack* stack, Node* operator) {
    while (!isOperator(peek(stack, 0)) &&
            isHigherPrecedence(peek(stack, 1), operator))
        reduceTop(stack);
}

static void appendSyntax(Rules rules) {
    Rules* new = (Rules*)malloc(sizeof(Rules));
    *new = rules;
    append(RULES, new);
}

void addSyntax(Node* name, Precedence leftPrecedence,
        Precedence rightPrecedence, Fixity fixity, Associativity associativity,
        void (*shifter)(Stack*, Node*), Node* (*reducer)(Node*, Node*, Node*)) {
    String lexeme = getLexeme(name);
    syntaxErrorIf(findRules(lexeme) != NULL, "syntax already defined", name);
    appendSyntax((Rules){lexeme, leftPrecedence, rightPrecedence, fixity,
        associativity, shifter, reducer});
}

void addBuiltinSyntax(const char* symbol, Precedence leftPrecedence,
        Precedence rightPrecedence, Fixity fixity, Associativity associativity,
        void (*shifter)(Stack*, Node*), Node* (*reducer)(Node*, Node*, Node*)) {
    if (RULES == NULL)
        RULES = newArray(1024);
    String lexeme = newString(symbol, (unsigned int)strlen(symbol));
    appendSyntax((Rules){lexeme, leftPrecedence, rightPrecedence, fixity,
        associativity, shifter, reducer});
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
