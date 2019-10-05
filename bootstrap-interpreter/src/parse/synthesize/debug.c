#include <stdio.h>
#include "shared/lib/tree.h"
#include "shared/lib/util.h"
#include "parse/shared/ast.h"

static void serializeAST(Node* node, FILE* stream) {
    if (!isLeaf(node)) {
        fputs("(", stream);
        serializeAST(getLeft(node), stream);
        fputs(isArrow(node) ? " -> " : " ", stream);
        serializeAST(getRight(node), stream);
        fputs(")", stream);
    } else if (isNumber(node)) {
        // numbers can be generated, so not all numbers will exist in input
        fputll(getValue(node), stream);
    } else {
        printString(getLexeme(node), stream);
        fputs("#", stream);
        fputll(getValue(node), stream);
    }
}

void debugAST(Node* node) {
    serializeAST(node, stderr);
}
