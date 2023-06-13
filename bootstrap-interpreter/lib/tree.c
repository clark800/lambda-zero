#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>  // exit
#include <string.h>
#include "freelist.h"
#include "tree.h"

typedef enum {GC_NONE=0, GC_LEFT=1, GC_RIGHT=2, GC_BOTH=3} Flags;

struct Node {
    unsigned int referenceCount;
    char flags, type, variety;
    Tag tag;
    union {
        struct {Node *left, *right;} branches;
        void* pointer;
        long long value;
        Lexeme lexeme;
    } data;
};

Node VOID_TAG = {.referenceCount=1, .flags=GC_NONE, .tag=NULL,
    .data={.lexeme={.start="VOID", .length=4}}};
Node VOID_NODE = {.referenceCount=1, .flags=GC_NONE, .tag=(Tag)(&VOID_TAG)};
Node *const VOID = &VOID_NODE;

void initNodeAllocator() {initPool(sizeof(Node), 4096);}
void destroyNodeAllocator() {destroyPool();}
Tag getTag(Node* node) {return node->tag;}
char getType(Node* node) {return node->type;}
void setType(Node* node, char type) {node->type = type;}
char getVariety(Node* node) {return node->variety;}
void setVariety(Node* node, char variety) {node->variety = variety;}
Node* getLeft(Node* node) {return node->data.branches.left;}
Node* getRight(Node* node) {return node->data.branches.right;}
long long getValue(Node* node) {return node->data.value;}
void setValue(Node* node, long long value) {node->data.value = value;}
void* getData(Node* node) {return node->data.pointer;}
static Node* copyNode(Node* node, Node* source) {return *node = *source, node;}
static Node* reference(Node* node) {return node->referenceCount += 1, node;}

Node* newBranch(Tag tag, char type, char variety, Node* left, Node* right) {
    return copyNode((Node*)allocate(), &(Node)
        {.referenceCount=0, .flags=GC_BOTH, .type=type, .variety=variety,
        .tag=(tag == NULL ? NULL : (Tag)reference((Node*)tag)),
        .data={.branches={.left=reference(left), .right=reference(right)}}});
}

Node* newPair(Node* left, Node* right) {
    return newBranch(NULL, -1, 0, left, right);
}

Node* newLeaf(Tag tag, char type, char variety, void* data) {
    return copyNode((Node*)allocate(), &(Node)
        {.referenceCount=0, .flags=GC_NONE, .type=type, .variety=variety,
        .tag=(Tag)reference((Node*)tag), .data={.pointer=data}});
}

static void releaseNode(Node* node) {
    assert(node->referenceCount > 0);
    node->referenceCount -= 1;
    if (node->referenceCount == 0) {
        if (node->flags & GC_LEFT)
            releaseNode(node->data.branches.left);
        if (node->flags & GC_RIGHT)
            releaseNode(node->data.branches.right);
        if (node->tag != NULL)
            releaseNode((Node*)(node->tag));
        reclaim(node);
    }
}

void setLeft(Node* node, Node* left) {
    assert((node->flags & GC_LEFT) && left != NULL);
    Node* oldLeft = node->data.branches.left;
    node->data.branches.left = reference(left);
    releaseNode(oldLeft);
}

void setTag(Node* node, Tag tag) {
    Tag oldTag = node->tag;
    node->tag = (Tag)reference((Node*)tag);
    releaseNode((Node*)oldTag);
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

Tag newTag(Lexeme lexeme, char fixity) {
    return (Tag)copyNode((Node*)allocate(), &(Node)
        {.referenceCount=0, .flags=GC_NONE, .type=fixity, .variety=0,
        .tag=NULL, .data={.lexeme=lexeme}});
}

Tag newLiteralTag(const char* name, Location location, char fixity) {
    Lexeme lexeme = newLexeme(name, (unsigned short)strlen(name), location);
    return newTag(lexeme, fixity);
}

Tag addPrefix(Tag tag, char prefix) {
    Node* node = copyNode((Node*)allocate(), (Node*)tag);
    node->referenceCount = 0;
    node->variety = prefix;
    return (Tag)node;
}

Lexeme getLexeme(Tag tag) {
    return ((Node*)tag)->data.lexeme;
}

char getTagFixity(Tag tag) {
    return ((Node*)tag)->type;
}

bool isThisTag(Tag a, const char* b) {
    return ((Node*)a)->variety == '\0' &&
        isThisLexeme(((Node*)a)->data.lexeme, b);
}

bool isSameTag(Tag a, Tag b) {
    return ((Node*)a)->variety == ((Node*)b)->variety &&
        isSameLexeme(((Node*)a)->data.lexeme, ((Node*)b)->data.lexeme);
}

void printTag(Tag tag, FILE* stream) {
    Lexeme lexeme = ((Node*)tag)->data.lexeme;
    if (((Node*)tag)->variety == '\0' && lexeme.length > 0 &&
            lexeme.start[0] == '\n') {
        fputs("(end of line)", stream);
    } else {
        if (((Node*)tag)->variety != '\0')
            fputc(((Node*)tag)->variety, stream);
        fwrite(lexeme.start, sizeof(char), lexeme.length, stream);
    }
}

void printTagWithLocation(Tag tag, FILE* stream) {
    fputs("'", stream);
    printTag(tag, stream);
    fputs("' at ", stream);
    printLocation(((Node*)tag)->data.lexeme.location, stream);
}

void syntaxError(const char* message, Tag tag) {
    fputs("Syntax error: ", stderr);
    fputs(message, stderr);
    fputs(" ", stderr);
    printTagWithLocation(tag, stderr);
    fputs("\n", stderr);
    exit(1);
}

void syntaxErrorIf(bool condition, const char* message, Tag tag) {
    if (condition)
        syntaxError(message, tag);
}
