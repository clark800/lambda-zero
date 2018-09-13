#include "lib/tree.h"
#include "ast.h"
#include "errors.h"
#include "operators.h"

bool isCommaList(Node* node) {
    return isApplication(node) && isThisString(getLexeme(node), ",");
}

static unsigned int getCommaListLength(Node* node) {
    return !isCommaList(node) ? 1 : 1 + getCommaListLength(getLeft(node));
}

static Node* applyToCommaList(Tag tag, Node* base, Node* arguments) {
    if (!isCommaList(arguments))
        return base == NULL ? arguments : newApplication(tag, base, arguments);
    return newApplication(tag, applyToCommaList(tag, base,
        getLeft(arguments)), getRight(arguments));
}

static Node* newSpine(Node* open, Node* commaList, const char* name) {
    unsigned int length = getCommaListLength(commaList) - 1;
    syntaxErrorIf(length > 32, "too many arguments", open);
    Tag tag = newTag(newString(name, length), getTag(open).location);
    return applyToCommaList(getTag(open), newName(tag), commaList);
}

static Node* newTuple(Node* open, Node* commaList) {
    return newSpine(open, commaList, ",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,");
}

Node* reduceParentheses(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "("), "missing open for", close);
    Tag tag = getTag(open);
    if (contents == NULL)
        return newName(renameTag(tag, "()"));
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
    if (isCommaList(contents))
        return newTuple(open, contents);
    if (isApplication(contents))
        return setTag(contents, tag);
    return contents;
}

Node* reduceSquareBrackets(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "["), "missing open for", close);
    Tag tag = getTag(open);
    if (contents == NULL)
        return newNil(tag);
    if (getFixity(open) == OPENCALL) {
        syntaxErrorIf(!isCommaList(contents), "missing argument to", open);
        return newSpine(open, contents, "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[");
    }
    Node* list = newNil(tag);
    if (!isCommaList(contents))
        return prepend(tag, contents, list);
    for(; isCommaList(contents); contents = getLeft(contents))
        list = prepend(tag, getRight(contents), list);
    return prepend(tag, contents, list);
}

Node* reduceCurlyBrackets(Node* close, Node* open, Node* patterns) {
    syntaxErrorIf(patterns == NULL, "missing patterns", open);
    syntaxErrorIf(!isThisToken(open, "{"), "missing open for", close);
    return newTuple(open, patterns);
}
