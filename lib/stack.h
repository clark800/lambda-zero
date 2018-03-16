#ifndef STACK_H
#define STACK_H

#include <assert.h>
#include <stddef.h>
#include "tree.h"

typedef struct Stack Stack;
typedef struct Iterator Iterator;

static inline Stack* newStack(Node* head) {
    return (Stack*)hold(newBranchNode(0, head, VOID));
}

static inline void deleteStack(Stack* stack) {
    release((Hold*)stack);
}

static inline Node* getHead(Stack* stack) {
    return getLeft((Node*)stack);
}

static inline void setHead(Stack* stack, Node* head) {
    setLeft((Node*)stack, head);
}

static inline bool isEmpty(Stack* stack) {
    return getHead(stack) == VOID;
}

static inline void push(Stack* stack, Node* node) {
    setHead(stack, newBranchNode(0, node, getHead(stack)));
}

static inline Hold* pop(Stack* stack) {
    assert(!isEmpty(stack));
    Node* head = getHead(stack);
    Hold* popped = hold(getLeft(head));
    setHead(stack, getRight(head));
    return popped;
}

static inline Node* peek(Stack* stack, size_t i) {
    assert(!isEmpty(stack));
    Node* node = getHead(stack); 
    for (; i > 0; i--)
        node = getRight(node);
    return getLeft(node);
}

static inline Node* peekSafe(Stack* stack, size_t i) {
    Node* node = getHead(stack); 
    for (; i > 0 && node != VOID; i--)
        node = getRight(node);
    return node == VOID ? NULL : getLeft(node);
}

static inline Iterator* iterate(Stack* stack) {
   return (Iterator*)getHead(stack); 
}

static inline Iterator* next(Iterator* iterator) {
    return (Iterator*)getRight((Node*)iterator);
}

static inline Node* cursor(Iterator* iterator) {
    return getLeft((Node*)iterator);
}

static inline bool end(Iterator* iterator) {
    return (Node*)iterator == VOID;
}

#endif
