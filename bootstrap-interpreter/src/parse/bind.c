#include "tree.h"
#include "array.h"
#include "opp/errors.h"
#include "ast.h"
#include "term.h"
#include "bind.h"

static unsigned long long findDebruijnIndex(Node* name, Array* parameters) {
    syntaxErrorNodeIf(isUnused(name),
        "cannot reference a symbol starting with underscore", name);
    for (size_t i = 1; i <= length(parameters); ++i) {
        Node* parameter = elementAt(parameters, length(parameters) - i);
        if (isSameTag(getTag(parameter), getTag(name))) {
            syntaxErrorNodeIf(isForbidden(name), "cannot reference", name);
            return (unsigned long long)i;
        }
    }
    return 0;
}

static OperationCode findOperationCode(Node* name) {
    for (OperationCode i = 0; i < sizeof(Operations)/sizeof(char*); ++i)
        if (isThisName(name, Operations[i]))
            return i;
    return NONE;
}

static void bindReference(Node* node, Array* parameters, size_t globalDepth) {
    OperationCode operationCode = findOperationCode(node);
    if (isPseudoOperation(operationCode)) {
        setType(node, OPERATION);
        setVariety(node, (char)operationCode);
        return;
    }
    unsigned long long i = (unsigned long long)getValue(node);
    unsigned long long index = i > 0 ? i : findDebruijnIndex(node, parameters);
    syntaxErrorNodeIf(index == 0, "undefined symbol", node);
    unsigned long long localDepth = length(parameters) - globalDepth;
    long long debruijn = (long long)(index <= localDepth ? index :
        index - length(parameters) - 1);
    setType(node, VARIABLE);
    setValue(node, debruijn);
}

static void bindWith(Node* node, Array* parameters, const Array* globals) {
    switch (getASTType(node)) {
        case REFERENCE: bindReference(node, parameters, length(globals)); break;
        case ARROW:
            append(parameters, getParameter(node));
            bindWith(getBody(node), parameters, globals);
            unappend(parameters);
            setTag(node, getTag(getParameter(node)));
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
            syntaxErrorNode("missing scope for definition", node); break;
        case ASPATTERN:
            syntaxErrorNode("as pattern not in valid location", node); break;
        case COMMAPAIR:
            syntaxErrorNode("comma not inside brackets", node); break;
        case COLONPAIR:
            syntaxErrorNode("colon not in valid location", node); break;
        case SETBUILDER:
            syntaxErrorNode("bracket not in valid location", node); break;
        case OPERATOR:
            assert(false); break;
    }
}

Array* bind(Hold* root) {
    Node* node = root;
    Array* parameters = newArray(2048);         // names of globals and locals
    Array* globals = newArray(2048);            // values of globals
    while (isLet(node) && !isUnderscore(getParameter(getLeft(node)))) {
        Node* definiendum = getParameter(getLeft(node));
        Tag tag = getTag(definiendum);
        Node* definiens = getRight(node);
        bindWith(definiens, parameters, globals);
        OperationCode code = findOperationCode(definiendum);
        if (code != NONE && !isPseudoOperation(code))
            setRight(node, Operation(tag, code, definiens));
        append(parameters, definiendum);
        append(globals, getRight(node));
        setType(node, APPLICATION);
        setType(getLeft(node), ABSTRACTION);
        setTag(getLeft(node), getTag(getParameter(getLeft(node))));
        node = getBody(getLeft(node));
    }
    bindWith(node, parameters, globals);
    deleteArray(parameters);
    append(globals, node);
    return globals;
}
