#include <string.h>
#include "shared/lib/tree.h"
#include "parse/shared/errors.h"
#include "parse/shared/ast.h"
#include "symbols.h"

static unsigned int getCommaListLength(Node* node) {
    return !isCommaPair(node) ? 1 : 1 + getCommaListLength(getLeft(node));
}

static Node* applyToCommaList(Tag tag, Node* base, Node* arguments) {
    if (!isCommaPair(arguments))
        return Juxtaposition(tag, base, arguments);
    return Juxtaposition(tag, applyToCommaList(tag, base,
        getLeft(arguments)), getRight(arguments));
}

static Node* newSpineName(Node* node, const char* name, unsigned int length) {
    unsigned int maxLength = (unsigned int)strlen(name);
    syntaxErrorIf(length > maxLength, "too many arguments", node);
    return Name(newTag(newString(name, length), getTag(node).location));
}

static Node* newTuple(Node* open, Node* commaList) {
    const char* lexeme = ",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,";
    Node* name = newSpineName(open, lexeme, getCommaListLength(commaList) - 1);
    return applyToCommaList(getTag(open), name, commaList);
}

static Node* wrapLeftSection(Tag tag, Node* body) {
    return LockedArrow(FixedName(tag, "*."), body);
}

static Node* wrapRightSection(Tag tag, Node* body) {
    return LockedArrow(FixedName(tag, ".*"), body);
}

static Node* wrapSection(Tag tag, Node* section) {
    Node* body = getSectionBody(section);
    switch ((SectionVariety)getVariety(section)) {
        case LEFTSECTION:
            return wrapLeftSection(tag, body);
        case RIGHTSECTION:
            if (isName(getLeft(body)))
                return getLeft(body);   // parenthesized postfix operator
            return wrapRightSection(tag, body);
        case LEFTRIGHTSECTION:
            return wrapLeftSection(tag, wrapRightSection(tag, body));
    }
    assert(false);
    return NULL;
}

Node* reduceParentheses(Node* open, Node* function, Node* contents) {
    syntaxErrorIf(!isThisOperator(open, "("), "missing close for", open);
    Tag tag = getTag(open);
    if (contents == NULL) {
        Node* unit = FixedName(tag, "()");
        return function == NULL ? unit : Juxtaposition(tag, function, unit);
    }
    if (isDefinition(contents))
        syntaxError("missing scope for definition", contents);
    if (isSection(contents))
        contents = wrapSection(tag, contents);
    if (function != NULL)
        return applyToCommaList(tag, function, contents);
    if (isCommaPair(contents))
        return newTuple(open, contents);
    if (isArrow(contents))
        setVariety(contents, LOCKEDARROW);
    if (isJuxtaposition(contents))
        setTag(contents, tag);
    return contents;
}

Node* reduceSquareBrackets(Node* open, Node* left, Node* contents) {
    syntaxErrorIf(!isThisOperator(open, "["), "missing close for", open);
    Tag tag = getTag(open);
    if (contents == NULL) {
        syntaxErrorIf(left != NULL, "missing argument to", open);
        return Nil(tag);
    }
    syntaxErrorIf(isSection(contents), "invalid section", contents);
    if (left != NULL) {
        const char* lexeme = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[";
        Node* name = newSpineName(open, lexeme, getCommaListLength(contents));
        return applyToCommaList(tag, Juxtaposition(tag, name, left), contents);
    }
    Node* list = Nil(tag);
    if (!isCommaPair(contents))
        return prepend(tag, contents, list);
    for(; isCommaPair(contents); contents = getLeft(contents))
        list = prepend(tag, getRight(contents), list);
    return prepend(tag, contents, list);
}

Node* reduceCurlyBrackets(Node* open, Node* left, Node* patterns) {
    syntaxErrorIf(left != NULL, "missing space before", open);
    syntaxErrorIf(!isThisOperator(open, "{"), "missing close for", open);
    if (patterns == NULL)
        return SetBuilder(renameTag(getTag(open), "{}"), VOID);
    syntaxErrorIf(isSection(patterns), "invalid section", patterns);
    return SetBuilder(getTag(open), newTuple(open, patterns));
}

Node* reduceEOF(Node* open, Node* left, Node* contents) {
    syntaxErrorIf(left != NULL, "invalid syntax", open);  // should never happen
    syntaxErrorIf(!isEOF(open), "missing close for", open);
    syntaxErrorIf(isCommaPair(contents), "comma not inside brackets", contents);
    return contents;
}

Node* reduceUnmatched(Node* open, Node* left, Node* right) {
    (void)left, (void)right;
    syntaxError("missing close for", open);
    return NULL;
}
