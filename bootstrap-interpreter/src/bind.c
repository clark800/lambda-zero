#include "lib/tree.h"
#include "lib/array.h"
#include "ast.h"
#include "errors.h"

static unsigned long long findDebruijnIndex(Node* symbol, Array* parameters) {
    for (unsigned long long i = 1; i <= length(parameters); ++i)
        if (isSameLexeme(elementAt(parameters, length(parameters) - i), symbol))
            return i;
    return 0;
}

static int findBuiltinCode(Node* node) {
    // names in builtins must line up with codes in BuiltinCode, except
    // UNDEFINED, EXIT, PUT, GET which don't have accessible names
    static const char* const builtins[] = {"+", "-", "*", "//", "%",
        "=", "=/=", "<", ">", "<=", ">=", "up", "error", "(undefined)"};
    for (unsigned int i = 0; i < sizeof(builtins)/sizeof(char*); ++i)
        if (isThisLexeme(node, builtins[i]))
            return (int)i;
    return -1;
}

static void bindSymbol(Node* node, Array* parameters, size_t globalDepth) {
    if (isUnused(node))
       syntaxError("cannot reference a symbol starting with underscore", node);
    int code = findBuiltinCode(node);
    if (code >= 0) {
        setValue(node, code);
        setType(node, BUILTIN);
        return;
    }
    unsigned long long index = findDebruijnIndex(node, parameters);
    syntaxErrorIf(index == 0, "undefined symbol", node);
    unsigned long long localDepth = length(parameters) - globalDepth;
    setValue(node, index <= localDepth ? (long long)index :
        (long long)(index - length(parameters) - 1));
}

static bool isDefined(Node* parameter, Array* parameters) {
    return isNatural(parameter) || (!isUnderscore(parameter) &&
        (findBuiltinCode(parameter) >= 0 ||
        findDebruijnIndex(parameter, parameters) != 0));
}

static void bindWith(Node* node, Array* parameters, const Array* globals) {
    // this error should never happen, but if something invalid gets through
    // we can at least point to the location of the problem
    if (isSymbol(node) && getValue(node) == 0) {
        bindSymbol(node, parameters, length(globals));
    } else if (isLambda(node)) {
        if (isDefined(getParameter(node), parameters))
            syntaxError("symbol already defined", getParameter(node));
        append(parameters, getParameter(node));
        bindWith(getBody(node), parameters, globals);
        unappend(parameters);
    } else if (isApplication(node)) {
        bindWith(getLeft(node), parameters, globals);
        bindWith(getRight(node), parameters, globals);
    } else if (isDefinition(node)) {
        syntaxError("missing scope for definition", node);
    }
}

static bool isDesugaredDefinition(Node* node) {
    return isApplication(node) && isLambda(getLeft(node)) &&
        !isUnderscore(getParameter(getLeft(node)));
}

Array* bind(Hold* root) {
    Node* node = getNode(root);
    Array* parameters = newArray(2048);         // names of globals and locals
    Array* globals = newArray(2048);            // values of globals
    while (isDesugaredDefinition(node)) {
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
