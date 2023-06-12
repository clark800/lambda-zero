#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include "freelist.h"
#include "tree.h"

typedef enum {GC_NONE=0, GC_LEFT=1, GC_RIGHT=2, GC_BOTH=3} Flags;

struct Node {
    unsigned int referenceCount;
    char flags, type, variety;
    Tag tag;
    union {
        struct {Node *left, *right;} branches;
        long long value;
        Lexeme lexeme;
    } data;
};

Node VOID_NODE = {.referenceCount=1, .tag={.lexeme={.start="VOID", .length=4}}};
Node *const VOID = &VOID_NODE;

void initNodeAllocator() {initPool(sizeof(Node), 4096);}
void destroyNodeAllocator() {destroyPool();}
Tag getTag(Node* node) {return node->tag;}
void setTag(Node* node, Tag tag) {node->tag = tag;}
char getType(Node* node) {return node->type;}
void setType(Node* node, char type) {node->type = type;}
char getVariety(Node* node) {return node->variety;}
void setVariety(Node* node, char variety) {node->variety = variety;}
Node* getLeft(Node* node) {return node->data.branches.left;}
Node* getRight(Node* node) {return node->data.branches.right;}
long long getValue(Node* node) {return node->data.value;}
void setValue(Node* node, long long value) {node->data.value = value;}
void* getData(Node* node) {return (void*)node->data.branches.left;}
static Node* copyNode(Node* node, Node* source) {return *node = *source, node;}
static Node* reference(Node* node) {return node->referenceCount += 1, node;}

static Node* newNode(Tag tag, char flags, char type, char variety,
        Node* left, Node* right) {
    return copyNode((Node*)allocate(), &(Node)
        {.referenceCount=0, .flags=flags, .type=type, .variety=variety,
        .tag=tag, .data={.branches={.left=left, .right=right}}});
}

Node* newBranch(Tag tag, char type, char variety, Node* left, Node* right) {
    return newNode(tag, GC_BOTH, type, variety,
        reference(left), reference(right));
}

Node* newPair(Node* left, Node* right) {
    Tag tag = newTag(newLexeme(NULL, 0, newLocation(0, 0, 0)), 0);
    return newBranch(tag, -1, 0, left, right);
}

Node* newLeaf(Tag tag, char type, char variety, void* data) {
    return newNode(tag, GC_NONE, type, variety, (Node*)data, NULL);
}

static void releaseNode(Node* node) {
    assert(node->referenceCount > 0);
    node->referenceCount -= 1;
    if (node->referenceCount == 0) {
        if (node->flags & GC_LEFT)
            releaseNode(node->data.branches.left);
        if (node->flags & GC_RIGHT)
            releaseNode(node->data.branches.right);
        reclaim(node);
    }
}

void setLeft(Node* node, Node* left) {
    assert((node->flags & GC_LEFT) && left != NULL);
    Node* oldLeft = node->data.branches.left;
    node->data.branches.left = reference(left);
    releaseNode(oldLeft);
}

void setRight(Node* node, Node* right) {
    assert((node->flags & GC_RIGHT) && right != NULL);
    Node* oldRight = node->data.branches.right;
    node->data.branches.right = reference(right);
    releaseNode(oldRight);
}

Hold* hold(Node* node) {return (Hold*)reference(node);}
void release(Hold* nodeHold) {releaseNode((Node*)nodeHold);}
Node* getNode(Hold* nodeHold) {return (Node*)nodeHold;}

Node* getListElement(Node* node, unsigned long long n) {
    assert((node->flags & GC_BOTH) == GC_BOTH);
    for (unsigned long long i = 0; i < n; ++i) {
        node = node->data.branches.right;
        assert((node->flags & GC_BOTH) == GC_BOTH);
    }
    return node->data.branches.left;
}
