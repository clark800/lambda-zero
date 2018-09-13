#include <stdio.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "print.h"

void debug(const char* message) {
    fputs(message, stderr);
}

static void debugLexeme(String lexeme) {
    printLexeme(lexeme, stderr);
}

static void debugLine(void) {
    debug("======================================");
    debug("======================================\n");
}

static void serializeAST(Node* node, FILE* stream) {
    if (node == NULL) {
        fputs("NULL", stream);     // for debugging
    } else if (node == VOID) {
        fputs("VOID", stream);
    } else if (!isLeaf(node)) {
        fputs("(", stream);
        serializeAST(getLeft(node), stream);
        fputs(isLambda(node) ? " -> " : " ", stream);
        serializeAST(getRight(node), stream);
        fputs(")", stream);
    } else if (isInteger(node)) {
        // builtins create integers, so not all integers will exist in input
        fputll(getValue(node), stream);
    } else if (isReference(node)) {
        printToken(node, stream);
        fputs("#", stream);
        fputll(getValue(node), stream);
    } else {
        printToken(node, stream);
    }
}

void debugAST(Node* node) {
    serializeAST(node, stderr);
}

static void debugStack(Stack* stack, Node* (*select)(Node*)) {
    debug("[");
    for (Iterator* it = iterate(stack); !end(it); it = next(it)) {
        debugAST(select == NULL ? cursor(it) : select(cursor(it)));
        if (!end(next(it)))
            debug(", ");
    }
    debug("]");
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
        debugLexeme(tag.lexeme);
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
