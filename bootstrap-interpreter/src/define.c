#include "lib/tree.h"
#include "ast.h"
#include "errors.h"
#include "operators.h"
#include "define.h"

static bool hasRecursiveCalls(Node* node, Node* name) {
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

static Node* transformRecursion(Node* name, Node* value) {
    if (isComma(name) || !isName(name) || !hasRecursiveCalls(value, name))
        return value;
    // value ==> (Y (name -> value))
    Tag tag = getTag(name);
    Node* yCombinator = newYCombinator(tag);
    return newApplication(tag, yCombinator, newLambda(tag, name, value));
}

bool isTupleConstructor(Node* token) {
    return isLeaf(token) && getLexeme(token).start[0] == ',';
}

bool isTuple(Node* node) {
    return isApplication(node) ? isTuple(getLeft(node)) :
        isTupleConstructor(node) && getValue(node) == CONVERSION;
}

Node* reduceDefine(Node* operator, Node* left, Node* right) {
    Tag tag = getTag(operator);
    if (isTuple(left))
        return newDefinition(tag, left, right);
    for (; isApplication(left); left = getLeft(left))
        right = newPatternLambda(operator, getRight(left), right);
    syntaxErrorIf(!isName(left), "invalid left hand side", operator);
    return newDefinition(tag, left, transformRecursion(left, right));
}

static Node* applyDefinition(Node* definition, Node* scope) {
    // simple case: ((name = value) scope) ==> ((\name scope) value)
    Node* left = getLeft(definition);
    Node* right = getRight(definition);
    if (isApplication(left)) {  // must be a tuple
        for (; isApplication(left); left = getLeft(left))
            scope = newPatternLambda(definition, getRight(left), scope);
        return newApplication(getTag(definition), right, scope);
    }
    return newApplication(getTag(definition),
        newLambda(getTag(definition), left, scope), right);
}

static Node* newChurchPair(Tag tag, Node* left, Node* right) {
    return newLambda(tag, newBlank(tag), newApplication(tag,
        newApplication(tag, newBlankReference(tag, 1), left), right));
}

static Node* newMainCall(Node* main) {
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
