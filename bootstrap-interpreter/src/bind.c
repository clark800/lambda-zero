#include "lib/tree.h"
#include "lib/array.h"
#include "ast.h"
#include "errors.h"

static unsigned long long findDebruijnIndex(Node* symbol, Array* parameters) {
    for (unsigned long long i = 1; i <= length(parameters); ++i)
        if (isSameToken(elementAt(parameters, length(parameters) - i), symbol))
            return i;
    return 0;
}

static void bindReference(Node* node, Array* parameters, size_t globalDepth) {
    syntaxErrorIf(isThisToken(node, "_"), "cannot reference", node);
    unsigned long long index = findDebruijnIndex(node, parameters);
    syntaxErrorIf(index == 0, "undefined symbol", node);
    unsigned long long localDepth = length(parameters) - globalDepth;
    setValue(node, index <= localDepth ? (long long)index :
        (long long)(index - length(parameters) - 1));
}

static bool isDefined(Node* reference, Array* parameters) {
    return !isThisToken(reference, "_") &&
        findDebruijnIndex(reference, parameters) != 0;
}

static void bindWith(Node* node, Array* parameters, const Array* globals) {
    // this error should never happen, but if something invalid gets through
    // we can at least point to the location of the problem
    syntaxErrorIf(getParseType(node) != OPERAND, "invalid syntax", node);
    if (isReference(node) && getValue(node) == 0) {
        bindReference(node, parameters, length(globals));
    } else if (isLambda(node)) {
        if (isDefined(getParameter(node), parameters))
            syntaxError("symbol already defined", getParameter(node));
        append(parameters, getParameter(node));
        bindWith(getBody(node), parameters, globals);
        unappend(parameters);
    } else if (isApplication(node)) {
        bindWith(getLeft(node), parameters, globals);
        bindWith(getRight(node), parameters, globals);
    }
}

static bool isDesugaredDefinition(Node* node) {
    return isApplication(node) && isLambda(getLeft(node)) &&
        !isThisToken(getParameter(getLeft(node)), "_");
}

Array* bind(Hold* root) {
    Node* node = getNode(root);
    Array* parameters = newArray(2048);         // names of globals and locals
    Array* globals = newArray(2048);            // values of globals
    for (; isDesugaredDefinition(node); node = getBody(getLeft(node))) {
        Node* definedSymbol = getParameter(getLeft(node));
        Node* definedValue = getRight(node);
        if (isDefined(definedSymbol, parameters))
            syntaxError("symbol already defined", definedSymbol);

        bindWith(definedValue, parameters, globals);
        append(parameters, definedSymbol);
        append(globals, definedValue);
    }
    bindWith(node, parameters, globals);
    deleteArray(parameters);
    append(globals, node);
    return globals;
}
