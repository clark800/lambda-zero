#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "errors.h"
#include "operators.h"

static Node* applyToCommaList(Tag tag, Node* base, Node* arguments) {
    if (!isCommaList(arguments))
        return base == NULL ? arguments : newApplication(tag, base, arguments);
    return newApplication(tag, applyToCommaList(tag, base,
        getLeft(arguments)), getRight(arguments));
}

static Node* newTuple(Node* open, Node* commaList, const char name[32]){
    unsigned int length = getCommaListLength(commaList) - 1;
    syntaxErrorIf(length > 32, "too many arguments", open);
    Tag tag = newTag(newString(name, length), getTag(open).location);
    return applyToCommaList(getTag(open), newName(tag), commaList);
}

Node* reduceParentheses(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "("), "missing open for", close);
    Tag tag = getTag(open);
    if (contents == NULL)
        return newName(renameTag(tag, "()"));
    syntaxErrorIf(isDefinition(contents), "missing scope", contents);
    if (getFixity(open) == OPENCALL) {
        return isCommaList(contents) ? applyToCommaList(tag, NULL, contents) :
            // desugar f() to f(())
            newApplication(tag, contents, newName(renameTag(tag, "()")));
    }

    if (isOperator(contents)) {
        // update rules to favor infix over prefix inside parenthesis
        setRules(contents, false);
        if (isSpecialOperator(contents) && !isComma(contents))
            syntaxError("operator cannot be parenthesized", contents);
        return convertOperator(contents);
    }
    static const char commas[32] = ",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,";
    if (isCommaList(contents))
        return newTuple(open, contents, commas);
    if (isApplication(contents))
        return setLocation(contents, getLocation(open));
    return contents;
}

Node* reduceSquareBrackets(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "["), "missing open for", close);
    Tag tag = getTag(open);
    if (contents == NULL)
        return newNil(tag);
    syntaxErrorIf(isDefinition(contents), "missing scope", contents);
    if (getFixity(open) == OPENCALL) {
        static const char opens[32] = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[";
        syntaxErrorIf(!isCommaList(contents), "missing argument to", open);
        // not really a tuple, but it is the same structure
        return newTuple(open, contents, opens);
    }
    Node* list = newNil(tag);
    if (!isCommaList(contents))
        return prepend(tag, contents, list);
    for(; isCommaList(contents); contents = getLeft(contents))
        list = prepend(tag, getRight(contents), list);
    return prepend(tag, contents, list);
}

static Node* newConstructorDefinition(Node* pattern, int i, int n) {
    // pattern is an application of a constructor name to a number of asterisks
    // i is the index of this constructor in this algebraic data type
    // n is the total number of constructors for this algebraic data type

    // verify that all arguments in pattern are asterisks and count to get k
    int k = 0;
    for (; isApplication(pattern); ++k, pattern = getLeft(pattern))
        syntaxErrorIf(!isThisToken(getRight(pattern), "(*)"),
            "constructor parameters must be asterisks", getRight(pattern));

    Tag tag = getTag(pattern);
    // let p_* be constructor parameters (k total)
    // let c_* be constructor names (n total)
    // build: p_1 -> ... -> p_k -> c_1 -> ... -> c_n -> c_i p_1 ... p_k
    Node* constructor = newBlankReference(tag, (unsigned long long)(n - i));
    for (int j = 0; j < k; ++j)
        constructor = newApplication(tag, constructor,
            newBlankReference(tag, (unsigned long long)(n + k - j)));
    for (int j = 0; j < n + k; ++j)
        constructor = newLambda(tag, newBlank(tag), constructor);
    syntaxErrorIf(!isReference(pattern), "invalid constructor name", pattern);
    return newDefinition(tag, pattern, constructor);
}

Node* reduceCurlyBrackets(Node* close, Node* open, Node* patterns) {
    syntaxErrorIf(patterns == NULL, "missing patterns", open);
    syntaxErrorIf(!isThisToken(open, "{"), "missing open for", close);
    syntaxErrorIf(isDefinition(patterns), "missing scope", patterns);
    // for each item in the patterns comma list, define a constructor function,
    // then return all of these definitions as a comma list, which the parser
    // converts to a sequence of lines
    int n = (int)getCommaListLength(patterns);
    Stack* definitions = newStack();
    for (int i = 0; isCommaList(patterns); ++i, patterns = getLeft(patterns))
        push(definitions,
            newConstructorDefinition(getRight(patterns), n - i - 1, n));
    push(definitions, newConstructorDefinition(patterns, 0, n));
    return newADT(getTag(open), (Node*)definitions);
}
