#include <stdio.h>
#include "tree.h"
#include "stack.h"
#include "util.h"
#include "ast.h"
#include "debug.h"

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
        printTag(getTag(node), stream);
        fputs("#", stream);
        fputll(getValue(node), stream);
    }
}

static void debugAST(Node* node) {
    serializeAST(node, stderr);
}

static void debugStack(Stack* stack, void debugNode(Node*)) {
    Stack* reversed = newStack();
    for (Iterator* it = iterate(stack); !end(it); it = next(it))
        push(reversed, cursor(it));
    for (Iterator* it = iterate(reversed); !end(it); it = next(it)) {
        debugNode(cursor(it));
        fputs(" | ", stderr);
    }
    deleteStack(reversed);
}

void debugParseState(Tag tag, Stack* stack, bool trace) {
    if (trace) {
        fputs("Token: '", stderr);
        printTag(tag, stderr);
        fputs("'  Stack: ", stderr);
        debugStack(stack, debugAST);
        fputs("\n", stderr);
    }
}

void debugParseStage(const char* label, Node* node, bool trace) {
    if (trace) {
        fputs("======================================", stderr);
        fputs("======================================\n", stderr);
        fputs(label, stderr);
        fputs(": ", stderr);
        debugAST(node);
        fputs("\n", stderr);
    }
}
