#include <stdio.h>  // fputs
#include <stdlib.h> // exit
#include "tree.h"

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
