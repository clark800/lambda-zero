#include <assert.h>
#include "lib/lltoa.h"
#include "lib/tree.h"
#include "ast.h"
#include "lex.h"
#include "closure.h"
#include "serialize.h"

bool DEBUG = false;
bool PROFILE = false;

void debug(const char* message) {
    fputs(message, stderr);
}

void debugLine(void) {
    debug("======================================");
    debug("======================================\n");
}

void serializeInteger(long long n, FILE* stream) {
    char buffer[3 * sizeof(long long)];
    fputs(lltoa(n, buffer, 10), stream);
}

void debugLoopCount(int loopCount) {
    if (PROFILE) {
        debug("Loops: ");
        serializeInteger(loopCount, stderr);
        debug("\n");
    }
}

void serializeAST(Node* node, FILE* stream) {
    if (node == NULL) {
        fputs("NULL", stream);     // for debugging
    } else if (isBranchNode(node)) {
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
        serializeInteger(getInteger(node), stream);
    } else if (isNewline(node)) {
        fputs("\\n", stream);
    } else {
        printToken(node, stream);
    }
}

void debugAST(const char* label, Node* node) {
    if (DEBUG) {
        debugLine();
        debug(label);
        debug(": ");
        serializeAST(node, stderr);
        debug("\n");
    }
}

void serializeParseStack(Stack* stack, FILE* stream) {
    fputs("[", stream);
    for (Iterator* it = iterate(stack); !end(it); it = next(it)) {
        serializeAST(cursor(it), stream);
        fputs(", ", stream);
    }
    fputs("]", stream);
}

void debugParseState(Node* token, Stack* stack) {
    if (DEBUG) {
        debug("Token: ");
        serializeAST(token, stderr);
        debug("  Stack: ");
        serializeParseStack(stack, stderr);
        debug("\n");
    }
}

void serializeClosureStack(Node* head, FILE* stream) {
    fputs("[", stream);
    Stack* stack = newStack(head);
    for (Iterator* it = iterate(stack); !end(it); it = next(it)) {
        serializeAST(getClosureTerm(cursor(it)), stream);
        fputs(", ", stream);
    }
    fputs("]", stream);
    deleteStack(stack);
}

void serializeEvalState(Node* node, Stack* stack, Stack* env, FILE* stream) {
    fputs("node: ", stream);
    serializeAST(node, stream);
    fputs("\nstack: ", stream);
    serializeClosureStack(getHead(stack), stream);
    fputs("\nenv: ", stream);
    serializeClosureStack(getHead(env), stream);
    fputs("\n", stream);
}

void debugEvalState(Node* node, Stack* stack, Stack* env) {
    if (DEBUG) {
        debugLine();
        serializeEvalState(node, stack, env, stderr);
    }
}

void serializeNode(Node* node, Stack* env, unsigned int depth, FILE* stream) {
    if (isApplication(node)) {
        fputs("(", stream);
        serializeNode(getLeft(node), env, depth, stream);
        fputs(" ", stream);
        serializeNode(getRight(node), env, depth, stream);
        fputs(")", stream);
    } else if (isAbstraction(node)) {
        fputs("(", stream);
        printToken(getParameter(node), stream);
        fputs(" -> ", stream);
        serializeNode(getBody(node), env, depth + 1, stream);
        fputs(")", stream);
    } else if (isReference(node)) {
        unsigned long long debruijn = getDebruijnIndex(node);
        if (debruijn > depth) {
            Node* closure = peek(env, debruijn - depth - 1);
            Node* nextNode = getLeft(closure);
            Stack* nextEnv = newStack(getRight(closure));
            serializeNode(nextNode, nextEnv, 0, stream);
            deleteStack(nextEnv);
        } else {
            printToken(node, stream);
        }
    } else if (isInteger(node)) {
        serializeInteger(getInteger(node), stream);
    } else if (isBuiltin(node)) {
        printToken(node, stream);
    } else {
        assert(false);
    }
}

void serializeClosure(Node* closure, FILE* stream) {
    Node* root = getClosureTerm(closure);
    Stack* env = newStack(getClosureEnv(closure));
    serializeNode(root, env, 0, stream);
    deleteStack(env);
}
