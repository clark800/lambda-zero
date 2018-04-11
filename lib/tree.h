#ifndef TREE_H
#define TREE_H

#include <stdbool.h>

typedef struct Node Node;
extern Node* const VOID;

void initNodeAllocator(void);
void destroyNodeAllocator(void);

Node* newBranch(int location, Node* left, Node* right);
Node* newLeaf(int location, long long type);

bool isLeaf(Node* node);
bool isBranch(Node* node);
int getLocation(Node* node);

Node* getLeft(Node* branchNode);
Node* getRight(Node* branchNode);
void setLeft(Node* branchNode, Node* left);
void setRight(Node* branchNode, Node* right);

long long getType(Node* leafNode);
void setType(Node* leafNode, long long type);
long long getValue(Node* leafNode);
void setValue(Node* leafNode, long long value);

typedef struct Hold Hold;
Hold* hold(Node* node);
void release(Hold* node);
Hold* replaceHold(Hold* oldHold, Hold* newHold);
Node* getNode(Hold* hold);

void negateLocations(Node* node);
Node* getListElement(Node* node, unsigned long long n);

#endif
