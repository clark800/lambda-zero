#include "tree.h"
#include "stack.h"
#include "errors.h"
#include "operator.h"
#include "shift.h"

void shiftPostfix(Stack* stack, Node* operator) {
    if (isOperator(peek(stack, 0)))
        syntaxError("missing left operand for", operator);
    Hold* operand = pop(stack);
    push(stack, reduce(operator, getNode(operand), VOID));
    release(operand);
}

void shiftInfix(Stack* stack, Node* operator) {
    if (isOperator(peek(stack, 0)))
        syntaxError("missing left operand for", operator);
    push(stack, operator);
}

void shiftBracket(Stack* stack, Node* open, Node* close, Node* contents) {
    if (isEmpty(stack) || isOperator(peek(stack, 0))) {
        push(stack, reduceBracket(open, close, NULL, contents));
    } else {
        Hold* before = pop(stack);
        push(stack, reduceBracket(open, close, getNode(before), contents));
        release(before);
    }
}

void shiftClose(Stack* stack, Node* close) {
    Hold* hold = pop(stack);
    Node* contents = getNode(hold);
    if (isOperator(contents)) {
        if (getFixity(contents) == OPENFIX)
            shiftBracket(stack, contents, close, NULL);
        else syntaxError("missing right operand for", contents);
    } else {
        Hold* open = pop(stack);
        shiftBracket(stack, getNode(open), close, contents);
        release(open);
    }
    release(hold);
}

static void reduceTop(Stack* stack) {
    Hold* right = pop(stack);
    Hold* operator = pop(stack);
    Fixity fixity = getFixity(getNode(operator));
    Hold* left = fixity == INFIX ? pop(stack) : hold(VOID);
    shift(stack, reduce(getNode(operator), getNode(left), getNode(right)));
    release(right);
    release(operator);
    release(left);
}

void reduceLeft(Stack* stack, Node* operator) {
    while (!isEmpty(stack) && !isOperator(peek(stack, 0)) &&
            isHigherPrecedence(peek(stack, 1), operator))
        reduceTop(stack);
}

void shift(Stack* stack, Node* node) {
    if (isOperator(node)) {
        reduceLeft(stack, node);
        switch(getFixity(node)) {
            case INFIX: shiftInfix(stack, node); break;
            case POSTFIX: shiftPostfix(stack, node); break;
            case CLOSEFIX: shiftClose(stack, node); break;
            default: push(stack, node); break;
        }
    } else {
        if (!isEmpty(stack) && !isOperator(peek(stack, 0)))
            syntaxError("missing operator before", node);
        push(stack, node);
    }
}
