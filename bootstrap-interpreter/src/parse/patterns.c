#include "tree.h"
#include "opp/errors.h"
#include "ast.h"

bool isValidPattern(Node* node) {
    return isName(node) ||
        (isColonPair(node) && isValidPattern(getLeft(node))) ||
        (isJuxtaposition(node) &&
        isValidPattern(getLeft(node)) && isValidPattern(getRight(node)));
}

unsigned int getArgumentCount(Node* application) {
    unsigned int i = 0;
    for (Node* n = application; isJuxtaposition(n); ++i)
        n = getLeft(n);
    return i;
}

static Node* newProjector(Tag tag, unsigned int size, unsigned int index) {
    Node* projector = Underscore(tag, size - index);
    for (unsigned int i = 0; i < size; ++i)
        projector = UnderscoreArrow(tag, projector);
    return projector;
}

Node* newLazyArrow(Node* left, Node* right) {
    if (isColonPair(left))
        return newLazyArrow(getLeft(left), right);
    if (isName(left))
        return LockedArrow(left, right);

    // example: p@(x, y) -> body  ~>  p -> (((x, y) -> body) p)
    if (isAsPattern(left))
        return newLazyArrow(getLeft(left), Juxtaposition(getTag(left),
            newLazyArrow(getRight(left), right), Underscore(getTag(left), 1)));

    // example: (x, y) -> body  ~>  _ -> (x -> y -> body) first(_) second(_)
    syntaxErrorIf(!isJuxtaposition(left), "invalid parameter", left);
    Node* node = left;
    Node* body = right;
    for (; isJuxtaposition(node); node = getLeft(node))
        body = newLazyArrow(getRight(node), body);
    Tag tag = getTag(node);
    for (unsigned int i = 0, size = getArgumentCount(left); i < size; ++i)
        body = Juxtaposition(tag, body, Juxtaposition(tag,
            Underscore(tag, 1), newProjector(tag, size, i)));
    return UnderscoreArrow(tag, body);
}

Node* newCaseArrow(Node* left, Node* right) {
    if (isUnderscore(left))
        return SimpleArrow(left, right);
    // example: (x, y) -> B ---> (,)(x)(y) -> B ---> _ -> _ (x -> y -> B)
    Node* body = right;
    Node* items = isAsPattern(left) ? getRight(left) : left;
    for (; isJuxtaposition(items); items = getLeft(items))
        body = newLazyArrow(getRight(items), body);
    Node* constructor = items;
    Tag tag = getTag(constructor);
    if (isAsPattern(left))
        // example: p@(x, y) -> body  ~>  p -> (((x, y) -> body) p)
        body = Juxtaposition(tag, newLazyArrow(getLeft(left), body),
            Underscore(tag, 1));
    body = Juxtaposition(tag, Underscore(tag, 1), body);
    return StrictArrow(tag, body);
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

static bool isDefaultCase(Node* node) {
    return isSimpleArrow(node) && isUnderscore(getLeft(node));
}

static Node* getReconstructor(Node* arrow) {
    return isSimpleArrow(arrow) ? getRight(arrow) : getRight(getRight(arrow));
}

static Node* chainCases(Tag tag, Node* caseArrow, Node* fallback) {
    // Constructor(a) -> b; _ -> c  ~>  _ -> @Constructor(a -> b, _ -> c, _#1)
    // we wrap it in a lambda so that transformRecursion knows it's a function
    Node* deconstructor = Name(addPrefix(getTag(caseArrow), '@'));
    Node* reconstructor = getReconstructor(caseArrow);
    Node* body = Juxtaposition(tag, Juxtaposition(tag, Juxtaposition(tag,
        deconstructor, reconstructor), fallback), Underscore(tag, 1));
    return SimpleArrow(FixedName(tag, "_"), body);
}

Node* combineCases(Tag tag, Node* left, Node* right) {
    return isDefaultCase(right) ? chainCases(tag, left, right) :
        StrictArrow(tag, newCaseBody(tag, left, right));
}
