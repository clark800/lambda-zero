#include <assert.h>
#include "stack.h"

Stack* newStack(Node* head) {
    return (Stack*)hold(newBranch(0, VOID, head));
}

void deleteStack(Stack* stack) {
    release((Hold*)stack);
}

Node* getHead(Stack* stack) {
    return getRight((Node*)stack);
}

void setHead(Stack* stack, Node* head) {
    setRight((Node*)stack, head);
}

bool isEmpty(Stack* stack) {
    return getHead(stack) == VOID;
}

void push(Stack* stack, Node* node) {
    setHead(stack, newBranch(0, node, getHead(stack)));
}

Hold* pop(Stack* stack) {
    assert(!isEmpty(stack));
    Node* head = getHead(stack);
    Hold* popped = hold(getLeft(head));
    setHead(stack, getRight(head));
    return popped;
}

Node* peek(Stack* stack, size_t i) {
    assert(!isEmpty(stack));
    return getListElement(getHead(stack), i);
}

Iterator* iterate(Stack* stack) {
   return (Iterator*)getHead(stack);
}

Iterator* next(Iterator* iterator) {
    return (Iterator*)getRight((Node*)iterator);
}

Node* cursor(Iterator* iterator) {
    return getLeft((Node*)iterator);
}

bool end(Iterator* iterator) {
    return (Node*)iterator == VOID;
}
