#include <string.h>
#include "tree.h"
#include "opp/errors.h"
#include "opp/operator.h"
#include "ast.h"
#include "brackets.h"

Node* Nil(Tag tag) {return FixedName(tag, "[]");}

Node* prepend(Tag tag, Node* item, Node* list) {
    return Juxtaposition(tag, Juxtaposition(tag,
        Name(setTagFixity(renameTag(tag, "::"), INFIX)), item), list);
}

static unsigned int getCommaListLength(Node* node) {
    return !isCommaPair(node) ? 1 : 1 + getCommaListLength(getLeft(node));
}

static Node* applyToCommaList(Tag tag, Node* base, Node* arguments) {
    if (!isCommaPair(arguments))
        return Juxtaposition(tag, base, arguments);
    return Juxtaposition(tag, applyToCommaList(tag, base,
        getLeft(arguments)), getRight(arguments));
}

static Node* newSpineName(Tag tag, const char* name, unsigned int length) {
    syntaxErrorIf(length > strlen(name), "too many arguments", tag);
    String lexeme = newString(name, (unsigned char)length);
    return Name(newTag(lexeme, tag.location));
}

static Node* newTuple(Tag tag, Node* commaList) {
    const char* lexeme = ",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,";
    Node* name = newSpineName(tag, lexeme, getCommaListLength(commaList) - 1);
    return applyToCommaList(tag, name, commaList);
}

Node* reduceOpenParenthesis(Tag tag, Node* before, Node* contents) {
    if (contents == NULL) {
        syntaxErrorIf(before != NULL, "missing argument to", tag);
        return FixedName(tag, "()");
    }
    if (isDefinition(contents))
        syntaxErrorNode("missing scope for definition", contents);
    if (before != NULL)
        return applyToCommaList(tag, before, contents);
    if (isCommaPair(contents))
        return newTuple(tag, contents);
    if (isArrow(contents))
        setVariety(contents, SINGLE);
    if (isJuxtaposition(contents))
        setTag(contents, tag);
    return contents;
}

Node* reduceOpenSquareBracket(Tag tag, Node* before, Node* contents) {
    if (contents == NULL) {
        syntaxErrorIf(before != NULL, "missing argument to", tag);
        return Nil(tag);
    }
    if (before != NULL) {
        const char* lexeme = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[";
        Node* name = newSpineName(tag, lexeme, getCommaListLength(contents));
        Node* base = Juxtaposition(tag, name, before);
        return applyToCommaList(tag, base, contents);
    }
    Node* list = Nil(tag);
    if (!isCommaPair(contents))
        return prepend(tag, contents, list);
    for (; isCommaPair(contents); contents = getLeft(contents))
        list = prepend(tag, getRight(contents), list);
    return prepend(tag, contents, list);
}

Node* reduceOpenBrace(Tag tag, Node* before, Node* patterns) {
    syntaxErrorIf(before != NULL, "invalid operand before", tag);
    if (patterns == NULL)
        return SetBuilder(setTagFixity(renameTag(tag, "{}"), NOFIX), VOID);
    return SetBuilder(tag, newTuple(tag, patterns));
}

Node* reduceOpenFile(Tag tag, Node* before, Node* contents) {
    syntaxErrorIf(before != NULL, "invalid operand before", tag);
    return contents;
}
