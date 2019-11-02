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

Node* reduceBracket(Node* open, Node* close, Node* before, Node* contents) {
    return reduce(close, before, reduce(open, before, contents));
}

void shiftBracket(Stack* stack, Node* open, Node* close, Node* contents) {
    if (contents != NULL && isOperator(contents))
        syntaxError("missing right operand for", contents);

    if (getBracketType(open) != getBracketType(close)) {
        if (getBracketType(close) == '\0')
            syntaxError("missing close for", open);
        else syntaxError("missing open for", close);
    }

    if (isEmpty(stack) || isOperator(peek(stack, 0))) {
        push(stack, reduceBracket(open, close, NULL, contents));
    } else {
        Hold* left = pop(stack);
        push(stack, reduceBracket(open, close, getNode(left), contents));
        release(left);
    }
}

void shiftClose(Stack* stack, Node* close) {
    Hold* contents = pop(stack);
    if (isOpenOperator(getNode(contents))) {
        shiftBracket(stack, getNode(contents), close, NULL);
    } else {
        Hold* open = pop(stack);
        shiftBracket(stack, getNode(open), close, getNode(contents));
        release(open);
    }
    release(contents);
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
