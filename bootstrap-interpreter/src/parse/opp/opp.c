#include "tree.h"
#include "stack.h"
#include "errors.h"
#include "operator.h"
#include "opp.h"

void reduceLeft(Stack* stack, Node* op);

NodeStack* newNodeStack() { return (NodeStack*)newStack(); }
void deleteNodeStack(NodeStack* stack) { deleteStack((Stack*)stack); }

Node* getTop(NodeStack* stack) {
    return isEmpty((Stack*)stack) ? NULL : peek((Stack*)stack, 0);
}

void eraseNode(Stack* stack, const char* lexeme) {
    if (!isEmpty(stack) && isThisOperator(peek(stack, 0), lexeme))
        release(pop(stack));
}

void erase(NodeStack* stack, const char* lexeme) {
    eraseNode((Stack*)stack, lexeme);
}

Node* reduceBracket(Node* open, Node* close, Node* before, Node* contents) {
    return reduce(close, before, reduce(open, before, contents));
}

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

void pushBracket(Stack* stack, Node* open, Node* close, Node* contents) {
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
        pushBracket(stack, getNode(contents), close, NULL);
    } else {
        Hold* open = pop(stack);
        pushBracket(stack, getNode(open), close, getNode(contents));
        release(open);
    }
    release(contents);
}

static void shiftNode(Stack* stack, Node* node) {
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

void shift(NodeStack* stack, Node* node) { shiftNode((Stack*)stack, node); }

static void reduceTop(Stack* stack) {
    Hold* right = pop(stack);
    Hold* operator = pop(stack);
    Fixity fixity = getFixity(getNode(operator));
    Hold* left = fixity == INFIX ? pop(stack) : hold(VOID);
    shiftNode(stack, reduce(getNode(operator), getNode(left), getNode(right)));
    release(right);
    release(operator);
    release(left);
}

void reduceLeft(Stack* stack, Node* operator) {
    while (!isEmpty(stack) && !isOperator(peek(stack, 0)) &&
            isHigherPrecedence(peek(stack, 1), operator))
        reduceTop(stack);
}

void debugNodeStack(NodeStack* nodeStack, void debugNode(Node*)) {
    Stack* stack = (Stack*)nodeStack;
    Stack* reversed = newStack();
    for (Iterator* it = iterate(stack); !end(it); it = next(it))
        push(reversed, cursor(it));
    for (Iterator* it = iterate(reversed); !end(it); it = next(it)) {
        debugNode(cursor(it));
        fputs(" | ", stderr);
    }
    deleteStack(reversed);
}
