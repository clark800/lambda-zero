#include "lib/tree.h"
#include "ast.h"
#include "errors.h"
#include "operators.h"
#include "define.h"

bool hasRecursiveCalls(Node* node, Node* name) {
    if (isLambda(node)) {
        if (isSameToken(getParameter(node), name))
            syntaxError("symbol already defined", getParameter(node));
        return hasRecursiveCalls(getBody(node), name);
    }
    if (isApplication(node))
        return hasRecursiveCalls(getLeft(node), name)
            || hasRecursiveCalls(getRight(node), name);
    if (isName(node) || isReference(node))
        return isSameToken(node, name);
    return false;
}

Node* transformRecursion(Node* name, Node* value) {
    if (isComma(name) || !isName(name) || !hasRecursiveCalls(value, name))
        return value;
    // value ==> (Y (name -> value))
    Tag tag = getTag(name);
    Node* yCombinator = newYCombinator(tag);
    return newApplication(tag, yCombinator, newLambda(tag, name, value));
}

Node* reduceDefine(Node* operator, Node* left, Node* right) {
    Tag tag = getTag(operator);
    for (; isApplication(left); left = getLeft(left))
        right = newPatternLambda(operator, getRight(left), right);
    return newDefinition(tag, left, transformRecursion(left, right));
}

Node* applyDefinition(Node* definition, Node* scope) {
    // simple case: ((name = value) scope) ==> ((\name scope) value)
    Node* name = getLeft(definition);
    Node* value = getRight(definition);
    return newApplication(getTag(definition),
        newPatternLambda(definition, name, scope), value);
}

Node* newChurchPair(Tag tag, Node* left, Node* right) {
    return newLambda(tag, newBlank(tag), newApplication(tag,
        newApplication(tag, newBlankReference(tag, 1), left), right));
}

Node* newMainCall(Node* main) {
    Tag tag = getTag(main);
    Node* printer = newPrinter(tag);
    Node* get = newBuiltin(renameTag(tag, "get"), GET);
    Node* get0 = newApplication(tag, get, newInteger(tag, 0));
    Node* operators = newChurchPair(tag,
        newName(renameTag(tag, "[]")), newName(renameTag(tag, "::")));
    Node* input = newApplication(tag, get0, operators);
    return newApplication(tag, newApplication(tag, main, input), printer);
}

Node* transformDefinition(Node* definition) {
    Node* name = getLeft(definition);
    syntaxErrorIf(!isThisToken(name, "main"), "missing scope", definition);
    return applyDefinition(definition, newMainCall(name));
}

Node* reduceNewline(Node* operator, Node* left, Node* right) {
    if (isDefinition(right))
        right = transformDefinition(right);
    if (isDefinition(left))
        return applyDefinition(left, right);
    return newApplication(getTag(operator), left, right);
}
