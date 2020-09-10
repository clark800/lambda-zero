#include "tree.h"

void syntaxError(const char* message, Node* node) {
    throwError(message, getTag(node));
}

void syntaxErrorIf(bool condition, const char* message, Node* node) {
    if (condition)
        syntaxError(message, node);
}
