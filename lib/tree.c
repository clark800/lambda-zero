#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include "freelist.h"
#include "tree.h"

typedef union {
    Node* child;
    long long value;
} Branch;

struct Node {
    unsigned int referenceCount;
    String label;
    Branch left;
    Branch right;
};

Node VOID_NODE = {1, {"VOID", 4}, {NULL}, {NULL}};    // same as integer "0"
Node *const VOID = &VOID_NODE;

void initNodeAllocator() {
    initPool(sizeof(Node), 4096);
}

void destroyNodeAllocator() {
    destroyPool();
}

Node* newBranch(String label, Node* left, Node* right) {
    assert(left != NULL && right != NULL);
    Node* node = (Node*)allocate();
    node->label = label;
    node->referenceCount = 0;
    node->left.child = left;
    node->right.child = right;
    node->left.child->referenceCount += 1;
    node->right.child->referenceCount += 1;
    return node;
}

Node* newLeaf(String label, long long type) {
    Node* node = (Node*)allocate();
    node->label = label;
    node->referenceCount = 0;
    node->left.child = (Node*)type;
    node->right.child = NULL;
    return node;
}

bool isLeaf(Node* node) {
    return node->left.child == NULL || node->right.child == NULL;
}

bool isBranch(Node* node) {
    return node->left.child != NULL && node->right.child != NULL;
}

void releaseNode(Node* node) {
    assert(node->referenceCount > 0);
    node->referenceCount -= 1;
    if (node->referenceCount == 0) {
        if (!isLeaf(node)) {
            releaseNode(node->left.child);
            releaseNode(node->right.child);
        }
        assert(node != VOID);
        reclaim(node);
    }
}

Node* getLeft(Node* node) {
    assert(isBranch(node));
    return node->left.child;
}

Node* getRight(Node* node) {
    assert(isBranch(node));
    return node->right.child;
}

String getLabel(Node* node) {
    return node->label;
}

void setLeft(Node* node, Node* left) {
    assert(isBranch(node) && left != NULL);
    Node* oldLeft = node->left.child;
    node->left.child = left;
    node->left.child->referenceCount += 1;
    releaseNode(oldLeft);
}

void setRight(Node* node, Node* right) {
    assert(isBranch(node) && right != NULL);
    Node* oldRight = node->right.child;
    node->right.child = right;
    node->right.child->referenceCount += 1;
    releaseNode(oldRight);
}

long long getType(Node* node) {
    assert(isLeaf(node) && node != VOID);
    return node->left.value;
}

void setType(Node* node, long long type) {
    assert(isLeaf(node) && node != VOID && node->right.child == NULL);
    node->left.value = type;
}

long long getValue(Node* node) {
    assert(isLeaf(node) && node != VOID && node->left.child == NULL);
    return node->right.value;
}

void setValue(Node* node, long long value) {
    assert(isLeaf(node) && node != VOID && node->left.child == NULL);
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

Node* getListElement(Node* node, unsigned long long n) {
    assert(isBranch(node));
    for (unsigned long long i = 0; i < n; i++) {
        node = node->right.child;
        assert(isBranch(node));
    }
    return node->left.child;
}
