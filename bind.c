#include "lib/tree.h"
#include "lib/array.h"
#include "ast.h"
#include "objects.h"
#include "lex.h"
#include "errors.h"
#include "bind.h"

static unsigned long long findDebruijnIndex(Node* symbol, Array* parameters) {
    for (unsigned long long i = 1; i <= length(parameters); i++)
        if (isSameToken(elementAt(parameters, length(parameters) - i), symbol))
            return i;
    return 0;
}

unsigned long long lookupBuiltinCode(Node* token) {
    const char* const builtins[] = {"+", "-", "*", "/", "\\", "==", "=/=",
        "<", ">", "<=", ">=", "error", "'exit'", "'put'", "'get'"};
    for (unsigned long long i = 0; i < sizeof(builtins)/sizeof(char*); i++)
        if (isThisToken(token, builtins[i]))
            return i + BUILTIN;
    return 0;
}

static void bindSymbol(Node* symbol, Array* parameters, size_t globalDepth) {
    if (IDENTITY != NULL && isThisToken(symbol, "_"))
        syntaxError("cannot reference", symbol);
    unsigned long long code = lookupBuiltinCode(symbol);
    if (code > 0) {
        convertToBuiltin(symbol, code);
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
        convertToGlobal(symbol, length(parameters) - index);
    else
        convertToReference(symbol, index);
}

bool isDefined(Node* symbol, Array* parameters) {
    // internal tokens are exempted to allow e.g. lists
    return !isThisToken(symbol, "_") && (lookupBuiltinCode(symbol) != 0 ||
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
        !isTuple(getLeft(node)) && !isList(getLeft(node)) &&
        !isThisToken(getParameter(getLeft(node)), "_");
}

Program bind(Hold* root, bool optimize) {
    Node* node = getNode(root);
    Array* parameters = newArray(2048);        // names of globals and locals
    Array* globals = newArray(optimize ? 2048 : 0);   // values of globals
    while (optimize && isLetExpression(node)) {
        Node* definedSymbol = getParameter(getLeft(node));
        Node* definedValue = getRight(node);
        if (isDefined(definedSymbol, parameters))
            syntaxError("symbol already defined", definedSymbol);

        bindWith(definedValue, parameters, globals);
        append(parameters, definedSymbol);
        append(globals, definedValue);
        node = getBody(getLeft(node));
    }
    bool IO = length(parameters) > 0 &&
        isThisToken(elementAt(parameters, length(parameters) - 1), "main");
    bindWith(node, parameters, globals);
    deleteArray(parameters);
    return (Program){root, node, globals, IO};
}
