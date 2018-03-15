#include <stdbool.h>
#include "lib/tree.h"
#include "ast.h"
#include "lex.h"
#include "objects.h"
#include "desugar.h"

static inline bool isDefinition(Node* node) {
    return isApplication(node) && isThisToken(node, "=");
}

bool hasRecursiveCalls(Node* node, Node* name) {
    if (isLambda(node)) {
        syntaxErrorIf(isSameToken(getParameter(node), name), getParameter(node),
            "cannot use definition name as parameter name");
        return hasRecursiveCalls(getBody(node), name);
    }
    if (isApplication(node))
        return hasRecursiveCalls(getLeft(node), name)
            || hasRecursiveCalls(getRight(node), name);
    if (isSymbol(node) || isReference(node))
        return isSameToken(node, name);
    return false;
}

Node* transformRecursion(Node* name, Node* value) {
    if (!hasRecursiveCalls(value, name))
        return value;
    // value ==> (Y (name -> value))
    int location = getLocation(name);
    return newApplication(location, YCOMBINATOR,
        newLambda(location, newParameter(getLocation(name)), value));
}

Node* getScope(Node* explicitScope, Node* name) {
    if (explicitScope != NULL)
        return explicitScope;
    if (!isThisToken(name, "main"))
        return name;
    IO = true;
    int location = getLocation(name);
    return newApplication(location,
            newApplication(location, name, INPUT), PRINT);
}

Node* transformDefine(Node* definition, Node* explicitScope) {
    // simple case: ((name = value) scope) ==> ((\name scope) value)
    Node* left = getLeft(definition);
    Node* right = getRight(definition);
    while (isApplication(left)) {
        Node* parameterName = getRight(left);
        syntaxErrorIf(!isName(parameterName), parameterName,
            "expected name but got");
        Node* parameter = newParameter(getLocation(parameterName));
        right = newLambda(getLocation(parameter), parameter, right);
        left = getLeft(left);
    }
    Node* name = left;
    Node* value = transformRecursion(name, right);
    int location = getLocation(name);
    return newBranchNode(location, newLambda(location,
                newParameter(location), getScope(explicitScope, name)), value);
}

Node* constructDefine(Node* node, Node* left, Node* right) {
    if (isDefinition(right))
        right = transformDefine(right, NULL);
    if (isDefinition(left)) {
        syntaxErrorIf(isDefinition(node), node, "cannot define a definition");
        return transformDefine(left, right);
    }
    return newBranchNode(getLocation(node), left, right);
}

Hold* desugarDefine(Node* node) {
    if (isLeafNode(node))
        return hold(node);
    Hold* left = desugarDefine(getLeft(node));
    Hold* right = desugarDefine(getRight(node));
    Hold* result = hold(constructDefine(node, getNode(left), getNode(right)));
    release(left);
    release(right);
    return result;
}

Hold* desugar(Node* node) {
    Hold* result = desugarDefine(node);
    if (!isDefinition(getNode(result)))
        return result;
    return replaceHold(result, hold(transformDefine(getNode(result), NULL)));
}
