#include <stdio.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "print.h"
#include "debug.h"

void debug(const char* message) {
    fputs(message, stderr);
}

void debugLine(void) {
    debug("======================================");
    debug("======================================\n");
}

void serializeAST(Node* node, FILE* stream) {
    if (node == NULL) {
        fputs("NULL", stream);     // for debugging
    } else if (node == VOID) {
        fputs("VOID", stream);
    } else if (isBranch(node)) {
        fputs("(", stream);
        serializeAST(getLeft(node), stream);
        fputs(isDefinition(node) ? " = " : " ", stream);
        serializeAST(getRight(node), stream);
        fputs(")", stream);
    } else if (isParameter(node)) {
        printToken(node, stream);
        fputs(" ->", stream);
    } else if (isInteger(node)) {
        // builtins create integers, so not all integers will exist in input
        fputll(getInteger(node), stream);
    } else if (isReference(node)) {
        printToken(node, stream);
        fputs("#", stream);
        fputll((long long)getDebruijnIndex(node), stream);
    } else {
        printToken(node, stream);
    }
}

void debugAST(Node* node) {
    serializeAST(node, stderr);
}

void debugStack(Stack* stack, Node* (*select)(Node*)) {
    debug("[");
    for (Iterator* it = iterate(stack); !end(it); it = next(it)) {
        debugAST(select == NULL ? cursor(it) : select(cursor(it)));
        if (!end(next(it)))
            debug(", ");
    }
    debug("]");
}
