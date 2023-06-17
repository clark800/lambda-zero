#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "lexeme.h"

typedef struct Node Node;
typedef struct Tag* Tag;

void initNodeAllocator(void);
void destroyNodeAllocator(void);

Node* newBranch(Tag tag, char type, char variety, Node* left, Node* right);
Node* newPair(Node* left, Node* right);
Node* newLeaf(Tag tag, char type, char variety, void* data);

Tag getTag(Node* node);
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

Tag newTag(Lexeme lexeme, char fixity);
Tag newLiteralTag(const char* name, Location location, char fixity);
Tag addPrefix(Tag tag, char prefix);
Lexeme getLexeme(Tag tag);
char getTagFixity(Tag tag);
bool isThisTag(Tag a, const char* b);
bool isSameTag(Tag a, Tag b);
void printTag(Tag tag, FILE* stream);
void printTagWithLocation(Tag tag, FILE* stream);
void syntaxError(const char* message, Tag tag);
void syntaxErrorIf(bool condition, const char* message, Tag tag);
