#include "lib/tree.h"
#include "errors.h"
#include "ast.h"

unsigned int getArgumentCount(Node* application) {
    unsigned int i = 0;
    for (Node* n = application; isJuxtaposition(n); ++i)
        n = getLeft(n);
    return i;
}

Node* newProjector(Tag tag, unsigned int size, unsigned int index) {
    Node* projector = Underscore(tag, size - index);
    for (unsigned int i = 0; i < size; ++i)
        projector = UnderscoreArrow(tag, projector);
    return projector;
}

Node* newPatternLambda(Tag tag, Node* left, Node* right) {
    if (isName(left))
        return Arrow(tag, left, right);

    // example: p@(x, y) -> body  ~>  p -> (((x, y) -> body) p)
    if (isAsPattern(left))
        return Arrow(tag, getLeft(left), Juxtaposition(tag,
            newPatternLambda(tag, getRight(left), right),
            Reference(getTag(getLeft(left)), 1)));

    // example: (x, y) -> body  ~>  _ -> (x -> y -> body) first(_) second(_)
    syntaxErrorIf(!isJuxtaposition(left), "invalid parameter", left);
    Node* body = right;
    for (Node* items = left; isJuxtaposition(items); items = getLeft(items))
        body = newPatternLambda(tag, getRight(items), body);
    for (unsigned int i = 0, size = getArgumentCount(left); i < size; ++i)
        body = Juxtaposition(tag, body, Juxtaposition(tag,
            Underscore(tag, 1), newProjector(tag, size, i)));
    return UnderscoreArrow(tag, body);
}

Node* newCasePatternLambda(Tag tag, Node* pattern, Node* body) {
    // Unit is a special case because it's the only type where you know
    // the exact form of any instance of the type, so you can actually
    // pattern match against a value instead of a variable
    pattern = isThisName(pattern, "()") ? Name(renameTag(tag, "_")) : pattern;
    return newPatternLambda(tag, pattern, body);
}

Node* reduceCase(Node* operator, Node* left, Node* right) {
    // strict pattern matching
    // example: (x, y) -> B ---> (,)(x)(y) -> B ---> _ -> _ (x -> y -> B)
    Tag tag = getTag(operator);
    Node* body = right;
    Node* items = isAsPattern(left) ? getRight(left) : left;
    for (; isJuxtaposition(items); items = getLeft(items))
        body = newCasePatternLambda(tag, getRight(items), body);
    if (isAsPattern(left))
        // example: p@(x, y) -> body  ~>  p -> (((x, y) -> body) p)
        body = Juxtaposition(tag, newPatternLambda(tag, getLeft(left), body),
            Underscore(tag, 1));
    // discard left, which is now just the constructor
    return Case(tag, Juxtaposition(tag, Underscore(tag, 1), body));
}

static Node* mergeCaseBodies(Tag tag, Node* left, Node* right) {
    if (!isJuxtaposition(right))
        return left;
    Node* merged = mergeCaseBodies(tag, left, getLeft(right));
    return Juxtaposition(tag, merged, getRight(right));
}

Node* combineCases(Node* left, Node* right) {
    // left == c1(a1) -> b1     -->   _ -> _ (a1 -> b1)
    // right == c2(a2) -> b2    -->   _ -> _ (a2 -> b2)
    // result == _ -> _ (a1 -> b1) (a2 -> b2)
    Tag tag = renameTag(getTag(left), "_");
    return Case(tag, mergeCaseBodies(tag, getRight(left), getRight(right)));
}
