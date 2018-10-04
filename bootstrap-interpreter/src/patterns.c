#include "lib/tree.h"
#include "ast.h"
#include "errors.h"

unsigned int getArgumentCount(Node* application) {
    unsigned int i = 0;
    for (Node* n = application; isApplication(n); ++i)
        n = getLeft(n);
    return i;
}

Node* newProjector(Tag tag, unsigned int size, unsigned int index) {
    Node* projector = newBlankReference(tag, size - index);
    for (unsigned int i = 0; i < size; ++i)
        projector = newLambda(tag, newBlank(tag), projector);
    return projector;
}

Node* reduceLambda(Node* operator, Node* left, Node* right) {
    // lazy pattern matching
    Tag tag = getTag(operator);
    if (isValidParameter(left))
        return newLambda(tag, left, right);
    syntaxErrorIf(!isApplication(left), "invalid parameter", left);

    // example: p@(x, y) -> body  ~>  p -> (((x, y) -> body) p)
    if (isThisLexeme(left, "@")) {
        if (!isSymbol(getLeft(left)) || isUnused(getLeft(left)))
           syntaxError("invalid left operand to", left);
        return newLambda(getTag(left), getLeft(left), newApplication(tag,
            reduceLambda(operator, getRight(left), right), getLeft(left)));
    }

    // example: (x, y) -> body  ~>  _ -> (x -> y -> body) first(_) second(_)
    Node* body = right;
    for (Node* items = left; isApplication(items); items = getLeft(items))
        body = reduceLambda(operator, getRight(items), body);
    for (unsigned int i = 0, size = getArgumentCount(left); i < size; ++i)
        body = newApplication(tag, body, newApplication(tag,
            newBlankReference(tag, 1), newProjector(tag, size, i)));
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
    if (isThisLexeme(lambda, "@")) {
        Tag tag = getTag(lambda);
        Node* extension = getPatternExtension(getLeft(getBody(lambda)));
        return newApplication(tag, newLambda(tag,
            getParameter(lambda), extension), newBlankReference(tag, 1));
    }
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
    // example: z -> 0; up(n) -> n  ~>  _ -> _ 0 (n -> n)
    // example: z -> 0; n' @ up(n) -> n  ~>
    //          z -> 0; (n' -> ((up(n) -> n) n'))  ~>
    //          _ -> _ 0 ((n' -> (n -> n)) _)
    Node* base = isStrictPatternLambda(left) ? getBody(left) : newApplication(
        tag, newBlankReference(tag, 1), getPatternExtension(left));
    return newLambda(tag, newBlank(tag),
        newApplication(tag, base, getPatternExtension(right)));
}
