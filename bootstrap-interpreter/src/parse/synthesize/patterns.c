#include "shared/lib/tree.h"
#include "parse/shared/errors.h"
#include "parse/shared/ast.h"

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

Node* newLazyArrow(Tag tag, Node* left, Node* right) {
    if (isName(left))
        return LockedArrow(tag, left, right);

    // example: p@(x, y) -> body  ~>  p -> (((x, y) -> body) p)
    if (isAsPattern(left))
        return LockedArrow(tag, getLeft(left), Juxtaposition(tag,
            newLazyArrow(tag, getRight(left), right),
            Reference(getTag(getLeft(left)), 1, 0)));

    // example: (x, y) -> body  ~>  _ -> (x -> y -> body) first(_) second(_)
    syntaxErrorIf(!isJuxtaposition(left), "invalid parameter", left);
    Node* body = right;
    for (Node* items = left; isJuxtaposition(items); items = getLeft(items))
        body = newLazyArrow(tag, getRight(items), body);
    for (unsigned int i = 0, size = getArgumentCount(left); i < size; ++i)
        body = Juxtaposition(tag, body, Juxtaposition(tag,
            Underscore(tag, 1), newProjector(tag, size, i)));
    return UnderscoreArrow(tag, body);
}

Node* newCaseArrow(Tag tag, Node* left, Node* right) {
    // example: (x, y) -> B ---> (,)(x)(y) -> B ---> _ -> _ (x -> y -> B)
    Node* body = right;
    Node* items = isAsPattern(left) ? getRight(left) : left;
    for (; isJuxtaposition(items); items = getLeft(items))
        body = newLazyArrow(tag, getRight(items), body);
    if (isAsPattern(left))
        // example: p@(x, y) -> body  ~>  p -> (((x, y) -> body) p)
        body = Juxtaposition(tag, newLazyArrow(tag, getLeft(left), body),
            Underscore(tag, 1));
    // discard left, which is now just the constructor
    body = Juxtaposition(tag, Underscore(tag, 1), body);
    return StrictArrow(tag, FixedName(tag, "_"), body);
}

static Node* addCases(Tag tag, Node* base, Node* extension) {
    if (!isJuxtaposition(extension))
        return base;
    Node* merged = addCases(tag, base, getLeft(extension));
    return Juxtaposition(tag, merged, getRight(extension));
}

static Node* newCaseBody(Tag tag, Node* left, Node* right) {
    // left and right are either simple arrows or strict arrows
    // left == c1(a1) -> b1     -->   _ -> _ (a1 -> b1)
    // right == c2(a2) -> b2    -->   _ -> _ (a2 -> b2)
    // result == _ -> _ (a1 -> b1) (a2 -> b2)
    Node* base = isSimpleArrow(left) ?
        Juxtaposition(tag, Underscore(tag, 1), getRight(left)) : getRight(left);
    return isSimpleArrow(right) ? Juxtaposition(tag, base, getRight(right)) :
        addCases(tag, base, getRight(right));
}

Node* combineCases(Tag tag, Node* left, Node* right) {
    return StrictArrow(tag, FixedName(tag, "_"), newCaseBody(tag, left, right));
}
