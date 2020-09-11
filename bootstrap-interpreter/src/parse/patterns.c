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
    syntaxErrorNodeIf(!isJuxtaposition(left), "invalid parameter", left);
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
    syntaxErrorNodeIf(isAsPattern(left), "invalid '@' in case", left);
    if (isUnderscore(left))
        return SimpleArrow(left, right);
    // example: (x, y) -> B ---> (,)(x)(y) -> B ---> this -> this(x -> y -> B)
    for (; isJuxtaposition(left); left = getLeft(left))
        right = newLazyArrow(getRight(left), right);
    Tag tag = getTag(left);
    Node* this = FixedName(tag, "this");
    return StrictArrow(tag, this, Juxtaposition(tag, this, right));
}

static Node* addCases(Tag tag, Node* base, Node* extension) {
    if (!isJuxtaposition(extension))
        return base;
    Node* merged = addCases(tag, base, getLeft(extension));
    return Juxtaposition(tag, merged, getRight(extension));
}

static Node* newCaseBody(Tag tag, Node* this, Node* left, Node* right) {
    // left and right are either simple arrows or strict arrows
    // left == c1(a1) -> b1     -->   this -> this(a1 -> b1)
    // right == c2(a2) -> b2    -->   this -> this(a2 -> b2)
    // result == this(a1 -> b1)(a2 -> b2)
    Node* base = isSimpleArrow(left) ?
        Juxtaposition(tag, this, getRight(left)) : getRight(left);
    return isSimpleArrow(right) ? Juxtaposition(tag, base, getRight(right)) :
        addCases(tag, base, getRight(right));
}

static bool isDefaultCase(Node* node) {
    return isSimpleArrow(node) &&
        (isUnderscore(getLeft(node)) || isThisName(getLeft(node), "this"));
}

static Node* getReconstructor(Node* arrow) {
    return isSimpleArrow(arrow) ? getRight(arrow) : getRight(getRight(arrow));
}

static Node* attachDefaultCase(Tag tag, Node* caseArrow, Node* fallback) {
    // Constructor(a) -> b; _ -> c  ~>
    //   this -> @Constructor(a -> b, _ -> c, this)
    // we wrap it in a lambda so that transformRecursion knows it's a function
    Tag constructorTag = getTag(caseArrow);
    Node* deconstructor = Name(addPrefix(constructorTag, '@'));
    Node* reconstructor = getReconstructor(caseArrow);
    Node* this = FixedName(tag, "this");
    Node* body = Juxtaposition(tag, Juxtaposition(tag, Juxtaposition(tag,
        deconstructor, reconstructor), fallback), this);
    return SimpleArrow(this, body);
}

Node* combineCases(Tag tag, Node* left, Node* right) {
    if (isDefaultCase(right))
        return attachDefaultCase(tag, left, right);
    Node* this = FixedName(tag, "this");
    return StrictArrow(tag, this, newCaseBody(tag, this, left, right));
}
