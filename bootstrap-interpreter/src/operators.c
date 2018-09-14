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
    return ((Rules*)getValue(node))->symbol[0] == ' ';
}

bool isSpecialOperator(Node* operator) {
    Rules* rules = (Rules*)getValue(operator);
    return rules->leftPrecedence <= 5 || rules->rightPrecedence <= 5;
}

Fixity getFixity(Node* operator) {
    return ((Rules*)getValue(operator))->fixity;
}

Node* reduceOperator(Node* operator, Node* left, Node* right) {
    return ((Rules*)getValue(operator))->reduce(operator, left, right);
}

void shiftOperator(Stack* stack, Node* operator) {
    ((Rules*)getValue(operator))->shift(stack, operator);
}

bool isHigherPrecedence(Node* left, Node* right) {
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

Node* parseOperator(Tag tag, Stack* stack) {
    Node* operator = newOperator(tag);
    setRules(operator, isEmpty(stack) || isOperator(peek(stack, 0)));
    if (getFixity(operator) == PRE && isOperator(peek(stack, 0)) &&
            isSpaceOperator(peek(stack, 0)))
        setRules(operator, isOperator(peek(stack, 1)));
    return operator;
}

