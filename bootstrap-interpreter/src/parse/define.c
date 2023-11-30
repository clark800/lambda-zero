#include "tree.h"
#include "opp/operator.h"
#include "ast.h"
#include "patterns.h"
#include "define.h"

bool isIO = false;

static Node* getHead(Node* node) {
    for (; isJuxtaposition(node); node = getLeft(node));
    return node;
}

static Node* applyPlainDefinition(Tag tag, Node* left, Node* right, Node* scope)
{
    // simple case: ((name = value) scope) ==> ((\name scope) value)
    return Let(tag, newArrow(left, scope), right);
}

static Node* applyMaybeDefinition(Tag tag, Node* left, Node* right, Node* scope)
{
    return Juxtaposition(tag, Juxtaposition(tag, FixedName(tag, "onJust"),
            newArrow(left, scope)), right);
}

static Node* applyTryDefinition(Tag tag, Node* left, Node* right, Node* scope) {
    // try a := b; c --> onRight(b, (a -> c))
    return Juxtaposition(tag, Juxtaposition(tag, FixedName(tag, "onRight"),
            newArrow(left, scope)), right);
}

static Node* applyBindDefinition(Tag tag, Node* left, Node* right, Node* scope){
    // left <- right;; scope ==> right >> (left -> scope)
    return Juxtaposition(tag, Juxtaposition(tag,
        FixedName(tag, ">>"), right), newArrow(left, scope));
}

static Node* applyTryBindDefinition(Tag tag, Node* left, Node* right,
        Node* scope) {
    // try a <- b; c --> onRightState(b, (a -> c))
    return Juxtaposition(tag, Juxtaposition(tag, FixedName(tag, "onRightState"),
            newArrow(left, scope)), right);
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
    Node* operators = newChurchPair(tag, FixedName(tag, "[]"),
        Name(newLiteralTag("::", getLexeme(tag).location, INFIX)));
    Node* input = Juxtaposition(tag, get0, operators);
    return Juxtaposition(tag, print, Juxtaposition(tag, name, input));
}

static bool containsFreeName(Node* node, Node* name) {
    switch (getASTType(node)) {
        case REFERENCE:
            return getValue(node) == 0 && isSameTag(getTag(node), getTag(name));
        case ARROW:
            return !isSameTag(getTag(getLeft(node)), getTag(name))
                && containsFreeName(getRight(node), name);
        case LET:
        case JUXTAPOSITION:
            return containsFreeName(getLeft(node), name)
                || containsFreeName(getRight(node), name);
        case NUMBER:
            return false;
        default:
            assert(false);
            return false;
    }
}

static Node* transformRecursion(Node* name, Node* value) {
    if (!isName(name) || !isArrow(value) || !containsFreeName(value, name))
        return value;
    // value ==> (fix (name -> value))
    Tag tag = getTag(name);
    return Juxtaposition(tag, FixedName(tag, "fix"), SingleArrow(name, value));
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

    syntaxErrorNodeIf(!isName(name), "invalid constructor name", name);

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
            syntaxErrorNode("invalid constructor parameter", getRight(node));
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
    unsigned int n = forms == NULL ? 0 : getArgumentCount(forms);
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
        case BINDDEFINITION:
            return applyBindDefinition(tag, definiendum, definiens, scope);
        case TRYBINDDEFINITION:
            return applyTryBindDefinition(tag, definiendum, definiens, scope);
    }
    assert(false);
    return NULL;
}

static Node* reduceAdfix(Tag tag, Node* argument) {
    return Juxtaposition(tag, Name(tag), argument);
}

Node* reducePrefix(Tag tag, Node* left, Node* right) {
    (void)left;
    return reduceAdfix(tag, right);
}

static Node* reducePostfix(Tag tag, Node* left, Node* right) {
    (void)right;
    return reduceAdfix(tag, left);
}

Node* reduceInfix(Tag tag, Node* left, Node* right) {
    return Juxtaposition(tag, reduceAdfix(tag, left), right);
}

static void defineSyntax(Tag definitionTag, Node* left, Node* right) {
    syntaxErrorNodeIf(!isJuxtaposition(left), "invalid left operand", left);
    Node* name = getRight(left);
    syntaxErrorNodeIf(!isName(name), "expected name operand to", getLeft(left));
    if (!isJuxtaposition(right))
        syntaxError("invalid syntax definition", definitionTag);

    Tag tag = getTag(name);
    Node* fixity = getLeft(right);
    Node* argument = getRight(right);

    if (isThisName(fixity, "alias") || isThisName(fixity, "syntax")) {
        if (!isName(argument))
            syntaxErrorNode("expected operator name", argument);
        addSyntaxCopy(getLexeme(tag), argument, isThisName(fixity, "alias"));
        return;
    }

    Precedence precedence = isNumber(argument) ?
        (Precedence)getValue(argument) : 0;
    syntaxErrorNodeIf(precedence > 99, "invalid precedence", argument);

    Node* prior = isNumber(argument) ? NULL : argument;

    if (isThisName(fixity, "infix"))
        addSyntax(tag, prior, precedence, INFIX, N, reduceInfix);
    else if (isThisName(fixity, "infixL"))
        addSyntax(tag, prior, precedence, INFIX, L, reduceInfix);
    else if (isThisName(fixity, "infixR"))
        addSyntax(tag, prior, precedence, INFIX, R, reduceInfix);
    else if (isThisName(fixity, "interfix"))
        addSyntax(tag, prior, precedence, INFIX, L, Juxtaposition);
    else if (isThisName(fixity, "prefix"))
        addSyntax(tag, prior, precedence, PREFIX, L, reducePrefix);
    else if (isThisName(fixity, "postfix"))
        addSyntax(tag, prior, precedence, POSTFIX, L, reducePostfix);
    else syntaxErrorNode("invalid fixity", fixity);
}

Node* reduceDefine(Tag tag, Node* left, Node* right) {
    DefinitionVariety variety = PLAINDEFINITION;
    syntaxErrorIf(isCommaPair(right), "invalid right hand side", tag);
    if (isKeyphrase(left, "maybe")) {
        variety = MAYBEDEFINITION;
        left = getRight(left);
    } else if (isKeyphrase(left, "try")) {
        variety = TRYDEFINITION;
        left = getRight(left);
    } else if (isKeyphrase(left, "syntax")) {
        defineSyntax(tag, left, right);
        return Definition(tag, SYNTAXDEFINITION, left, right);
    }

    if (isTuple(left) || isAsPattern(left))
        return Definition(tag, variety, left, right);
    for (; isJuxtaposition(left); left = getLeft(left))
        right = newArrow(getRight(left), right);
    if (isColonPair(left))
        left = getLeft(left);
    syntaxErrorIf(!isName(left), "invalid left hand side", tag);
    if (isThisName(left, "main"))
        return applyPlainDefinition(tag, left, right, newMainCall(left));
    return Definition(tag, variety, left, transformRecursion(left, right));
}

Node* reduceADTDefinition(Tag tag, Node* left, Node* right) {
    syntaxErrorIf(!isValidPattern(left), "invalid left hand side", tag);
    syntaxErrorIf(!isSetBuilder(right), "ADT required to right of", tag);
    return Definition(tag, ADTDEFINITION, left, right);
}
