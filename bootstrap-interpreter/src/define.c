#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "errors.h"
#include "patterns.h"

static bool hasRecursiveCalls(Node* node, Node* name) {
    if (isLambda(node)) {
        if (isSameToken(getParameter(node), name))
            syntaxError("symbol already defined", getParameter(node));
        return hasRecursiveCalls(getBody(node), name);
    }
    if (isApplication(node))
        return hasRecursiveCalls(getLeft(node), name)
            || hasRecursiveCalls(getRight(node), name);
    if (isReference(node))
        return isSameToken(node, name);
    return false;
}

static Node* newYCombinator(Tag tag) {
    Node* x = newBlankReference(tag, 1);
    Node* y = newBlankReference(tag, 2);
    Node* yxx = newApplication(tag, y, newApplication(tag, x, x));
    Node* xyxx = newLambda(tag, newBlank(tag), yxx);
    return newLambda(tag, newBlank(tag), newApplication(tag, xyxx, xyxx));
}

static Node* transformRecursion(Node* name, Node* value) {
    if (isComma(name) || !isReference(name) || !hasRecursiveCalls(value, name))
        return value;
    // value ==> (Y (name -> value))
    Tag tag = getTag(name);
    Node* yCombinator = newYCombinator(tag);
    return newApplication(tag, yCombinator, newLambda(tag, name, value));
}

static bool isTupleConstructor(Node* token) {
    return isLeaf(token) && getLexeme(token).start[0] == ',';
}

static bool isTuple(Node* node) {
    return isApplication(node) ? isTuple(getLeft(node)) :
        isTupleConstructor(node);
}

Node* reduceDefine(Node* operator, Node* left, Node* right) {
    Tag tag = getTag(operator);
    if (isTuple(left))
        return newDefinition(tag, left, right);
    for (; isApplication(left); left = getLeft(left))
        right = newDestructuringLambda(operator, getRight(left), right);
    syntaxErrorIf(isBuiltin(left), "cannot define a builtin operator", left);
    syntaxErrorIf(!isReference(left), "invalid left hand side", operator);
    return newDefinition(tag, left, transformRecursion(left, right));
}

static Node* applyDefinition(Node* definition, Node* scope) {
    // simple case: ((name = value) scope) ==> ((\name scope) value)
    Node* f = newDestructuringLambda(definition, getLeft(definition), scope);
    return newApplication(getTag(definition), f, getRight(definition));
}

static Node* newChurchPair(Tag tag, Node* left, Node* right) {
    return newLambda(tag, newBlank(tag), newApplication(tag,
        newApplication(tag, newBlankReference(tag, 1), left), right));
}

static Node* newMainCall(Node* main) {
    Tag tag = getTag(main);
    Node* print = newPrinter(tag);
    Node* get = newBuiltin(renameTag(tag, "get"), GET);
    Node* get0 = newApplication(tag, get, newInteger(tag, 0));
    Node* operators = newChurchPair(tag,
        newName(renameTag(tag, "[]")), newName(renameTag(tag, "::")));
    Node* input = newApplication(tag, get0, operators);
    return newApplication(tag, print, newApplication(tag, main, input));
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
    if (isADT(left)) {
        Stack* stack = (Stack*)getLeft(left);
        for (Iterator* it = iterate(stack); !end(it); it = next(it))
            right = applyDefinition(cursor(it), right);
        return right;
    }
    return newApplication(getTag(operator), left, right);
}

Node* reduceADTDefinition(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(!isValidPattern(left), "invalid left hand side", operator);
    syntaxErrorIf(!isADT(right), "right side must be an ADT", operator);
    return right;
}
