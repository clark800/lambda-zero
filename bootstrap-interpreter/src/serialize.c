#include <stdio.h>
#include "lib/tree.h"
#include "lib/array.h"
#include "ast.h"
#include "print.h"
#include "closure.h"
#include "serialize.h"

static void serializeNode(Node* node, Node* locals, const Array* globals,
        unsigned int depth, FILE* stream) {
    if (isApplication(node)) {
        fputs("(", stream);
        serializeNode(getLeft(node), locals, globals, depth, stream);
        fputs(" ", stream);
        serializeNode(getRight(node), locals, globals, depth, stream);
        fputs(")", stream);
    } else if (isLambda(node)) {
        fputs("(", stream);
        printToken(getParameter(node), stream);
        fputs(" -> ", stream);
        serializeNode(getBody(node), locals, globals, depth + 1, stream);
        fputs(")", stream);
    } else if (isReference(node)) {
        long long debruijn = getValue(node);
        if (debruijn > depth) {
            Closure* next = getListElement(locals, debruijn - depth - 1);
            serializeNode(getTerm(next), getLocals(next), globals, 0, stream);
        } else {
            printToken(node, stream);
        }
    } else if (isInteger(node)) {
        fputll(getValue(node), stream);
    } else if (isBuiltin(node)) {
        printToken(node, stream);
    } else if (isGlobal(node)) {
        Node* value = elementAt(globals, (size_t)getValue(node));
        serializeNode(value, locals, globals, 0, stream);
    } else {
        assert(false);
    }
}

void serialize(Closure* closure, const Array* globals) {
    serializeNode(getTerm(closure), getLocals(closure), globals, 0, stdout);
    fputs("\n", stdout);
}
