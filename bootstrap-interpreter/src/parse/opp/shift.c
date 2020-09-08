#include "tree.h"
#include "stack.h"
#include "errors.h"
#include "operator.h"
#include "shift.h"

void shiftOperand(Stack* stack, Node* operand) {
    if (!isEmpty(stack)) {
        Node* top = peek(stack, 0);
        if (isOperator(top)) {
            Node* operator = getBinaryPrefixInfixOperator(top);
            if (operator != NULL) {
                release(pop(stack));
                push(stack, operand);
                push(stack, operator);
            } else push(stack, operand);
        } else syntaxError("missing operator before", operand);
    } else push(stack, operand);
}

void shiftPostfix(Stack* stack, Node* operator) {
    if (isOperator(peek(stack, 0)))
        syntaxError("missing left operand for", operator);
    Hold* operand = pop(stack);
    shiftOperand(stack, reduce(operator, getNode(operand), VOID));
    release(operand);
}

void shiftInfix(Stack* stack, Node* operator) {
    if (isOperator(peek(stack, 0)))
        syntaxError("missing left operand for", operator);
    push(stack, operator);
}

void shiftBracket(Stack* stack, Node* open, Node* close, Node* contents) {
    if (isEmpty(stack) || isOperator(peek(stack, 0))) {
        shiftOperand(stack, reduceBracket(open, close, NULL, contents));
    } else {
        Hold* before = pop(stack);
        shiftOperand(stack,
            reduceBracket(open, close, getNode(before), contents));
        release(before);
    }
}

void shiftClose(Stack* stack, Node* close) {
    Hold* contentsHold = pop(stack);
    Node* contents = getNode(contentsHold);
    if (isOperator(contents)) {
        if (getFixity(contents) == OPENFIX)
            shiftBracket(stack, contents, close, NULL);
        else syntaxError("missing right operand for", contents);
    } else {
        Hold* open = pop(stack);
        shiftBracket(stack, getNode(open), close, contents);
        release(open);
    }
    release(contentsHold);
}

static void reduceTop(Stack* stack) {
    Hold* right = pop(stack);
    Hold* operator = pop(stack);
    Fixity fixity = getFixity(getNode(operator));
    Hold* left = fixity == INFIX ? pop(stack) : hold(VOID);
    Node* operand = reduce(getNode(operator), getNode(left), getNode(right));
    shiftOperand(stack, operand);
    release(right);
    release(operator);
    release(left);
}

void reduceLeft(Stack* stack, Node* operator) {
    while (!isEmpty(stack) && !isOperator(peek(stack, 0)) &&
            isHigherPrecedence(peek(stack, 1), operator))
        reduceTop(stack);
}

void shiftOperator(Stack* stack, Node* operator) {
    reduceLeft(stack, operator);
    switch(getFixity(operator)) {
        case INFIX: shiftInfix(stack, operator); break;
        case POSTFIX: shiftPostfix(stack, operator); break;
        case CLOSEFIX: shiftClose(stack, operator); break;
        default: push(stack, operator); break;
    }
}

void shift(Stack* stack, Node* node) {
    if (isOperator(node))
        shiftOperator(stack, node);
    else shiftOperand(stack, node);
}
