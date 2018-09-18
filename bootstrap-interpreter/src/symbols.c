#include <stdlib.h>
#include "lib/array.h"
#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "errors.h"
#include "symbols.h"

static Array* RULES = NULL;

typedef struct {
    const char* symbol;
    Precedence leftPrecedence, rightPrecedence;
    Fixity fixity;
    Associativity associativity;
    void (*shift)(Stack* stack, Node* operator);
    Node* (*reduce)(Node* operator, Node* left, Node* right);
} Rules;

static inline Rules* getRules(Node* node) {
    if (!isSymbol(node))
        return elementAt(RULES, 0);
    Rules* rules = (Rules*)getData(node);
    return rules == NULL ? elementAt(RULES, 0) : rules;
}

Fixity getFixity(Node* operator) {
    return getRules(operator)->fixity;
}

bool isOperator(Node* node) {
    return isSymbol(node) && getFixity(node) != NOFIX;
}

bool isSpaceOperator(Node* node) {
    return isOperator(node) && getRules(node)->symbol[0] == ' ';
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

void addSymbol(const char* symbol, Precedence leftPrecedence,
        Precedence rightPrecedence, Fixity fixity, Associativity associativity,
        void (*shifter)(Stack*, Node*), Node* (*reducer)(Node*, Node*, Node*)) {
    if (RULES == NULL)
        RULES = newArray(1024);
    Rules* rules = (Rules*)malloc(sizeof(Rules));
    *rules = (Rules){symbol, leftPrecedence, rightPrecedence, fixity,
        associativity, shifter, reducer};
    append(RULES, rules);
}

static bool allowsOperatorBefore(Rules rules) {
    return rules.fixity == PREFIX || rules.fixity == OPENFIX ||
        rules.fixity == CLOSEFIX || rules.fixity == NOFIX;
}

static Rules* findRules(String lexeme, bool isAfterOperator) {
    Rules* result = elementAt(RULES, 0);
    for (unsigned int i = 0; i < length(RULES); ++i) {
        Rules* rules = (Rules*)elementAt(RULES, i);
        if (isThisString(lexeme, rules->symbol)) {
            result = rules;
            if (!isAfterOperator || allowsOperatorBefore(*rules))
                return result;
        }
    }
    return result;
}

static Node* getPrevious(Stack* stack) {
    return isSpaceOperator(peek(stack, 0)) ? peek(stack, 1) : peek(stack, 0);
}

Node* parseSymbol(Tag tag, Stack* stack) {
    return newSymbol(tag, findRules(tag.lexeme,
        isEmpty(stack) || isOperator(getPrevious(stack))));
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
