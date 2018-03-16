#include <assert.h>
#include "freelist.h"
#include "tree.h"

typedef union {
    Node* child;
    long long value;
} Branch;

struct Node {
    unsigned int referenceCount;
    int location;
    Branch left;
    Branch right;
};

static Node FAKENULL_NODE = {0, 0, {NULL}, {NULL}};
static Node *const FAKENULL = &FAKENULL_NODE;

void initNodeAllocator() {
    initPool(sizeof(Node), 32768);
}

void destroyNodeAllocator() {
    destroyPool();
}

Node* newBranchNode(int location, Node* left, Node* right) {
    Node* node = (Node*)allocate();
    node->location = location;
    node->referenceCount = 0;
    node->left.child = left == NULL ? FAKENULL : left;
    node->right.child = right == NULL ? FAKENULL : right;
    node->left.child->referenceCount += 1;
    node->right.child->referenceCount += 1;
    return node;
}

Node* newLeafNode(int location, long long type) {
    Node* node = (Node*)allocate();
    node->location = location;
    node->referenceCount = 0;
    node->left.child = (Node*)type;
    node->right.child = NULL;
    return node;
}

bool isLeafNode(Node* node) {
    assert(node != NULL);
    return node->left.child == NULL || node->right.child == NULL;
}

bool isBranchNode(Node* node) {
    return !isLeafNode(node);
}

int getLocation(Node* node) {
    return node->location;
}

void releaseNode(Node* node) {
    assert(node->referenceCount > 0);
    node->referenceCount -= 1;
    if (node->referenceCount == 0 && node != FAKENULL) {
        if (!isLeafNode(node)) {
            releaseNode(node->left.child);
            releaseNode(node->right.child);
        }
        reclaim(node);
    }
}

Node* getLeft(Node* node) {
    assert(isBranchNode(node));
    return node->left.child == FAKENULL ? NULL : node->left.child;
}

Node* getRight(Node* node) {
    assert(isBranchNode(node));
    return node->right.child == FAKENULL ? NULL : node->right.child;
}

void setLeft(Node* node, Node* left) {
    assert(isBranchNode(node));
    Node* oldLeft = node->left.child;
    node->left.child = left == NULL ? FAKENULL : left;
    node->left.child->referenceCount += 1;
    releaseNode(oldLeft);
}

void setRight(Node* node, Node* right) {
    assert(isBranchNode(node));
    Node* oldRight = node->right.child;
    node->right.child = right == NULL ? FAKENULL : right;
    node->right.child->referenceCount += 1;
    releaseNode(oldRight);
}

long long getType(Node* node) {
    assert(isLeafNode(node));
    return node->left.value;
}

void setType(Node* node, long long type) {
    assert(node->right.child == NULL);
    node->left.value = type;
}

long long getValue(Node* node) {
    assert(node->left.child == NULL);
    return node->right.value;
}

void setValue(Node* node, long long value) {
    assert(node->left.child == NULL);
    node->right.value = value;
}

Hold* hold(Node* node) {
    node->referenceCount += 1;
    return (Hold*)node;
}

void release(Hold* nodeHold) {
    releaseNode((Node*)nodeHold);
}

Hold* replaceHold(Hold* oldHold, Hold* newHold) {
    release(oldHold); // even if newHold == oldHold, we still must release one
    return newHold;
}

Node* getNode(Hold* nodeHold) {
    return (Node*)nodeHold;
}

void negateLocations(Node* node) {
    node->location = -node->location;
    if (isBranchNode(node)) {
        negateLocations(node->left.child);
        negateLocations(node->right.child);
    }
}
