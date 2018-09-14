#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "errors.h"
#include "operators.h"

static bool isCommaList(Node* node) {
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
        if (isSpecialOperator(contents) && !isComma(contents))
            syntaxError("invalid operator in parentheses", contents);
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

Node* reduceEOF(Node* operator, Node* open, Node* contents) {
    (void)operator;
    syntaxErrorIf(!isEOF(open), "missing close for", open);
    syntaxErrorIf(isEOF(contents), "no input", open);
    syntaxErrorIf(isCommaList(contents), "comma not inside brackets", contents);
    return contents;
}

Node* reduceUnmatched(Node* operator, Node* left, Node* right) {
    syntaxError("missing close for", operator);
    return left == NULL ? right : left; // suppress unused parameter warning
}

static Node* newSection(Node* operator, Node* left, Node* right) {
    if (getFixity(operator) != IN || isSpecialOperator(operator))
        syntaxError("invalid operator in section", operator);
    Node* body = reduceOperator(operator, left, right);
    return newLambda(getTag(operator), newBlank(getTag(operator)), body);
}

static Node* constructSection(Node* left, Node* right) {
    if (isOperator(left) == isOperator(right))   // e.g. right is prefix
        syntaxError("invalid operator in section", right);
    return isOperator(left) ?
        newSection(left, newBlankReference(getTag(left), 1), right) :
        newSection(right, left, newBlankReference(getTag(right), 1));
}

static Node* reduceSection(Stack* stack, Node* right) {
    Hold* left = pop(stack);
    Node* result = constructSection(getNode(left), right);
    release(left);
    return result;
}

static bool isOpenOperator(Node* token) {
    return isOperator(token) &&
        (getFixity(token) == OPEN || getFixity(token) == OPENCALL);
}

void shiftBracket(Stack* stack, Node* close) {
    syntaxErrorIf(isEOF(peek(stack, 0)), "missing open for", close);
    Hold* contents = pop(stack);
    if (isOpenOperator(getNode(contents))) {
        push(stack, reduceOperator(close, getNode(contents), NULL));
    } else {
        if (!isEOF(close) && isEOF(peek(stack, 0)))
            syntaxError("missing open for", close);
        Node* right = getNode(contents);
        if (isCloseParen(close) && !isOpenParen(peek(stack, 0)))
            contents = replaceHold(contents, hold(reduceSection(stack, right)));
        if (!isCloseParen(close) && isOperator(right))
            syntaxError("missing right argument to", right);
        Hold* open = pop(stack);
        push(stack,
            reduceOperator(close, getNode(open), getNode(contents)));
        release(open);
    }
    release(contents);
}

void shiftOpenCall(Stack* stack, Node* operator) {
    Hold* top = pop(stack);
    push(stack, operator);
    push(stack, getNode(top));
    push(stack, parseOperator(renameTag(getTag(operator), ","), stack));
    release(top);
}

void shiftOpenCurly(Stack* stack, Node* operator) {
    if (!isThisToken(peek(stack, 0), "::="))
        syntaxError("must appear on the right side of '::='", operator);
    push(stack, operator);
}
