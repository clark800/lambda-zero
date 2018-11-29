#include <string.h>
#include "shared/lib/tree.h"
#include "shared/lib/stack.h"
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
    return Arrow(tag, Name(renameTag(tag, "*.")), body);
}

static Node* wrapRightSection(Tag tag, Node* body) {
    return Arrow(tag, Name(renameTag(tag, ".*")), body);
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
        return ADT(renameTag(getTag(open), "{}"), VOID);
    syntaxErrorIf(isSection(patterns), "invalid section", patterns);
    return ADT(getTag(open), newTuple(open, patterns));
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

void pushBracket(Stack* stack, Node* open, Node* close, Node* contents) {
    if (isEOF(open) || isOperator(peek(stack, 0))) {
        push(stack, reduceBracket(open, close, NULL, contents));
    } else {
        Hold* left = pop(stack);
        push(stack, reduceBracket(open, close, getNode(left), contents));
        release(left);
    }
}

void shiftOpen(Stack* stack, Node* open) {
    reduceLeft(stack, open);
    push(stack, open);
}

void shiftClose(Stack* stack, Node* close) {
    erase(stack, " ");
    erase(stack, "\n");
    erase(stack, ";");

    Node* top = peek(stack, 0);
    if (isOperator(top) && !isSpecial(top) && !isEOF(close)) {
        if (isLeftPlaceholder(peek(stack, 1))) {
            // bracketed infix operator
            Hold* op = pop(stack);
            release(pop(stack));
            push(stack, Name(getTag(getNode(op))));
            release(op);
        } else if (isOpenOperator(peek(stack, 1))) {
            // bracketed prefix operator
            Hold* op = pop(stack);
            Tag tag = getTag(getNode(op));
            if (isThisOperator(getNode(op), "(+)"))
                tag = renameTag(tag, "+");
            else if (isThisOperator(getNode(op), "(-)"))
                tag = renameTag(tag, "-");
            push(stack, Name(tag));
            release(op);
        } else if (getFixity(top) == INFIX || getFixity(top) == PREFIX)
            push(stack, RightPlaceholder(getTag(top)));
    }

    reduceLeft(stack, close);
    syntaxErrorIf(isEOF(peek(stack, 0)), "missing open for", close);
    Hold* contents = pop(stack);
    if (isOpenOperator(getNode(contents))) {
        pushBracket(stack, getNode(contents), close, NULL);
    } else {
        Hold* open = pop(stack);
        if (isEOF(getNode(open)) && !isEOF(close))
            syntaxError("missing open for", close);
        if (isOperator(getNode(contents)))
            syntaxError("missing right operand for", getNode(contents));
        pushBracket(stack, getNode(open), close, getNode(contents));
        release(open);
    }
    release(contents);
}
