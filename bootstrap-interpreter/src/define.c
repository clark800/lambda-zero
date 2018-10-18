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
    Node* underscore = newUnderscore(tag, 0);
    Node* x = newUnderscore(tag, 1);
    Node* y = newUnderscore(tag, 2);
    Node* yxx = newApplication(tag, y, newApplication(tag, x, x));
    Node* xyxx = newLambda(tag, underscore, yxx);
    return newLambda(tag, underscore, newApplication(tag, xyxx, xyxx));
}

static Node* transformRecursion(Node* name, Node* value) {
    if (!isSymbol(name) || !hasRecursiveCalls(value, name))
        return value;
    // value ==> (Y (name -> value))
    Tag tag = getTag(name);
    Node* yCombinator = newYCombinator(tag);
    return newApplication(tag, yCombinator, newLambda(tag, name, value));
}

static bool isTuple(Node* node) {
    // a tuple is a spine of applications headed by a symbol starting with comma
    return isApplication(node) ? isTuple(getLeft(node)) :
        (isSymbol(node) && getLexeme(node).start[0] == ',');
}

static Node* newDefinition(Tag tag, Node* name, Node* value, Node* scope) {
    // simple case: ((name = value) scope) ==> ((\name scope) value)
    return newApplication(tag, reduceLambda(name, name, scope), value);
}

static Node* newChurchPair(Tag tag, Node* left, Node* right) {
    return newLambda(tag, newUnderscore(tag, 0), newApplication(tag,
        newApplication(tag, newUnderscore(tag, 1), left), right));
}

static Node* newMainCall(Node* main) {
    isIO = true;
    Tag tag = getTag(main);
    Node* print = newPrinter(tag);
    Node* get = newBuiltin(renameTag(tag, "get"), GET);
    Node* get0 = newApplication(tag, get, newNatural(tag, 0));
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
        isSymbol(getRight(getLeft(parameter))) &&
        isThisLeaf(getLeft(getLeft(parameter)), ":");
}

static Node* newGetterDefinition(Tag tag, Node* parameter, Node* scope,
        unsigned int i, unsigned int n, unsigned int j, unsigned int m) {
    if (!isValidConstructorParameter(parameter))
        syntaxError("invalid constructor parameter", parameter);
    Node* name = getRight(getLeft(parameter));
    if (isUnused(name))
        return scope;
    // defined argument = c_1 -> ... -> c_n -> c_i P_1 ... P_m
    // undefined argument = c_1 -> ... -> c_n -> c_x ...    (x != i)
    // getter is a function that returns P_j for defined arguments and
    // undefined for undefined arguments:
    // getter = _ -> _ undefined (1) ... projector (i) ... undefined (n)
    Node* projector = newProjector(tag, m, j);
    Node* getter = newUnderscore(tag, 1);
    for (unsigned int k = 0; k < n; ++k)
        getter = newApplication(tag, getter, k == i ? projector :
            newName(renameTag(tag, "undefined")));
    getter = newLambda(tag, newUnderscore(tag, 0), getter);
    return newDefinition(tag, name, getter, scope);
}

static Node* newConstructorDefinition(Tag tag, Node* pattern, Node* scope,
        unsigned int i, unsigned int n) {
    // pattern is an application of a constructor name to a number of parameters
    // i is the index of this constructor in this algebraic data type
    // n is the total number of constructors for this algebraic data type
    // j is the index of the constructor parameter
    // m is the total number of parameters for this constructor
    // k = m - j - 1
    unsigned int m = getArgumentCount(pattern);
    for (unsigned int k = 0; k < m; ++k, pattern = getLeft(pattern))
        scope = newGetterDefinition(tag, getRight(pattern), scope, i, n,
            m - k - 1, m);
    syntaxErrorIf(!isSymbol(pattern), "invalid constructor name", pattern);

    // let p_* be constructor parameters (m total)
    // let c_* be constructor names (n total)
    // build: p_1 -> ... -> p_m -> c_1 -> ... -> c_n -> c_i p_1 ... p_m
    Node* constructor = newUnderscore(tag, (unsigned long long)(n - i));
    for (unsigned int j = 0; j < m; ++j)
        constructor = newApplication(tag, constructor,
            newUnderscore(tag, (unsigned long long)(n + m - j)));
    for (unsigned int q = 0; q < n + m; ++q)
        constructor = newLambda(tag, newUnderscore(tag, 0), constructor);
    return newDefinition(tag, pattern, constructor, scope);
}

Node* reduceADTDefinition(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(!isValidPattern(left), "invalid left hand side", operator);
    if (!isApplication(right) || !isThisLexeme(right, "\n"))
        syntaxError("missing scope", operator);

    Node* adt = getLeft(right);
    if (!isThisLexeme(adt, "{") && !isThisLexeme(adt, "{}"))
        syntaxError("ADT required to right of", operator);

    // define ADT name so that the symbol can't be defined twice
    Tag tag = getTag(operator);
    Node* undefined = newName(renameTag(tag, "undefined"));
    Node* scope = newDefinition(tag, getHead(left), undefined, getRight(right));

    // for each item in the patterns tuple, define a constructor function
    unsigned int n = getArgumentCount(adt);
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
