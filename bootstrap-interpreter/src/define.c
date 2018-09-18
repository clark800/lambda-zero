#include "lib/tree.h"
#include "ast.h"
#include "errors.h"
#include "patterns.h"

bool isIO = false;

static bool hasRecursiveCalls(Node* node, Node* name) {
    if (isLambda(node)) {
        if (isSameLexeme(getParameter(node), name))
            syntaxError("symbol already defined", getParameter(node));
        return hasRecursiveCalls(getBody(node), name);
    }
    if (isApplication(node))
        return hasRecursiveCalls(getLeft(node), name)
            || hasRecursiveCalls(getRight(node), name);
    if (isSymbol(node))
        return isSameLexeme(node, name);
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
    if (!isSymbol(name) || !hasRecursiveCalls(value, name))
        return value;
    // value ==> (Y (name -> value))
    Tag tag = getTag(name);
    Node* yCombinator = newYCombinator(tag);
    return newApplication(tag, yCombinator, newLambda(tag, name, value));
}

static bool isTupleConstructor(Node* node) {
    return isSymbol(node) && getLexeme(node).start[0] == ',';
}

static bool isTuple(Node* node) {
    return isApplication(node) ? isTuple(getLeft(node)) :
        isTupleConstructor(node);
}

static Node* newDefinition(Tag tag, Node* name, Node* value, Node* scope) {
    // simple case: ((name = value) scope) ==> ((\name scope) value)
    return newApplication(tag, reduceLambda(name, name, scope), value);
}

static Node* newChurchPair(Tag tag, Node* left, Node* right) {
    return newLambda(tag, newBlank(tag), newApplication(tag,
        newApplication(tag, newBlankReference(tag, 1), left), right));
}

static Node* newMainCall(Node* main) {
    isIO = true;
    Tag tag = getTag(main);
    Node* print = newPrinter(tag);
    Node* get = newBuiltin(renameTag(tag, "get"), GET);
    Node* get0 = newApplication(tag, get, newInteger(tag, 0));
    Node* operators = newChurchPair(tag,
        newName(renameTag(tag, "[]")), newName(renameTag(tag, "::")));
    Node* input = newApplication(tag, get0, operators);
    return newApplication(tag, print, newApplication(tag, main, input));
}

static inline bool isValidPattern(Node* node) {
    return isSymbol(node) || (isApplication(node) &&
        isValidPattern(getLeft(node)) && isValidPattern(getRight(node)));
}

static bool isValidConstructorParameter(Node* parameter) {
    return isApplication(parameter) && isApplication(getLeft(parameter)) &&
        isValidPattern(getRight(parameter)) &&
        isBlank(getRight(getLeft(parameter))) &&
        isThisLeaf(getLeft(getLeft(parameter)), ":");
}

static Node* newConstructorDefinition(Tag tag, Node* pattern, Node* scope,
        unsigned int i, unsigned int n) {
    // pattern is an application of a constructor name to a number of asterisks
    // i is the index of this constructor in this algebraic data type
    // n is the total number of constructors for this algebraic data type

    // verify that all arguments in pattern are asterisks and count to get k
    unsigned int k = 0;
    for (; isApplication(pattern); ++k, pattern = getLeft(pattern))
        if (!isValidConstructorParameter(getRight(pattern)))
            syntaxError("invalid constructor parameter", getRight(pattern));
    syntaxErrorIf(!isSymbol(pattern), "invalid constructor name", pattern);

    // let p_* be constructor parameters (k total)
    // let c_* be constructor names (n total)
    // build: p_1 -> ... -> p_k -> c_1 -> ... -> c_n -> c_i p_1 ... p_k
    Node* constructor = newBlankReference(tag, (unsigned long long)(n - i));
    for (unsigned int j = 0; j < k; ++j)
        constructor = newApplication(tag, constructor,
            newBlankReference(tag, (unsigned long long)(n + k - j)));
    for (unsigned int j = 0; j < n + k; ++j)
        constructor = newLambda(tag, newBlank(tag), constructor);
    return newDefinition(tag, pattern, constructor, scope);
}

Node* reduceADTDefinition(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(!isValidPattern(left), "invalid left hand side", operator);
    if (!isApplication(right) || getTag(right).lexeme.start[0] != '\n')
        syntaxError("missing scope", operator);
    Node* adt = getLeft(right);
    Node* scope = getRight(right);
    if (!isApplication(adt) || getTag(adt).lexeme.start[0] != '{')
        syntaxError("ADT required to right of", operator);
    // for each item in the patterns comma list, define a constructor function,
    // then return all of these definitions as a comma list, which the parser
    // converts to a sequence of lines
    unsigned int n = getArgumentCount(adt);
    Tag tag = getTag(operator);
    for (unsigned int i = 1; isApplication(adt); ++i, adt = getLeft(adt))
        scope = newConstructorDefinition(tag, getRight(adt), scope, n - i, n);
    return scope;
}

Node* reduceDefine(Node* operator, Node* left, Node* right) {
    Tag tag = getTag(operator);
    if (!isApplication(right) || getTag(right).lexeme.start[0] != '\n') {
        if (isApplication(left) && isThisLeaf(getLeft(left), "main"))
            return newMainCall(newLambda(tag, getRight(left), right));
        syntaxError("missing scope", operator);
    }
    Node* value = getLeft(right);
    Node* scope = getRight(right);
    if (isTuple(left))
        return newDefinition(tag, left, value, scope);
    for (; isApplication(left); left = getLeft(left))
        value = reduceLambda(operator, getRight(left), value);
    syntaxErrorIf(isBuiltin(left), "cannot define a builtin operator", left);
    syntaxErrorIf(!isSymbol(left), "invalid left hand side", operator);
    return newDefinition(tag, left, transformRecursion(left, value), scope);
}
