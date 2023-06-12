#include "tree.h"
#include "stack.h"
#include "errors.h"
#include "operator.h"
#include "shift.h"

static void shiftOperand(Stack* stack, Node* operand) {
    if (!isEmpty(stack) && !isOperator(peek(stack, 0)))
        syntaxErrorNode("missing operator before", operand);
    push(stack, operand);
}

static void shiftPrefix(Stack* stack, Node* operator) {
    if (!isOperator(peek(stack, 0)))
        syntaxErrorNode("missing operator before", operator);
    push(stack, operator);
}

static void shiftPostfix(Stack* stack, Node* operator) {
    if (isOperator(peek(stack, 0)))
        syntaxErrorNode("missing left operand for", operator);
    Hold* operand = pop(stack);
    shiftOperand(stack, reduce(operator, getNode(operand), VOID));
    release(operand);
}

static void shiftInfix(Stack* stack, Node* operator) {
    if (isOperator(peek(stack, 0)))
        syntaxErrorNode("missing left operand for", operator);
    push(stack, operator);
}

static void shiftBracket(Stack* stack, Node* open, Node* close, Node* contents){
    if (isEmpty(stack) || isOperator(peek(stack, 0))) {
        shiftOperand(stack, reduceBracket(open, close, NULL, contents));
    } else {
        Hold* before = pop(stack);
        shiftOperand(stack,
            reduceBracket(open, close, getNode(before), contents));
        release(before);
    }
}

static void shiftClose(Stack* stack, Node* close) {
    Hold* contentsHold = pop(stack);
    Node* contents = getNode(contentsHold);
    if (isOperator(contents)) {
        if (getFixity(contents) == OPENFIX)
            shiftBracket(stack, contents, close, NULL);
        else syntaxErrorNode("missing right operand for", contents);
    } else {
        Hold* open = pop(stack);
        shiftBracket(stack, getNode(open), close, contents);
        release(open);
    }
    release(contentsHold);
}

static Tag reduceTop(Stack* stack) {
    Hold* right = pop(stack);
    Hold* operator = pop(stack);
    Tag tag = getTag(getNode(operator));
    Fixity fixity = getFixity(getNode(operator));
    Hold* left = fixity == INFIX ? pop(stack) : hold(VOID);
    Node* operand = reduce(getNode(operator), getNode(left), getNode(right));
    shiftOperand(stack, operand);
    release(right);
    release(operator);
    release(left);
    return tag;
}

static void reduceLeft(Stack* stack, Node* operator) {
    Tag tag = {0};
    if (!isEmpty(stack))
        while (!isOperator(peek(stack, 0)) &&
                isHigherPrecedence(peek(stack, 1), operator))
            tag = reduceTop(stack);
    Lexeme prior = getPrior(operator);
    if (prior.length > 0 && !isSameLexeme(getLexeme(tag), prior))
        syntaxErrorNode("invalid prior for", operator);
}

static void shiftOperator(Stack* stack, Node* operator) {
    reduceLeft(stack, operator);
    switch (getFixity(operator)) {
        case NOFIX: shiftOperand(stack, reduce(operator, NULL, NULL)); break;
        case INFIX: shiftInfix(stack, operator); break;
        case PREFIX: shiftPrefix(stack, operator); break;
        case POSTFIX: shiftPostfix(stack, operator); break;
        case OPENFIX: push(stack, operator); break;
        case CLOSEFIX: shiftClose(stack, operator); break;
    }
}

void shift(Stack* stack, Node* node) {
    if (isOperator(node))
        shiftOperator(stack, node);
    else shiftOperand(stack, node);
}
