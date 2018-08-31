#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "tag.h"

typedef struct Node Node;
extern Node* const VOID;

void initNodeAllocator(void);
void destroyNodeAllocator(void);

Node* newBranch(Tag tag, unsigned char type, Node* left, Node* right);
Node* newPair(Node* left, Node* right);
Node* newLeaf(Tag tag, unsigned char type, long long value);

bool isLeaf(Node* node);
Location getLocation(Node* node);
Node* setLocation(Node* node, Location location);
Tag getTag(Node* node);

Node* getLeft(Node* branchNode);
Node* getRight(Node* branchNode);
void setLeft(Node* branchNode, Node* left);
void setRight(Node* branchNode, Node* right);

unsigned char getType(Node* node);
long long getValue(Node* leafNode);
Node* setValue(Node* leafNode, long long value);

typedef struct Hold Hold;
Hold* hold(Node* node);
void release(Hold* node);
Hold* replaceHold(Hold* oldHold, Hold* newHold);
Node* getNode(Hold* hold);

Node* getListElement(Node* node, unsigned long long n);
