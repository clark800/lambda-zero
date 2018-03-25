#include <stdbool.h>
#include "lib/tree.h"
#include "ast.h"
#include "lex.h"
#include "objects.h"
#include "desugar.h"

bool hasRecursiveCalls(Node* node, Node* name) {
    if (isLambda(node)) {
        if (isSameToken(getParameter(node), name))
            syntaxError("symbol already defined", getParameter(node));
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
        newLambda(location, newParameter(location), value));
}

Node* getScope(Node* explicitScope, Node* name) {
    if (explicitScope != NULL)
        return explicitScope;
    if (!isThisToken(name, "main"))
        return name;
    int location = getLocation(name);
    return newApplication(location,
        newApplication(location, name, INPUT), PRINT);
}

Node* transformDefine(Node* definition, Node* explicitScope) {
    // simple case: ((name = value) scope) ==> ((\name scope) value)
    Node* left = getLeft(definition);
    Node* right = getRight(definition);
    // special case to allow defining the comma operator for use in
    // (,) and sections: (, x) and (x ,). this is necessary because it will
    // be wrapped in a tuple abstraction by the time it reaches here
    if (isTuple(left))
        left = getBody(left);
    for (; isApplication(left); left = getLeft(left)) {
        Node* parameterName = getRight(left);
        if (!isName(parameterName))
            syntaxError("expected name but got", parameterName);
        Node* parameter = newParameter(getLocation(parameterName));
        right = newLambda(getLocation(parameter), parameter, right);
    }
    Node* name = left;
    syntaxErrorIf(!isSymbol(name), "expected symbol but got", name);
    Node* value = transformRecursion(name, right);
    int location = getLocation(name);
    return newBranchNode(location, newLambda(location,
                newParameter(location), getScope(explicitScope, name)), value);
}

Node* constructDefine(Node* node, Node* left, Node* right) {
    if (isDefinition(right))
        right = transformDefine(right, NULL);
    if (isDefinition(left)) {
        syntaxErrorIf(isDefinition(node), "cannot define a definition", node);
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
