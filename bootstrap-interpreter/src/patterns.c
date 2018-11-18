#include "lib/tree.h"
#include "ast.h"
#include "errors.h"

bool isCase(Node* node) {
    return isLambda(node) && isThisLexeme(node, "case");
}

unsigned int getArgumentCount(Node* application) {
    unsigned int i = 0;
    for (Node* n = application; isApplication(n); ++i)
        n = getLeft(n);
    return i;
}

Node* newProjector(Tag tag, unsigned int size, unsigned int index) {
    Node* projector = newUnderscore(tag, size - index);
    for (unsigned int i = 0; i < size; ++i)
        projector = newLambda(tag, newUnderscore(tag, 0), projector);
    return projector;
}

Node* newPatternLambda(Tag tag, Node* left, Node* right) {
    if (isValidParameter(left))
        return newLambda(tag, left, right);
    syntaxErrorIf(!isApplication(left), "invalid parameter", left);

    // example: p@(x, y) -> body  ~>  p -> (((x, y) -> body) p)
    if (isThisLexeme(left, "@"))
        return newPatternLambda(tag, getLeft(left), newApplication(tag,
            newPatternLambda(tag, getRight(left), right), getLeft(left)));

    // example: (x, y) -> body  ~>  _ -> (x -> y -> body) first(_) second(_)
    Node* body = right;
    for (Node* items = left; isApplication(items); items = getLeft(items))
        body = newPatternLambda(tag, getRight(items), body);
    for (unsigned int i = 0, size = getArgumentCount(left); i < size; ++i)
        body = newApplication(tag, body, newApplication(tag,
            newUnderscore(tag, 1), newProjector(tag, size, i)));
    return newLambda(tag, newUnderscore(tag, 0), body);
}

Node* newCasePatternLambda(Tag tag, Node* pattern, Node* body) {
    // Unit is a special case because it's the only type where you know
    // the exact form of any instance of the type, so you can actually
    // pattern match against a value instead of a variable
    pattern = isThisLeaf(pattern, "()") ? newUnderscore(tag, 0) : pattern;
    return newPatternLambda(tag, pattern, body);
}

Node* newCase(Tag tag, Node* left, Node* right) {
    // strict pattern matching
    // example: (x, y) -> B ---> (,)(x)(y) -> B ---> _ -> _ (x -> y -> B)
    Node* body = right;
    Node* items = isAsPattern(left) ? getRight(left) : left;
    for (; isApplication(items); items = getLeft(items))
        body = newCasePatternLambda(tag, getRight(items), body);
    if (isAsPattern(left))
        // example: p@(x, y) -> body  ~>  p -> (((x, y) -> body) p)
        body = newApplication(tag, newPatternLambda(tag, getLeft(left), body),
            newUnderscore(tag, 1));
    // discard left, which is now just the constructor
    Node* parameter = newUnderscore(tag, 0);
    Node* reference = newUnderscore(tag, 1);
    return newLambda(tag, parameter, newApplication(tag, reference, body));
}

static Node* mergeCaseBodies(Tag tag, Node* left, Node* right) {
    if (!isApplication(right))
        return left;
    Node* merged = mergeCaseBodies(tag, left, getLeft(right));
    return newApplication(tag, merged, getRight(right));
}

Node* combineCases(Tag tag, Node* left, Node* right) {
    // left == c1(a1) -> b1     -->   _ -> _ (a1 -> b1)
    // right == c2(a2) -> b2    -->   _ -> _ (a2 -> b2)
    // result == _ -> _ (a1 -> b1) (a2 -> b2)
    Node* body = mergeCaseBodies(tag, getBody(left), getBody(right));
    return newLambda(tag, newUnderscore(tag, 0), body);
}
