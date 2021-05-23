#include "tree.h"
#include "errors.h"

void syntaxErrorNode(const char* message, Node* node) {
    syntaxError(message, getTag(node));
}

void syntaxErrorNodeIf(bool condition, const char* message, Node* node) {
    if (condition)
        syntaxErrorNode(message, node);
}
