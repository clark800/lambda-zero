#include <string.h>
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
        return newApplication(tag, base, arguments);
    return newApplication(tag, applyToCommaList(tag, base,
        getLeft(arguments)), getRight(arguments));
}

static Node* newSpineName(Node* node, const char* name, unsigned int length) {
    unsigned int maxLength = (unsigned int)strlen(name);
    syntaxErrorIf(length > maxLength, "too many arguments", node);
    return newName(newTag(newString(name, length), getTag(node).location));
}

static Node* newTuple(Node* open, Node* commaList) {
    const char* lexeme = ",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,";
    Node* name = newSpineName(open, lexeme, getCommaListLength(commaList) - 1);
    return applyToCommaList(getTag(open), name, commaList);
}

Node* reduceParentheses(Node* open, Node* function, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "("), "missing close for", open);
    Tag tag = getTag(open);
    if (contents == NULL) {
        Node* unit = newName(renameTag(tag, "()"));
        return function == NULL ? unit : newApplication(tag, function, unit);
    }
    if (isOperator(contents)) {
        if (isSpecialOperator(contents) && !isComma(contents))
            syntaxError("invalid operator in parentheses", contents);
        contents = convertOperator(contents);
    }
    if (function != NULL)
        return applyToCommaList(tag, function, contents);
    if (isCommaList(contents))
        return newTuple(open, contents);
    if (isApplication(contents))
        return setTag(contents, tag);
    return contents;
}

Node* reduceSquareBrackets(Node* open, Node* left, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "["), "missing close for", open);
    Tag tag = getTag(open);
    if (contents == NULL) {
        syntaxErrorIf(left != NULL, "missing argument to", open);
        return newNil(tag);
    }
    if (left != NULL) {
        const char* lexeme = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[";
        Node* name = newSpineName(open, lexeme, getCommaListLength(contents));
        return applyToCommaList(tag, newApplication(tag, name, left), contents);
    }
    Node* list = newNil(tag);
    if (!isCommaList(contents))
        return prepend(tag, contents, list);
    for(; isCommaList(contents); contents = getLeft(contents))
        list = prepend(tag, getRight(contents), list);
    return prepend(tag, contents, list);
}

Node* reduceCurlyBrackets(Node* open, Node* left, Node* patterns) {
    (void)left;
    syntaxErrorIf(!isThisToken(open, "{"), "missing close for", open);
    syntaxErrorIf(patterns == NULL, "missing patterns", open);
    return newTuple(open, patterns);
}

Node* reduceEOF(Node* open, Node* left, Node* contents) {
    (void)left;
    syntaxErrorIf(!isEOF(open), "missing close for", open);
    syntaxErrorIf(isEOF(contents), "no input", open);
    syntaxErrorIf(isCommaList(contents), "comma not inside brackets", contents);
    return contents;
}

Node* reduceUnmatched(Node* open, Node* left, Node* right) {
    syntaxError("missing close for", open);
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

void shiftBracket(Stack* stack, Node* close) {
    syntaxErrorIf(isEOF(peek(stack, 0)), "missing open for", close);
    Hold* top = pop(stack);
    if (isOperator(getNode(top)) && getFixity(getNode(top)) == OPENCALL) {
        Hold* left = pop(stack);
        push(stack, reduceBracket(getNode(top), close, getNode(left), NULL));
        release(left);
    } else if (isOperator(getNode(top)) && getFixity(getNode(top)) == OPEN) {
        push(stack, reduceBracket(getNode(top), close, NULL, NULL));
    } else {
        Node* right = getNode(top);
        if (!isEOF(close) && isEOF(peek(stack, 0)))
            syntaxError("missing open for", close);
        if (isCloseParen(close) && !isOpenParen(peek(stack, 0)))
            right = reduceSection(stack, right);
        if (!isCloseParen(close) && isOperator(right))
            syntaxError("missing right argument to", right);
        Hold* open = pop(stack);
        Node* op = getNode(open);
        if (isOperator(op) && getFixity(op) == OPENCALL) {
            Hold* left = pop(stack);
            push(stack, reduceBracket(op, close, getNode(left), right));
            release(left);
        } else if (isOperator(op) && (getFixity(op) == OPEN || isEOF(op))) {
            push(stack, reduceBracket(op, close, NULL, right));
        } else {
           syntaxError("missing open for", close);
        }
        release(open);
    }
    release(top);
}

void shiftOpenCurly(Stack* stack, Node* operator) {
    if (!isThisToken(peek(stack, 0), "::="))
        syntaxError("must appear on the right side of '::='", operator);
    push(stack, operator);
}
