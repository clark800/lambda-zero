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

unsigned long long lookupBuiltinCode(Node* token, bool internal) {
    const char* const builtins[] = {"+", "-", "*", "/", "%", "=", "!=",
        "<", ">", "<=", ">=", "error", "exit", "put", "get"};
    unsigned long long length = sizeof(builtins)/sizeof(char*);
    // the last three builtins are only accessible from the internal code
    unsigned long long limit = internal ? length : length - 3;
    for (unsigned long long i = 0; i < limit; i++)
        if (isThisToken(token, builtins[i]))
            return i + BUILTIN;
    return 0;
}

static void bindSymbol(Node* symbol, Array* parameters, size_t globalDepth,
        bool internal) {
    if (!internal && isThisToken(symbol, "_"))
        syntaxError("cannot reference", symbol);
    unsigned long long code = lookupBuiltinCode(symbol, internal);
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

bool isDefined(Node* symbol, Array* parameters, bool internal) {
    return !isInternal(getLexeme(symbol)) && !isThisToken(symbol, "_") &&
        (lookupBuiltinCode(symbol, internal) != 0 ||
             findDebruijnIndex(symbol, parameters) != 0);
}

void bindWith(Node* node, Array* parameters, const Array* globals,
        bool internal) {
    if (isSymbol(node)) {
        bindSymbol(node, parameters, length(globals), internal);
    } else if (isLambda(node)) {
        if (!isTuple(node) && isDefined(getParameter(node),
                parameters, internal))
            syntaxError("symbol already defined", getParameter(node));
        append(parameters, getParameter(node));
        bindWith(getBody(node), parameters, globals, internal);
        unappend(parameters);
    } else if (isApplication(node)) {
        bindWith(getLeft(node), parameters, globals, internal);
        bindWith(getRight(node), parameters, globals, internal);
    }
}

bool isLetExpression(Node* node) {
    return isApplication(node) && isLambda(getLeft(node)) &&
        !isTuple(getLeft(node)) && !isList(getLeft(node)) &&
        !isThisToken(getParameter(getLeft(node)), "_");
}

Array* bind(Hold* root, bool internal) {
    Node* node = getNode(root);
    Array* parameters = newArray(2048);        // names of globals and locals
    Array* globals = newArray(internal ? 0 : 2048);   // values of globals
    while (!internal && isLetExpression(node)) {
        Node* definedSymbol = getParameter(getLeft(node));
        Node* definedValue = getRight(node);
        if (isDefined(definedSymbol, parameters, internal))
            syntaxError("symbol already defined", definedSymbol);

        bindWith(definedValue, parameters, globals, internal);
        append(parameters, definedSymbol);
        append(globals, definedValue);
        node = getBody(getLeft(node));
    }
    bindWith(node, parameters, globals, internal);
    deleteArray(parameters);
    append(globals, node);
    return globals;
}
