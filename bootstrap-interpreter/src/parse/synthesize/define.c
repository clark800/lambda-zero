#include "shared/lib/tree.h"
#include "parse/shared/errors.h"
#include "parse/shared/ast.h"
#include "patterns.h"

bool isIO = false;

Node* getHead(Node* node) {
    for (; isJuxtaposition(node); node = getLeft(node));
    return node;
}

Node* applyPlainDefinition(Tag tag, Node* left, Node* right, Node* scope) {
    // simple case: ((name = value) scope) ==> ((\name scope) value)
    return Let(tag, newPatternLambda(tag, left, scope), right);
}

Node* applyTryDefinition(Tag tag, Node* left, Node* right, Node* scope) {
    // try a := b; c --> b ? (a -> c) --> (((?) b) (a -> c))
    return Juxtaposition(tag, Juxtaposition(tag, FixedName(tag, "??"), right),
            newPatternLambda(tag, left, scope));
}

static Node* newChurchPair(Tag tag, Node* left, Node* right) {
    return UnderscoreArrow(tag, Juxtaposition(tag,
        Juxtaposition(tag, Underscore(tag, 1), left), right));
}

Node* Printer(Tag tag) {
    Node* put = Name(renameTag(tag, "(put)"), 0);
    Node* fold = FixedName(tag, "fold");
    Node* unit = FixedName(tag, "()");
    Node* under = Underscore(tag, 1);
    return UnderscoreArrow(tag, Juxtaposition(tag,
        Juxtaposition(tag, Juxtaposition(tag, fold, under), put), unit));
}

static Node* newMainCall(Node* name) {
    isIO = true;
    Tag tag = getTag(name);
    Node* print = Printer(tag);
    Node* get = Name(renameTag(tag, "(get)"), 0);
    Node* get0 = Juxtaposition(tag, get, Number(tag, 0));
    Node* operators = newChurchPair(tag,
        FixedName(tag, "[]"), FixedName(tag, "::"));
    Node* input = Juxtaposition(tag, get0, operators);
    return Juxtaposition(tag, print, Juxtaposition(tag, name, input));
}

static bool isTuple(Node* node) {
    // a tuple is a spine of applications headed by a name starting with comma
    return isJuxtaposition(node) ? isTuple(getLeft(node)) :
        (isName(node) && getLexeme(node).start[0] == ',');
}

static bool hasRecursiveCalls(Node* node, Node* name) {
    if (!isLeaf(node))
        return hasRecursiveCalls(getLeft(node), name)
            || hasRecursiveCalls(getRight(node), name);
    return isName(node) ? isSameLexeme(node, name) : false;
}

static Node* transformRecursion(Node* name, Node* value) {
    if (!isName(name) || (!isArrow(value) && !isCase(value)) ||
            !hasRecursiveCalls(value, name))
        return value;
    // value ==> (fix (name -> value))
    Tag tag = getTag(name);
    return Juxtaposition(tag, FixedName(tag, "fix"), Arrow(tag, name, value));
}

static inline bool isValidPattern(Node* node) {
    return isName(node) || (isJuxtaposition(node) &&
        isValidPattern(getLeft(node)) && isValidPattern(getRight(node)));
}

static bool isValidConstructorParameter(Node* parameter) {
    return isJuxtaposition(parameter) && isJuxtaposition(getLeft(parameter)) &&
        isValidPattern(getRight(parameter)) &&
        isName(getRight(getLeft(parameter))) &&
        (isThisName(getLeft(getLeft(parameter)), ":") ||
         isThisName(getLeft(getLeft(parameter)), "\u2208"));
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
        getter = Juxtaposition(tag, getter, k == i ? projector :
            FixedName(tag, "undefined"));
    getter = UnderscoreArrow(tag, getter);
    return applyPlainDefinition(tag, name, getter, scope);
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
        constructor = Juxtaposition(tag, constructor,
            Underscore(tag, (unsigned long long)(n + m - j)));
    for (unsigned int q = 0; q < n + m; ++q)
        constructor = UnderscoreArrow(tag, constructor);
    return applyPlainDefinition(tag, pattern, constructor, scope);
}

Node* applyADTDefinition(Tag tag, Node* left, Node* adt, Node* scope) {
    // define ADT name so that the symbol can't be defined twice
    // TODO: this should be the outermost definition
    Node* undefined = FixedName(tag, "undefined");
    scope = applyPlainDefinition(tag, getHead(left), undefined, scope);
    Node* t = getRight(adt);

    // for each item in the patterns tuple, define a constructor function
    unsigned int n = getArgumentCount(t);
    for (unsigned int i = 1; isJuxtaposition(t); ++i, t = getLeft(t))
        scope = newConstructorDefinition(tag, getRight(t), scope, n - i, n);
    return scope;
}

Node* applyDefinition(Node* definition, Node* scope) {
    Tag tag = getTag(definition);
    Node* name = getLeft(definition);
    Node* value = getRight(definition);
    switch ((DefinitionVariety)getVariety(definition)) {
        case PLAINDEFINITION:
            return applyPlainDefinition(tag, name, value, scope);
        case TRYDEFINITION:
            return applyTryDefinition(tag, name, value, scope);
        case SYNTAXDEFINITION:
            return scope;
        case ADTDEFINITION:
            return applyADTDefinition(tag, name, value, scope);
    }
    assert(false);
    return NULL;
}

Node* reduceDefine(Node* operator, Node* left, Node* right) {
    Tag tag = getTag(operator);
    DefinitionVariety variety = PLAINDEFINITION;
    if (isKeyphrase(left, "try")) {
        variety = TRYDEFINITION;
        left = getRight(left);
    }
    if (isKeyphrase(left, "syntax"))
        return Definition(tag, SYNTAXDEFINITION, left, right);
    if (isTuple(left) || isAsPattern(left))
        return Definition(tag, variety, left, right);
    for (; isJuxtaposition(left); left = getLeft(left))
        right = newPatternLambda(tag, getRight(left), right);
    syntaxErrorIf(!isName(left), "invalid left hand side", operator);
    if (isThisName(left, "main"))
        return applyPlainDefinition(tag, left, right, newMainCall(left));
    return Definition(tag, variety, left, transformRecursion(left, right));
}

Node* reduceADTDefinition(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(!isValidPattern(left), "invalid left hand side", operator);
    syntaxErrorIf(!isADT(right), "ADT required to right of", operator);
    return Definition(getTag(operator), ADTDEFINITION, left, right);
}
