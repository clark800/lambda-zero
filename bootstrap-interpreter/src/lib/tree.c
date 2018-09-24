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
    char isLeaf, type;
    Tag tag;
    Branch left, right;
};

Node VOID_NODE = {.referenceCount=1, .isLeaf=1, .type=0, .tag={.lexeme={
    .start="VOID", .length=4}, .location={.file=NULL, .line=0, .column=0}},
    .left={.value=0}, .right={.value=0}};
Node *const VOID = &VOID_NODE;

void initNodeAllocator() {initPool(sizeof(Node), 4096);}
void destroyNodeAllocator() {destroyPool();}
bool isLeaf(Node* node) {return node->isLeaf != 0;}
Tag getTag(Node* node) {return node->tag;}
void setTag(Node* node, Tag tag) {node->tag = tag;}
char getType(Node* node) {return node->type;}
Node* getLeft(Node* node) {return node->left.child;}
Node* getRight(Node* node) {return node->right.child;}
long long getValue(Node* node) {return node->left.value;}
void setValue(Node* node, long long value) {node->left.value = value;}
void* getData(Node* node) {return (void*)node->right.child;}
Node* copyNode(Node* node, Node* source) {return *node = *source, node;}
Node* reference(Node* node) {return node->referenceCount += 1, node;}

Node* newNode(Tag tag, char isLeaf, char type, Node* left, Node* right) {
    return copyNode((Node*)allocate(), &(Node)
        {.referenceCount=0, .isLeaf=isLeaf, .type=type, .tag=tag,
        .left={.child=left}, .right={.child=right}});
}

Node* newBranch(Tag tag, char type, Node* left, Node* right) {
    return newNode(tag, 0, type, reference(left), reference(right));
}

Node* newPair(Node* left, Node* right) {
    Tag tag = newTag(newString(NULL, 0), newLocation(NULL, 0, 0));
    return newBranch(tag, 0, left, right);
}

Node* newLeaf(Tag tag, char type, long long value, void* data) {
    Node* node = newNode(tag, 1, type, NULL, (Node*)data);
    setValue(node, value);
    return node;
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

void setLeft(Node* node, Node* left) {
    assert(!isLeaf(node) && left != NULL);
    Node* oldLeft = node->left.child;
    node->left.child = reference(left);
    releaseNode(oldLeft);
}

void setRight(Node* node, Node* right) {
    assert(!isLeaf(node) && right != NULL);
    Node* oldRight = node->right.child;
    node->right.child = reference(right);
    releaseNode(oldRight);
}

Hold* hold(Node* node) {return (Hold*)reference(node);}
void release(Hold* nodeHold) {releaseNode((Node*)nodeHold);}
Node* getNode(Hold* nodeHold) {return (Node*)nodeHold;}

Node* getListElement(Node* node, unsigned long long n) {
    assert(!isLeaf(node));
    for (unsigned long long i = 0; i < n; ++i) {
        node = node->right.child;
        assert(!isLeaf(node));
    }
    return node->left.child;
}
