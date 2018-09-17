#include <stdlib.h>
#include "lib/array.h"
#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "scan.h"       // isSpaceCharacter
#include "errors.h"
#include "operators.h"

static Array* RULES = NULL;

typedef struct {
    const char* symbol;
    Precedence leftPrecedence, rightPrecedence;
    Fixity fixity;
    Associativity associativity;
    void (*shift)(Stack* stack, Node* operator);
    Node* (*reduce)(Node* operator, Node* left, Node* right);
} Rules;

bool isSpaceOperator(Node* node) {
    return isOperator(node) && ((Rules*)getValue(node))->symbol[0] == ' ';
}

bool isSpecial(Node* node) {
    if (!isOperator(node))
        return false;
    Rules* rules = (Rules*)getValue(node);
    return (rules->leftPrecedence <= 5 || rules->rightPrecedence <= 5);
}

Fixity getFixity(Node* operator) {
    return ((Rules*)getValue(operator))->fixity;
}

bool isOpenOperator(Node* node) {
    return isOperator(node) && getFixity(node) == OPEN;
}

Node* reduceBracket(Node* open, Node* close, Node* left, Node* right) {
    return ((Rules*)getValue(close))->reduce(open, left, right);
}

static Node* propagateSection(Node* operator, Node* node, const char* name) {
    if (isSpecial(operator) || !isApplication(node))
        syntaxError("operator does not support sections", operator);
    return setTag(node, renameTag(getTag(node), name));
}

Node* reduceOperator(Node* operator, Node* left, Node* right) {
    Node* result = ((Rules*)getValue(operator))->reduce(operator, left, right);
    if (left != NULL && isSection(left))
        return propagateSection(operator, result,
            right != NULL && isSection(right) ?  "_._" : "_.");
    if (right != NULL && isSection(right))
        return propagateSection(operator, result, "._");
    return result;
}

void shiftOperator(Stack* stack, Node* operator) {
    ((Rules*)getValue(operator))->shift(stack, operator);
}

bool isHigherPrecedence(Node* left, Node* right) {
    assert(isOperator(left) || isOperator(right));
    if (isEOF(left))
        return false;
    Rules* leftRules = (Rules*)getValue(left);
    Rules* rightRules = (Rules*)getValue(right);
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

void addOperator(const char* symbol, Precedence leftPrecedence,
        Precedence rightPrecedence, Fixity fixity, Associativity associativity,
        void (*shift)(Stack*, Node*), Node* (*reduce)(Node*, Node*, Node*)) {
    if (RULES == NULL)
        RULES = newArray(1024);
    Rules* rules = (Rules*)malloc(sizeof(Rules));
    *rules = (Rules){symbol, leftPrecedence, rightPrecedence, fixity,
        associativity, shift, reduce};
    append(RULES, rules);
}

static bool allowsOperatorBefore(Rules rules) {
    return rules.fixity == PRE || rules.fixity == OPEN || rules.fixity == CLOSE;
}

static Rules* getRules(String lexeme, bool isAfterOperator) {
    if (isSpaceCharacter(lexeme.start[0]))
        lexeme = newString(" ", 1);
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

static Node* setRules(Node* operator, bool isAfterOperator) {
    Rules* rules = getRules(getLexeme(operator), isAfterOperator);
    return setValue(operator, (long long)rules);
}

Node* getPrevious(Stack* stack) {
    return isSpaceOperator(peek(stack, 0)) ? peek(stack, 1) : peek(stack, 0);
}

Node* parseOperator(Tag tag, Stack* stack) {
    return setRules(newOperator(tag),
        isEmpty(stack) || isOperator(getPrevious(stack)));
}
