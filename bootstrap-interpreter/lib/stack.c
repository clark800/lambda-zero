#include "tree.h"
#include "stack.h"

Stack* newStack() {return (Stack*)newPair(NULL, NULL);}
void deleteStack(Stack* stack) {release(hold((Node*)stack));}
static Node* getHead(Stack* stack) {return getRight((Node*)stack);}
static void setHead(Stack* stack, Node* head) {setRight((Node*)stack, head);}
bool isEmpty(Stack* stack) {return getHead(stack) == NULL;}
Iterator* iterate(Stack* stack) {return (Iterator*)getHead(stack);}
Node* cursor(Iterator* iterator) {return getLeft((Node*)iterator);}
bool end(Iterator* iterator) {return (Node*)iterator == NULL;}

Iterator* next(Iterator* iterator) {
    return (Iterator*)getRight((Node*)iterator);
}

void push(Stack* stack, Node* node) {
    setHead(stack, newPair(node, getHead(stack)));
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
