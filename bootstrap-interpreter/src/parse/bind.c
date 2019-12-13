#include "tree.h"
#include "array.h"
#include "opp/errors.h"
#include "ast.h"
#include "term.h"

static unsigned long long findDebruijnIndex(Node* name, Array* parameters) {
    if (isUnused(name))
       syntaxError("cannot reference a symbol starting with underscore", name);
    for (size_t i = 1; i <= length(parameters); ++i) {
        Node* parameter = elementAt(parameters, length(parameters) - i);
        if (isSameLexeme(parameter, name)) {
            if (getData(parameter) == NULL)
                return (unsigned long long)i;
            syntaxError("cannot reference", name);
        }
    }
    return 0;
}

static int findOperationCode(Node* name) {
    for (unsigned int i = 0; i < sizeof(Operations)/sizeof(char*); ++i)
        if (isThisName(name, Operations[i]))
            return (int)i;
    return -1;
}

static void bindReference(Node* node, Array* parameters, size_t globalDepth) {
    unsigned long long i = (unsigned long long)getValue(node);
    unsigned long long index = i > 0 ? i : findDebruijnIndex(node, parameters);
    if (index > 0) {
        unsigned long long localDepth = length(parameters) - globalDepth;
        setValue(node, index <= localDepth ? (long long)index :
            (long long)(index - length(parameters) - 1));
        setType(node, VARIABLE);
        return;
    }
    int code = findOperationCode(node);
    if (code >= 0) {
        setValue(node, code);
        setType(node, OPERATION);
        return;
    }
    syntaxError("undefined symbol", node);
}

static void bindWith(Node* node, Array* parameters, const Array* globals) {
    switch (getASTType(node)) {
        case REFERENCE: bindReference(node, parameters, length(globals)); break;
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
        case COLONPAIR:
            syntaxError("colon not in valid location", node); break;
        case SETBUILDER:
            syntaxError("must appear on the right side of '::='", node); break;
        case OPERATOR:
            assert(false); break;
    }
}

Array* bind(Hold* root) {
    Node* node = getNode(root);
    Array* parameters = newArray(2048);         // names of globals and locals
    Array* globals = newArray(2048);            // values of globals
    while (isLet(node) && !isUnderscore(getParameter(getLeft(node)))) {
        Node* definiendum = getParameter(getLeft(node));
        Node* definiens = getRight(node);
        bindWith(definiens, parameters, globals);
        append(parameters, definiendum);
        append(globals, definiens);
        node = getBody(getLeft(node));
    }
    bindWith(node, parameters, globals);
    deleteArray(parameters);
    append(globals, node);
    return globals;
}
