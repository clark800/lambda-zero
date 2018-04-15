#ifndef STACK_H
#define STACK_H

#include <stddef.h>
#include "tree.h"

typedef struct Stack Stack;
typedef struct Iterator Iterator;

Stack* newStack(Node* head);
void deleteStack(Stack* stack);
bool isEmpty(Stack* stack);
void push(Stack* stack, Node* node);
Hold* pop(Stack* stack);
Node* peek(Stack* stack, size_t i);
Iterator* iterate(Stack* stack);
Iterator* next(Iterator* iterator);
Node* cursor(Iterator* iterator);
bool end(Iterator* iterator);

#endif
