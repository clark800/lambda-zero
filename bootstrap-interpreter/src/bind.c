#include "lib/tree.h"
#include "lib/array.h"
#include "ast.h"
#include "errors.h"
#include "bind.h"

static unsigned long long findDebruijnIndex(Node* symbol, Array* parameters) {
    for (unsigned long long i = 1; i <= length(parameters); i++)
        if (isSameToken(elementAt(parameters, length(parameters) - i), symbol))
            return i;
    return 0;
}

int lookupBuiltinCode(Node* token) {
    for (unsigned int i = 0; i < sizeof(BUILTINS)/sizeof(char*); i++)
        if (isThisToken(token, BUILTINS[i]))
            return (int)i;
    return -1;
}

static void bindSymbol(Node* symbol, Array* parameters, size_t globalDepth) {
    if (isThisToken(symbol, "_"))
        syntaxError("cannot reference", symbol);
    int code = lookupBuiltinCode(symbol);
    if (code >= 0) {
        convertSymbol(symbol, BUILTIN, code);
        return;
    }
    unsigned long long index = findDebruijnIndex(symbol, parameters);
    if (index == 0) {
        if (isThisToken(symbol, ","))
            syntaxError("missing parentheses around", symbol);
        syntaxError("undefined symbol", symbol);
    }
    unsigned long long localDepth = length(parameters) - globalDepth;
    if (index > localDepth)
        convertSymbol(symbol, GLOBAL, (long long)(length(parameters) - index));
    else
        convertSymbol(symbol, REFERENCE, (long long)index);
}

bool isDefined(Node* symbol, Array* parameters) {
    return !isThisToken(symbol, "_") && (lookupBuiltinCode(symbol) >= 0 ||
            findDebruijnIndex(symbol, parameters) != 0);
}

void bindWith(Node* node, Array* parameters, const Array* globals) {
    if (isSymbol(node)) {
        bindSymbol(node, parameters, length(globals));
    } else if (isLambda(node)) {
        if (!isTuple(node) && isDefined(getParameter(node), parameters))
            syntaxError("symbol already defined", getParameter(node));
        append(parameters, getParameter(node));
        bindWith(getBody(node), parameters, globals);
        unappend(parameters);
    } else if (isApplication(node)) {
        bindWith(getLeft(node), parameters, globals);
        bindWith(getRight(node), parameters, globals);
    }
}

bool isLetExpression(Node* node) {
    return isApplication(node) && isLambda(getLeft(node)) &&
        !isTuple(getLeft(node)) &&
        !isThisToken(getParameter(getLeft(node)), "_");
}

Array* bind(Hold* root) {
    Node* node = getNode(root);
    Array* parameters = newArray(2048);         // names of globals and locals
    Array* globals = newArray(2048);            // values of globals
    while (isLetExpression(node)) {
        Node* definedSymbol = getParameter(getLeft(node));
        Node* definedValue = getRight(node);
        if (isDefined(definedSymbol, parameters))
            syntaxError("symbol already defined", definedSymbol);

        bindWith(definedValue, parameters, globals);
        append(parameters, definedSymbol);
        append(globals, definedValue);
        node = getBody(getLeft(node));
    }
    bindWith(node, parameters, globals);
    deleteArray(parameters);
    append(globals, node);
    return globals;
}
