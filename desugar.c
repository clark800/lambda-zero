#include <stdbool.h>
#include "lib/tree.h"
#include "ast.h"
#include "lex.h"
#include "parse.h"
#include "builtins.h"
#include "operators.h"
#include "desugar.h"

bool detectDefine(Node* node) {
    return isApplication(node) && isAssignment(getLeft(node));
}

bool hasRecursiveCalls(Node* node, Node* name) {
    if (isAbstraction(node)) {
        Node* parameter = getParameter(node);
        syntaxErrorIf(isSameToken(parameter, name), parameter,
            "cannot use definition name as parameter name");
        return hasRecursiveCalls(getBody(node), name);
    }
    if (isApplication(node))
        return hasRecursiveCalls(getLeft(node), name)
            || hasRecursiveCalls(getRight(node), name);
    if (isSymbol(node) || isReference(node))
        // could have a reference here for the placeholder
        return isSameToken(node, name);
    assert(isInteger(node));
    return false;
}

Node* transformLambdaSugar(Node* operator, Node* left, Node* right) {
    while (isApplication(left)) {
        Node* parameterName = getRight(left);
        syntaxErrorIf(!isName(parameterName), parameterName,
            "expected name but got");
        Node* parameter = newParameter(getLocation(parameterName));
        right = newLambda(getLocation(operator), parameter, right);
        left = getLeft(left);
    }
    return collapseLambda(operator, left, right);
}

void swapRight(Node* a, Node* b) {
    Hold* rightB = hold(getRight(b));
    setRight(b, getRight(a));
    setRight(a, getNode(rightB));
    release(rightB);
}

void transformRecursion(Node* definition) {
    Node* name = getLeft(definition);
    Node* value = getRight(definition);
    if (hasRecursiveCalls(value, name)) {
        // value ==> (Y (name -> value))
        int location = getLocation(definition);
        setRight(definition, newApplication(location, YCOMBINATOR,
            newLambda(location, newParameter(getLocation(name)), value)));
    }
}

void transformDefine(Node* node) {
    // simple case: ((name = value) body) ==> ((\name body) value)
    Node* definition = getLeft(node);
    Node* value = getRight(definition);
    Node* leftHandSide = getLeft(definition);
    setLeft(node, transformLambdaSugar(definition, leftHandSide, value));
    transformRecursion(getLeft(node));
    swapRight(node, getLeft(node));
}

void desugar(Node* node) {
    if (detectDefine(node))
        transformDefine(node);

    if (isBranchNode(node)) {
        desugar(getLeft(node));
        desugar(getRight(node));
    }
}
