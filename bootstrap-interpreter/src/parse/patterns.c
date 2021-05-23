#include "tree.h"
#include "opp/errors.h"
#include "ast.h"
#include "patterns.h"

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

Node* newArrow(Node* left, Node* right) {
    if (isColonPair(left))
        return newArrow(getLeft(left), right);
    if (isName(left))
        return SingleArrow(left, right);

    // example: p@(x, y) -> body  ~>  p -> (((x, y) -> body) p)
    if (isAsPattern(left))
        return newArrow(getLeft(left), Juxtaposition(getTag(left),
            newArrow(getRight(left), right), Underscore(getTag(left), 1)));

    // example: (x, y) -> body  ~>  _ -> (x -> y -> body) first(_) second(_)
    syntaxErrorNodeIf(!isJuxtaposition(left), "invalid parameter", left);
    Node* node = left;
    Node* body = right;
    for (; isJuxtaposition(node); node = getLeft(node))
        body = newArrow(getRight(node), body);
    Tag tag = getTag(node);
    for (unsigned int i = 0, size = getArgumentCount(left); i < size; ++i)
        body = Juxtaposition(tag, body, Juxtaposition(tag,
            Underscore(tag, 1), newProjector(tag, size, i)));
    return UnderscoreArrow(tag, body);
}

Node* newCase(Node* left, Node* right) {
    if (isUnderscore(left))
        return DefaultCaseArrow(FixedName(getTag(left), "this"), right);

    // example: (x, y) -> B ---> (,)(x)(y) -> B ---> this -> this(x -> y -> B)
    for (; isJuxtaposition(left); left = getLeft(left))
        right = newArrow(getRight(left), right);
    syntaxErrorNodeIf(!isName(left) && !isNumber(left), "invalid case", left);

    Tag tag = getTag(left);
    Node* this = FixedName(tag, "this");
    return ExplicitCaseArrow(tag, this, Juxtaposition(tag, this, right));
}

static Node* attachDefaultCase(Tag tag, Node* caseArrow, Node* fallback) {
    // Constructor(a) -> b; _ -> c  ~>
    //   this -> @Constructor(a -> b, _ -> c, this)
    // we wrap it in a lambda so that transformRecursion knows it's a function
    Tag constructorTag = getTag(caseArrow);
    Node* deconstructor = Name(addPrefix(constructorTag, '@'));
    Node* reconstructor = getRight(getRight(caseArrow));
    Node* this = FixedName(tag, "this");
    Node* body = Juxtaposition(tag, Juxtaposition(tag, Juxtaposition(tag,
        deconstructor, reconstructor), fallback), this);
    return DefaultCaseArrow(this, body);
}

static Node* combineCaseBodies(Tag tag, Node* base, Node* extension) {
    if (!isJuxtaposition(extension))
        return base;
    Node* merged = combineCaseBodies(tag, base, getLeft(extension));
    return Juxtaposition(tag, merged, getRight(extension));
}

Node* combineCases(Tag tag, Node* left, Node* right) {
    if (isDefaultCase(left))
        syntaxError("invalid default case position", getTag(left));
    if (isDefaultCase(right))
        return attachDefaultCase(tag, left, right);
    Node* this = FixedName(tag, "this");
    Node* body = combineCaseBodies(tag, getRight(left), getRight(right));
    return ExplicitCaseArrow(tag, this, body);
}
