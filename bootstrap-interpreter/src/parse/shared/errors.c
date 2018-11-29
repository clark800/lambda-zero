#include <stdio.h>
#include <stdlib.h>
#include "shared/lib/tree.h"
#include "shared/lib/util.h"

extern bool TEST;

void tokenErrorIf(bool condition, const char* message, Tag tag) {
    if (condition) {
        fputs("Syntax error: ", stderr);
        fputs(message, stderr);
        fputs(" ", stderr);
        printTag(tag, "\'", stderr);
        fputs("\n", stderr);
        exit(1);
    }
}

void syntaxErrorIf(bool condition, const char* message, Node* node) {
    tokenErrorIf(condition, message, getTag(node));
}

void syntaxError(const char* message, Node* node) {
    syntaxErrorIf(true, message, node);
}
