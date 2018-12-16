#include "shared/lib/tree.h"
#include "shared/lib/array.h"
#include "shared/term.h"
#include "parse/shared/errors.h"
#include "parse/shared/ast.h"

static unsigned long long findDebruijnIndex(Node* name, Array* parameters) {
    long long depth = 0;
    for (unsigned long long i = 1; i <= length(parameters); ++i)
        if (isSameLexeme(elementAt(parameters, length(parameters) - i), name))
            if (depth++ == getValue(name))
                return i;
    return 0;
}

static int findOperationCode(Node* name) {
    for (unsigned int i = 0; i < sizeof(Operations)/sizeof(char*); ++i)
        if (isThisName(name, Operations[i]))
            return (int)i;
    return -1;
}

static void bindName(Node* name, Array* parameters, size_t globalDepth) {
    if (isUnused(name))
       syntaxError("cannot reference a symbol starting with underscore", name);
    unsigned long long index = findDebruijnIndex(name, parameters);
    if (index > 0) {
        unsigned long long localDepth = length(parameters) - globalDepth;
        setValue(name, index <= localDepth ? (long long)index :
            (long long)(index - length(parameters) - 1));
        setType(name, VARIABLE);
        return;
    }
    int code = findOperationCode(name);
    if (code >= 0) {
        setValue(name, code);
        setType(name, OPERATION);
        return;
    }
    syntaxError("undefined symbol", name);
}

static void bindWith(Node* node, Array* parameters, const Array* globals) {
    switch (getASTType(node)) {
        case REFERENCE: setType(node, VARIABLE); break;
        case NAME: bindName(node, parameters, length(globals)); break;
        case ARROW:
            append(parameters, getParameter(node));
            bindWith(getBody(node), parameters, globals);
            unappend(parameters);
            setType(node, ABSTRACTION);
            break;
        case JUXTAPOSITION:
        case LET:
            bindWith(getLeft(node), parameters, globals);
            bindWith(getRight(node), parameters, globals);
            setType(node, APPLICATION);
            break;
        case NUMBER: setType(node, NUMERAL); break;
        case DEFINITION:
            syntaxError("missing scope for definition", node); break;
        case ASPATTERN:
            syntaxError("as pattern not in parameter position", node); break;
        case COMMAPAIR:
            syntaxError("comma not inside brackets", node); break;
        case SETBUILDER:
            syntaxError("must appear on the right side of '::='", node); break;
        case OPERATOR:
        case SECTION:
            assert(false); break;
    }
}

Array* bind(Hold* root) {
    Node* node = getNode(root);
    Array* parameters = newArray(2048);         // names of globals and locals
    Array* globals = newArray(2048);            // values of globals
    while (isLet(node) && !isUnderscore(getParameter(getLeft(node)))) {
        Node* definedName = getParameter(getLeft(node));
        Node* definedValue = getRight(node);
        bindWith(definedValue, parameters, globals);
        append(parameters, definedName);
        append(globals, definedValue);
        node = getBody(getLeft(node));
    }
    bindWith(node, parameters, globals);
    deleteArray(parameters);
    append(globals, node);
    return globals;
}
