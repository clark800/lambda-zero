#include <stdbool.h>
#include "lib/tree.h"
#include "ast.h"
#include "errors.h"
#include "lex.h"
#include "objects.h"
#include "operators.h"
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
    if (!isSymbol(name) || !hasRecursiveCalls(value, name))
        return value;
    // value ==> (Y (name -> value))
    int location = getLocation(name);
    return newApplication(location, YCOMBINATOR,
        newLambda(location, newParameter(location), value));
}

Node* getScope(Node* explicitScope, Node* name) {
    if (explicitScope != NULL)
        return explicitScope;
    if (!isName(name) || !isThisToken(name, "main"))
        return name;
    int location = getLocation(name);
    return newApplication(location,
        newApplication(location, name, INPUT), PRINT);
}

Node* transformDefine(Node* definition, Node* explicitScope) {
    // simple case: ((name = value) scope) ==> ((\name scope) value)
    Node* left = getLeft(definition);
    Node* right = getRight(definition);
    for (; isApplication(left); left = getLeft(left))
        right = newArrow(definition, getRight(left), right);
    Node* value = transformRecursion(left, right);
    Node* scope = getScope(explicitScope, left);
    return newApplication(getLocation(left),
        newArrow(definition, left, scope), value);
}

Node* constructDefine(Node* node, Node* left, Node* right) {
    if (isDefinition(right))
        right = transformDefine(right, NULL);
    if (isDefinition(left)) {
        syntaxErrorIf(isDefinition(node), "cannot define a definition", node);
        return transformDefine(left, right);
    }
    return newApplication(getLocation(node), left, right);
}

Hold* desugarDefine(Node* node) {
    if (isLeaf(node))
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
