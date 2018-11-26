#include "lib/tree.h"
#include "ast.h"
#include "errors.h"
#include "patterns.h"

bool isIO = false;

Node* getHead(Node* node) {
    for (; isApplication(node); node = getLeft(node));
    return node;
}

Node* applyDefinition(Tag tag, Node* left, Node* right, Node* scope) {
    // simple case: ((name = value) scope) ==> ((\name scope) value)
    return Application(tag, newPatternLambda(tag, left, scope), right);
}

Node* applyTryDefinition(Tag tag, Node* left, Node* right, Node* scope) {
    // try a := b; c --> b ? (a -> c) --> (((?) b) (a -> c))
    return Application(tag, Application(tag, FixedName(tag, "??"), right),
            newPatternLambda(tag, left, scope));
}

static Node* newChurchPair(Tag tag, Node* left, Node* right) {
    return Abstraction(tag, Underscore(tag, 0), Application(tag,
        Application(tag, Underscore(tag, 1), left), right));
}

static Node* newMainCall(Node* name) {
    isIO = true;
    Tag tag = getTag(name);
    Node* print = Printer(tag);
    Node* get = Operation(renameTag(tag, "get"), GET);
    Node* get0 = Application(tag, get, Numeral(tag, 0));
    Node* operators = newChurchPair(tag,
        FixedName(tag, "[]"), FixedName(tag, "::"));
    Node* input = Application(tag, get0, operators);
    return Application(tag, print, Application(tag, name, input));
}

static bool isTuple(Node* node) {
    // a tuple is a spine of applications headed by a name starting with comma
    return isApplication(node) ? isTuple(getLeft(node)) :
        (isName(node) && getLexeme(node).start[0] == ',');
}

static bool hasRecursiveCalls(Node* node, Node* name) {
    if (!isLeaf(node))
        return hasRecursiveCalls(getLeft(node), name)
            || hasRecursiveCalls(getRight(node), name);
    return isName(node) ? isSameLexeme(node, name) : false;
}

static Node* transformRecursion(Node* name, Node* value) {
    if (!isName(name) || !hasRecursiveCalls(value, name))
        return value;
    // value ==> (fix (name -> value))
    Tag tag = getTag(name);
    Node* fix = FixedName(tag, "fix");
    return Application(tag, fix, Abstraction(tag, name, value));
}

Node* reduceDefine(Node* operator, Node* left, Node* right) {
    Tag tag = renameTag(getTag(operator), ":=");
    if (isApplication(left) && isThisLexeme(left, "try")) {
        tag = renameTag(tag, "try");
        left = getRight(left);
    }
    if (isThisLexeme(left, "syntax") || isTuple(left) || isAsPattern(left))
        return Definition(tag, left, right);
    for (; isApplication(left); left = getLeft(left))
        right = newPatternLambda(tag, getRight(left), right);
    syntaxErrorIf(!isName(left), "invalid left hand side", operator);
    if (isThisLeaf(left, "main"))
        return applyDefinition(getTag(left), left, right, newMainCall(left));
    return Definition(tag, left, transformRecursion(left, right));
}

static inline bool isValidPattern(Node* node) {
    return isName(node) || (isApplication(node) &&
        isValidPattern(getLeft(node)) && isValidPattern(getRight(node)));
}

static bool isValidConstructorParameter(Node* parameter) {
    return isApplication(parameter) && isApplication(getLeft(parameter)) &&
        isValidPattern(getRight(parameter)) &&
        isName(getRight(getLeft(parameter))) &&
        (isThisLeaf(getLeft(getLeft(parameter)), ":") ||
         isThisLeaf(getLeft(getLeft(parameter)), "\u2208"));
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
    Node* getter = Underscore(tag, 1);
    for (unsigned int k = 0; k < n; ++k)
        getter = Application(tag, getter, k == i ? projector :
            FixedName(tag, "undefined"));
    getter = Abstraction(tag, Underscore(tag, 0), getter);
    return applyDefinition(tag, name, getter, scope);
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
    syntaxErrorIf(!isName(pattern), "invalid constructor name", pattern);

    // let p_* be constructor parameters (m total)
    // let c_* be constructor names (n total)
    // build: p_1 -> ... -> p_m -> c_1 -> ... -> c_n -> c_i p_1 ... p_m
    Node* constructor = Underscore(tag, (unsigned long long)(n - i));
    for (unsigned int j = 0; j < m; ++j)
        constructor = Application(tag, constructor,
            Underscore(tag, (unsigned long long)(n + m - j)));
    for (unsigned int q = 0; q < n + m; ++q)
        constructor = Abstraction(tag, Underscore(tag, 0), constructor);
    return applyDefinition(tag, pattern, constructor, scope);
}

Node* applyADTDefinition(Tag tag, Node* left, Node* adt, Node* scope) {
    // define ADT name so that the symbol can't be defined twice
    // TODO: this should be the outermost definition
    Node* undefined = FixedName(tag, "undefined");
    scope = applyDefinition(tag, getHead(left), undefined, scope);

    // for each item in the patterns tuple, define a constructor function
    unsigned int n = getArgumentCount(adt);
    for (unsigned int i = 1; isApplication(adt); ++i, adt = getLeft(adt))
        scope = newConstructorDefinition(tag, getRight(adt), scope, n - i, n);
    return scope;
}

Node* reduceADTDefinition(Node* operator, Node* left, Node* right) {
    Tag tag = renameTag(getTag(operator), "::=");
    syntaxErrorIf(!isValidPattern(left), "invalid left hand side", operator);
    if (!isThisLexeme(right, "{") && !isThisLexeme(right, "{}"))
        syntaxError("ADT required to right of", operator);
    return Definition(tag, left, right);
}
