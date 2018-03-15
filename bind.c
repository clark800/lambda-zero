#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "lex.h"
#include "bind.h"

static unsigned long long findDebruijnIndex(Node* symbol, Stack* parameters) {
    unsigned long long i = 0;
    for (Iterator* it = iterate(parameters); !end(it); it = next(it), i++)
        if (isSameToken(cursor(it), symbol))
            return i + 1;
    return 0;
}

unsigned long long lookupBuiltinCode(Node* token) {
    const char* const builtins[] = {"+", "-", "*", "/", "mod",
        "==", "=/=", "<", ">", "<=", ">=", "$put", "$get"};
    for (unsigned long long i = 0; i < sizeof(builtins)/sizeof(char*); i++)
        if (isThisToken(token, builtins[i]))
            return i + BUILTIN;
    return 0;
}

static void bindSymbol(Node* symbol, Stack* parameterStack) {
    unsigned long long code = lookupBuiltinCode(symbol);
    if (code > 0) {
        convertSymbolToBuiltin(symbol, code);
        return;
    }
    unsigned long long index = findDebruijnIndex(symbol, parameterStack);
    syntaxErrorIf(index == 0, symbol, "undefined symbol");
    convertSymbolToReference(symbol, index);
}

static bool isDefined(Node* symbol, Stack* parameterStack) {
    // internal tokens are exempted to allow e.g. tuples inside tuples
    return !isInternalToken(symbol) &&
        (lookupBuiltinCode(symbol) != 0 ||
        findDebruijnIndex(symbol, parameterStack) != 0);
}

static void bindWith(Node* node, Stack* parameterStack) {
    if (isSymbol(node)) {
        bindSymbol(node, parameterStack);
    } else if (isLambda(node)) {
        syntaxErrorIf(isDefined(getParameter(node), parameterStack),
            getParameter(node), "symbol already defined");
        push(parameterStack, getParameter(node));
        bindWith(getBody(node), parameterStack);
        release(pop(parameterStack));
    } else if (isApplication(node)) {
        bindWith(getLeft(node), parameterStack);
        bindWith(getRight(node), parameterStack);
    } else {
        // allow references so we can paste parsed trees in with sugars
        assert(isReference(node) || isInteger(node) || isBuiltin(node));
    }
}

void bind(Node* root) {
    Stack* parameterStack = newStack(NULL);
    bindWith(root, parameterStack);
    deleteStack(parameterStack);
}
