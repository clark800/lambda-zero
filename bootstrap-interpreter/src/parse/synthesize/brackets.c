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

Node* reduceOpenParenthesis(Node* open, Node* before, Node* contents) {
    Tag tag = getTag(open);
    if (contents == NULL) {
        Node* unit = FixedName(tag, "()");
        return before == NULL ? unit : Juxtaposition(tag, before, unit);
    }
    if (isDefinition(contents))
        syntaxError("missing scope for definition", contents);
    if (isSection(contents))
        contents = wrapSection(tag, contents);
    if (before != NULL)
        return applyToCommaList(tag, before, contents);
    if (isCommaPair(contents))
        return newTuple(open, contents);
    if (isArrow(contents))
        setVariety(contents, LOCKEDARROW);
    if (isJuxtaposition(contents))
        setTag(contents, tag);
    return contents;
}

Node* reduceOpenSquareBracket(Node* open, Node* before, Node* contents) {
    Tag tag = getTag(open);
    if (contents == NULL) {
        syntaxErrorIf(before != NULL, "missing argument to", open);
        return Nil(tag);
    }
    syntaxErrorIf(isSection(contents), "invalid section", contents);
    if (before != NULL) {
        const char* lexeme = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[";
        Node* name = newSpineName(open, lexeme, getCommaListLength(contents));
        Node* base = Juxtaposition(tag, name, before);
        return applyToCommaList(tag, base, contents);
    }
    Node* list = Nil(tag);
    if (!isCommaPair(contents))
        return prepend(tag, contents, list);
    for(; isCommaPair(contents); contents = getLeft(contents))
        list = prepend(tag, getRight(contents), list);
    return prepend(tag, contents, list);
}

Node* reduceOpenBrace(Node* open, Node* before, Node* patterns) {
    syntaxErrorIf(before != NULL, "invalid operand before", open);
    if (patterns == NULL)
        return SetBuilder(renameTag(getTag(open), "{}"), VOID);
    syntaxErrorIf(isSection(patterns), "invalid section", patterns);
    return SetBuilder(getTag(open), newTuple(open, patterns));
}

Node* reduceOpenFile(Node* open, Node* before, Node* contents) {
    syntaxErrorIf(before != NULL, "invalid operand before", open);
    syntaxErrorIf(!isEOF(open), "missing close for", open);
    syntaxErrorIf(isCommaPair(contents), "comma not inside brackets", contents);
    return contents;
}
