#include <stdio.h>
#include "lib/tree.h"
#include "lib/array.h"
#include "ast.h"
#include "print.h"
#include "closure.h"

static void serializeNode(Node* node, Node* locals, const Array* globals,
        unsigned int depth, FILE* stream) {
    switch (getNodeType(node)) {
        case APPLICATION:
            fputs("(", stream);
            serializeNode(getLeft(node), locals, globals, depth, stream);
            fputs(" ", stream);
            serializeNode(getRight(node), locals, globals, depth, stream);
            fputs(")", stream);
            break;
        case LAMBDA:
            fputs("(", stream);
            printLexeme(getLexeme(getParameter(node)), stream);
            fputs(" -> ", stream);
            serializeNode(getBody(node), locals, globals, depth + 1, stream);
            fputs(")", stream);
            break;
        case SYMBOL:
            if (isGlobalReference(node)) {
                Node* value = elementAt(globals, getGlobalIndex(node));
                serializeNode(value, locals, globals, 0, stream);
                return;
            }
            unsigned long long debruijn = getDebruijnIndex(node);
            if (debruijn >= depth) {
                Closure* next = getListElement(locals, debruijn - depth);
                serializeNode(getTerm(next), getLocals(next), globals, 0,
                    stream);
            } else {
                printLexeme(getLexeme(node), stream);
            }
            break;
        case NATURAL: fputll(getValue(node), stream); break;
        case BUILTIN: printLexeme(getLexeme(node), stream); break;
    }
}

void serialize(Closure* closure, const Array* globals) {
    serializeNode(getTerm(closure), getLocals(closure), globals, 0, stdout);
    fputs("\n", stdout);
}
