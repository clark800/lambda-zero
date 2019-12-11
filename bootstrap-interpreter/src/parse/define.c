#include "tree.h"
#include "opp/errors.h"
#include "opp/operator.h"
#include "ast.h"
#include "patterns.h"

bool isIO = false;

static Node* getHead(Node* node) {
    for (; isJuxtaposition(node); node = getLeft(node));
    return node;
}

static Node* applyPlainDefinition(Tag tag, Node* left, Node* right, Node* scope)
{
    // simple case: ((name = value) scope) ==> ((\name scope) value)
    return Let(tag, newLazyArrow(left, scope), right);
}

static Node* applyMaybeDefinition(Tag tag, Node* left, Node* right, Node* scope)
{
    return Juxtaposition(tag, Juxtaposition(tag, FixedName(tag, "onJust"),
            newLazyArrow(left, scope)), right);
}

static Node* applyTryDefinition(Tag tag, Node* left, Node* right, Node* scope) {
    // try a := b; c --> onRight(b, (a -> c)) --> (((onRight) b) (a -> c))
    return Juxtaposition(tag, Juxtaposition(tag, FixedName(tag, "onRight"),
            newLazyArrow(left, scope)), right);
}

static Node* newChurchPair(Tag tag, Node* left, Node* right) {
    return UnderscoreArrow(tag, Juxtaposition(tag,
        Juxtaposition(tag, Underscore(tag, 1), left), right));
}

Node* Printer(Tag tag) {
    Node* put = FixedName(tag, "(put)");
    Node* fold = FixedName(tag, "fold");
    Node* unit = FixedName(tag, "()");
    Node* under = Underscore(tag, 1);
    return UnderscoreArrow(tag, Juxtaposition(tag,
        Juxtaposition(tag, Juxtaposition(tag, fold, put), unit), under));
}

static Node* newMainCall(Node* name) {
    isIO = true;
    Tag tag = getTag(name);
    Node* print = Printer(tag);
    Node* get = FixedName(tag, "(get)");
    Node* get0 = Juxtaposition(tag, get, Number(tag, 0));
    Node* operators = newChurchPair(tag,
        FixedName(tag, "[]"), FixedName(tag, "::"));
    Node* input = Juxtaposition(tag, get0, operators);
    return Juxtaposition(tag, print, Juxtaposition(tag, name, input));
}

static bool hasRecursiveCalls(Node* node, Node* name) {
    if (!isLeaf(node))
        return hasRecursiveCalls(getLeft(node), name)
            || hasRecursiveCalls(getRight(node), name);
    return isName(node) ? isSameLexeme(node, name) : false;
}

static Node* transformRecursion(Node* name, Node* value) {
    if (!isName(name) || !isArrow(value) || !hasRecursiveCalls(value, name))
        return value;
    // value ==> (fix (name -> value))
    Tag tag = getTag(name);
    Node* fix = FixedName(tag, "fix");
    return Juxtaposition(tag, fix, LockedArrow(name, value));
}

static bool isValidConstructorParameter(Node* parameter) {
    return isColonPair(parameter) && isName(getLeft(parameter)) &&
        isValidPattern(getRight(parameter));
}

static Node* newConstructorDefinition(Tag tag, Node* form, Node* scope,
        unsigned int i, unsigned int n) {
    // form is an application of a constructor name to a number of parameters
    // i is the index of this constructor in this algebraic data type
    // n is the total number of constructors for this algebraic data type
    // j is the index of the constructor parameter
    // m is the total number of parameters for this constructor
    // k = m - j - 1
    Node* name = getHead(form);
    unsigned int m = getArgumentCount(form);

    if (isNumber(form) && getValue(form) == 0) {
        Node* zero = FixedName(getTag(form), "0");
        return applyPlainDefinition(tag, zero, form, scope);
    }

    if (isJuxtaposition(form) && isThisName(getLeft(form), "up")) {
        Node* increment = FixedName(getTag(name), "(increment)");
        return applyPlainDefinition(tag, name, increment, scope);
    }

    syntaxErrorIf(!isName(name), "invalid constructor name", name);

    // let p_* be constructor parameters (m total)
    // let c_* be constructor names (n total)
    // build: p_1 -> ... -> p_m -> c_1 -> ... -> c_n -> c_i p_1 ... p_m
    Node* constructor = Underscore(tag, (unsigned long long)(n - i));
    for (unsigned int j = 0; j < m; ++j)
        constructor = Juxtaposition(tag, constructor,
            Underscore(tag, (unsigned long long)(n + m - j)));
    for (unsigned int q = 0; q < n + m; ++q)
        constructor = UnderscoreArrow(tag, constructor);

    Node* node = form;
    for (unsigned int k = 0; k < m; ++k, node = getLeft(node))
        if (!isValidConstructorParameter(getRight(node)))
            syntaxError("invalid constructor parameter", getRight(node));
    return applyPlainDefinition(tag, name, constructor, scope);
}

static Node* newFallbackCase(Tag tag, unsigned int m) {
    // ignore all m parameters and return f applied to parameter above
    Node* fallback = Underscore(tag, m + 2);
    Node* instance = Underscore(tag, m + 1);
    Node* body = Juxtaposition(tag, fallback, instance);
    for (unsigned int j = 0; j < m; ++j)
        body = UnderscoreArrow(tag, body);
    return body;
}

static Node* newDeconstructorDefinition(Tag tag, Node* form, Node* scope,
        unsigned int ms[], unsigned int i, unsigned int n) {
    // deconstructor :
    // (reconstructor : p1 => ... => pm => T) => (fallback : A => T) => A => T
    Node* reconstructor = Underscore(tag, 3);
    Node* body = Underscore(tag, 1);
    for (unsigned int j = 0; j < n; ++j)
        body = Juxtaposition(tag, body, j == i ?
            reconstructor : newFallbackCase(tag, ms[j]));
    Node* deconstructor = UnderscoreArrow(tag,
        UnderscoreArrow(tag, UnderscoreArrow(tag, body)));
    Node* name = Name(addPrefix(getTag(getHead(form)), '@'));
    return applyPlainDefinition(tag, name, deconstructor, scope);
}

static Node* applyADTDefinition(Tag tag, Node* left, Node* adt, Node* scope) {
    // for each item in the forms tuple, define constructor functions
    Node* forms = getRight(adt);
    Node* node = forms;
    unsigned int n = getArgumentCount(forms);
    unsigned int ms[256]; // store the number of arguments for each constructor

    for (unsigned int i = 0; i < n; ++i, node = getLeft(node))
        ms[n - i - 1] = getArgumentCount(getRight(node));

    node = forms;
    // define deconstructor first in case the constructor name is the same
    // as the ADT name: this ensures that we always bind the ADT name
    for (unsigned int i = 0; i < n; ++i, node = getLeft(node)) {
        Node* form = isColonPair(getRight(node)) ?
            getLeft(getRight(node)) : getRight(node);
        scope = newConstructorDefinition(tag, form, scope, n - i - 1, n);
        scope = newDeconstructorDefinition(tag, form, scope, ms, n - i - 1, n);
    }

    // define ADT name, but make it forbidden to access because the defined
    // value is fake since this interpreter doesn't support types
    Node* name = ForbiddenName(getTag(getHead(left)));
    return applyPlainDefinition(tag, name, Number(tag, 0), scope);
}

Node* applyDefinition(Node* definition, Node* scope) {
    Tag tag = getTag(definition);
    Node* definiendum = getLeft(definition);
    Node* definiens = getRight(definition);
    switch ((DefinitionVariety)getVariety(definition)) {
        case PLAINDEFINITION:
            return applyPlainDefinition(tag, definiendum, definiens, scope);
        case MAYBEDEFINITION:
            return applyMaybeDefinition(tag, definiendum, definiens, scope);
        case TRYDEFINITION:
            return applyTryDefinition(tag, definiendum, definiens, scope);
        case SYNTAXDEFINITION:
            return scope;
        case ADTDEFINITION:
            return applyADTDefinition(tag, definiendum, definiens, scope);
    }
    assert(false);
    return NULL;
}

Node* reduceApply(Node* operator, Node* left, Node* right) {
    return Juxtaposition(getTag(operator), left, right);
}

static Node* reduceAdfix(Node* operator, Node* argument) {
    return reduceApply(operator, Name(getTag(operator)), argument);
}

Node* reducePrefix(Node* operator, Node* left, Node* right) {
    (void)left;
    return reduceAdfix(operator, right);
}

static Node* reducePostfix(Node* operator, Node* left, Node* right) {
    (void)right;
    return reduceAdfix(operator, left);
}

Node* reduceInfix(Node* operator, Node* left, Node* right) {
    return reduceApply(operator, reduceAdfix(operator, left), right);
}

static void defineSyntax(Node* definition, Node* left, Node* right) {
    syntaxErrorIf(!isJuxtaposition(left), "invalid left operand", left);
    Node* name = getRight(left);
    syntaxErrorIf(!isName(name), "expected symbol operand to", getLeft(left));
    if (!isJuxtaposition(right))
        syntaxError("invalid syntax definition", definition);

    Tag tag = getTag(name);
    Node* fixity = getLeft(right);
    Node* argument = getRight(right);

    if (isThisName(fixity, "alias") || isThisName(fixity, "syntax")) {
        syntaxErrorIf(!isName(argument), "expected operator name", argument);
        addSyntaxCopy(getLexeme(name), argument, isThisName(fixity, "alias"));
        return;
    }

    Precedence precedence = isNumber(argument) ?
        (Precedence)getValue(argument) : 0;
    syntaxErrorIf(precedence > 99, "invalid precedence", argument);

    Node* prior = isNumber(argument) ? NULL : argument;

    if (isThisName(fixity, "infix"))
        addSyntax(tag, prior, precedence, INFIX, N, reduceInfix);
    else if (isThisName(fixity, "infixL"))
        addSyntax(tag, prior, precedence, INFIX, L, reduceInfix);
    else if (isThisName(fixity, "infixR"))
        addSyntax(tag, prior, precedence, INFIX, R, reduceInfix);
    else if (isThisName(fixity, "interfix"))
        addSyntax(tag, prior, precedence, INFIX, L, reduceApply);
    else if (isThisName(fixity, "prefix"))
        addSyntax(tag, prior, precedence, PREFIX, L, reducePrefix);
    else if (isThisName(fixity, "postfix"))
        addSyntax(tag, prior, precedence, POSTFIX, L, reducePostfix);
    else syntaxError("invalid fixity", fixity);
}

Node* reduceDefine(Node* operator, Node* left, Node* right) {
    Tag tag = getTag(operator);
    DefinitionVariety variety = PLAINDEFINITION;
    if (isKeyphrase(left, "maybe")) {
        variety = MAYBEDEFINITION;
        left = getRight(left);
    } else if (isKeyphrase(left, "try")) {
        variety = TRYDEFINITION;
        left = getRight(left);
    } else if (isKeyphrase(left, "syntax")) {
        defineSyntax(operator, left, right);
        return Definition(tag, SYNTAXDEFINITION, left, right);
    }

    if (isColonPair(right))
        return reduceDefine(operator, left, getLeft(right));
    if (isColonPair(left))
        return reduceDefine(operator, getLeft(left), right);
    if (isTuple(left) || isAsPattern(left))
        return Definition(tag, variety, left, right);
    for (; isJuxtaposition(left); left = getLeft(left))
        right = newLazyArrow(getRight(left), right);
    syntaxErrorIf(!isName(left), "invalid left hand side", operator);
    if (isThisName(left, "main"))
        return applyPlainDefinition(tag, left, right, newMainCall(left));
    return Definition(tag, variety, left, transformRecursion(left, right));
}

Node* reduceADTDefinition(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(!isValidPattern(left), "invalid left hand side", operator);
    syntaxErrorIf(!isSetBuilder(right), "ADT required to right of", operator);
    return Definition(getTag(operator), ADTDEFINITION, left, right);
}
