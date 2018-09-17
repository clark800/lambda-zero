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
    unsigned int referenceCount, length;
    unsigned char isLeaf, type;
    Location location;
    const char* lexeme;
    Branch left, right;
};

Node VOID_NODE = {.referenceCount=1, .length=4, .isLeaf=1, .type=0,
    .location={0, 0}, .lexeme="VOID", .left={.value=0}, .right={.value=0}};
Node *const VOID = &VOID_NODE;

void initNodeAllocator() {
    initPool(sizeof(Node), 4096);
}

void destroyNodeAllocator() {
    destroyPool();
}

Location getLocation(Node* node) {
    return node->location;
}

Node* setTag(Node* node, Tag tag) {
    node->lexeme = tag.lexeme.start;
    node->length = tag.lexeme.length;
    node->location = tag.location;
    return node;
}

Node* newBranch(Tag tag, unsigned char type, Node* left, Node* right) {
    assert(left != NULL && right != NULL);
    Node* node = (Node*)allocate();
    node->referenceCount = 0;
    node->isLeaf = 0;
    node->type = type;
    node->length = tag.lexeme.length;
    node->location = tag.location;
    node->lexeme = tag.lexeme.start;
    node->left.child = left;
    node->right.child = right;
    node->left.child->referenceCount += 1;
    node->right.child->referenceCount += 1;
    return node;
}

Node* newPair(Node* left, Node* right) {
    return newBranch(newTag(EMPTY, newLocation(0, 0)), 0, left, right);
}

Node* newLeaf(Tag tag, unsigned char type, long long value, void* data) {
    Node* node = (Node*)allocate();
    node->referenceCount = 0;
    node->isLeaf = 1;
    node->type = type;
    node->length = tag.lexeme.length;
    node->location = tag.location;
    node->lexeme = tag.lexeme.start;
    node->left.value = value;
    node->right.child = (Node*)data;
    return node;
}

bool isLeaf(Node* node) {
    return node->isLeaf != 0;
}

void releaseNode(Node* node) {
    assert(node->referenceCount > 0);
    node->referenceCount -= 1;
    if (node->referenceCount == 0) {
        if (!isLeaf(node)) {
            releaseNode(node->left.child);
            releaseNode(node->right.child);
        }
        reclaim(node);
    }
}

Node* getLeft(Node* node) {
    assert(!isLeaf(node));
    return node->left.child;
}

Node* getRight(Node* node) {
    assert(!isLeaf(node));
    return node->right.child;
}

Tag getTag(Node* node) {
    return newTag(newString(node->lexeme, node->length), node->location);
}

void setLeft(Node* node, Node* left) {
    assert(!isLeaf(node) && left != NULL);
    Node* oldLeft = node->left.child;
    node->left.child = left;
    node->left.child->referenceCount += 1;
    releaseNode(oldLeft);
}

void setRight(Node* node, Node* right) {
    assert(!isLeaf(node) && right != NULL);
    Node* oldRight = node->right.child;
    node->right.child = right;
    node->right.child->referenceCount += 1;
    releaseNode(oldRight);
}

unsigned char getType(Node* node) {
    return node->type;
}

long long getValue(Node* node) {
    assert(isLeaf(node));
    return node->left.value;
}

Node* setValue(Node* node, long long value) {
    assert(isLeaf(node));
    node->left.value = value;
    return node;
}

void* getData(Node* node) {
    assert(isLeaf(node));
    return (void*)node->right.child;
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
    assert(!isLeaf(node));
    for (unsigned long long i = 0; i < n; ++i) {
        node = node->right.child;
        assert(!isLeaf(node));
    }
    return node->left.child;
}
