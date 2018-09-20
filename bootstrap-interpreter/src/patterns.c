#include "lib/tree.h"
#include "ast.h"
#include "errors.h"

unsigned int getArgumentCount(Node* application) {
    unsigned int i = 0;
    for (Node* n = application; isApplication(n); ++i)
        n = getLeft(n);
    return i;
}

static Node* newProjection(Tag tag, unsigned int size, unsigned int index) {
    Node* projection = newBlankReference(tag, size - index);
    for (unsigned int i = 0; i < size; ++i)
        projection = newLambda(tag, newBlank(tag), projection);
    return projection;
}

Node* reduceLambda(Node* operator, Node* left, Node* right) {
    // lazy pattern matching
    Tag tag = getTag(operator);
    if (isSymbol(left))
        return newLambda(tag, left, right);
    syntaxErrorIf(!isApplication(left), "invalid parameter", left);
    // example: (x, y) -> body ---> _ -> (x -> y -> body) first(_) second(_)
    Node* body = right;
    for (Node* items = left; isApplication(items); items = getLeft(items))
        body = reduceLambda(operator, getRight(items), body);
    for (unsigned int i = 0, size = getArgumentCount(left); i < size; ++i)
        body = newApplication(tag, body,
            newApplication(tag, newBlankReference(tag, 1),
                newProjection(tag, size, i)));
    return newLambda(tag, newBlank(tag), body);
}

/*
Node* newStrictDestructuringLambda(Node* operator, Node* left, Node* right) {
    Tag tag = getTag(operator);
    if (isSymbol(left))
        return newLambda(tag, left, right);
    syntaxErrorIf(!isApplication(left), "invalid parameter", left);
    // example: (x, y) -> body ---> _ -> _ (x -> y -> body)
    for (; isApplication(left); left = getLeft(left))
        right = newStrictDestructuringLambda(operator, getRight(left), right);
    // discard left, which is the value constructor
    return newLambda(tag, newBlank(tag),
        newApplication(tag, newBlankReference(tag, 1), right));
}
*/

Node* getHead(Node* node) {
    for (; isApplication(node); node = getLeft(node));
    return node;
}

static bool isStrictPatternLambda(Node* lambda) {
    return isBlank(getParameter(lambda)) && isBlank(getHead(getBody(lambda)));
}

static bool isLazyPatternLambda(Node* lambda) {
    return isBlank(getParameter(lambda)) && isApplication(getBody(lambda)) &&
        isApplication(getRight(getBody(lambda))) &&
        isBlank(getLeft(getRight(getBody(lambda))));
}

static Node* getPatternExtension(Node* lambda) {
    if (isStrictPatternLambda(lambda))
        return getRight(getBody(lambda));
    if (isLazyPatternLambda(lambda))
        return getHead(getBody(lambda));
    return getBody(lambda);
}

Node* reducePatternLambda(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(!isLambda(left), "expected lambda to left of", operator);
    syntaxErrorIf(!isLambda(right), "expected lambda to right of", operator);
    Tag tag = getTag(operator);
    Node* base = isStrictPatternLambda(left) ? getBody(left) : newApplication(
        tag, newBlankReference(tag, 1), getPatternExtension(left));
    return newLambda(tag, newBlank(tag),
        newApplication(tag, base, getPatternExtension(right)));
}
