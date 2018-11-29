#include <stdio.h>
#include "shared/lib/tree.h"
#include "shared/lib/stack.h"
#include "shared/lib/util.h"
#include "parse/shared/ast.h"

void debug(const char* message) {
    fputs(message, stderr);
}

static void debugLine(void) {
    debug("======================================");
    debug("======================================\n");
}

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

static void debugStack(Stack* stack, Node* (*select)(Node*)) {
    Stack* reversed = newStack();
    for (Iterator* it = iterate(stack); !end(it); it = next(it))
        push(reversed, cursor(it));
    for (Iterator* it = iterate(reversed); !end(it); it = next(it)) {
        debugAST(select == NULL ? cursor(it) : select(cursor(it)));
        debug(" | ");
    }
    deleteStack(reversed);
}

/*
#include "closure.h"
void debugState(Closure* closure, Stack* stack) {
    debugLine();
    debug("term: ");
    debugAST(getTerm(closure));
    debug("\nstack: ");
    debugStack(stack, (Node* (*)(Node*))getTerm);
    debug("\nlocals: ");
    debugStack((Stack*)closure, (Node* (*)(Node*))getTerm);
    debug("\n");
}
*/

void debugParseState(Tag tag, Stack* stack, bool trace) {
    if (trace) {
        debug("Token: '");
        printString(tag.lexeme, stderr);
        debug("'  Stack: ");
        debugStack(stack, NULL);
        debug("\n");
    }
}

void debugParseStage(const char* label, Node* node, bool trace) {
    if (trace) {
        debugLine();
        debug(label);
        debug(": ");
        debugAST(node);
        debug("\n");
    }
}
