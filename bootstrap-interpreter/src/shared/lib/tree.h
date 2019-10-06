#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "tag.h"

typedef struct Node Node;
extern Node* const VOID;

void initNodeAllocator(void);
void destroyNodeAllocator(void);

Node* newBranch(Tag tag, char type, char variety, Node* left, Node* right);
Node* newPair(Node* left, Node* right);
Node* newLeaf(Tag tag, char type, long long value, void* data);

bool isLeaf(Node* node);
Tag getTag(Node* node);
String getLexeme(Node* node);
void setTag(Node* node, Tag tag);
Node* getLeft(Node* branchNode);
Node* getRight(Node* branchNode);
void setLeft(Node* branchNode, Node* left);
void setRight(Node* branchNode, Node* right);
char getType(Node* node);
void setType(Node* node, char type);
char getVariety(Node* node);
void setVariety(Node* node, char variety);
long long getValue(Node* leafNode);
void setValue(Node* leafNode, long long value);
void* getData(Node* leafNode);

typedef struct Hold Hold;
Hold* hold(Node* node);
void release(Hold* node);
Node* getNode(Hold* hold);

Node* getListElement(Node* node, unsigned long long n);
