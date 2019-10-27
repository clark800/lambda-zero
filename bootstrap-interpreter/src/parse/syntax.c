#include "tree.h"
#include "opp/errors.h"
#include "opp/opp.h"
#include "ast.h"
#include "patterns.h"
#include "define.h"
#include "brackets.h"

static Node* reduceArrow(Node* operator, Node* left, Node* right) {
    if (isColonPair(right))
        return reduceArrow(operator, left, getLeft(right));
    if (isName(left))
        return SimpleArrow(left, right);
    if (isColonPair(left))
        return newLazyArrow(left, right);
    return newCaseArrow(left, right);
}

static Node* reducePipeline(Node* operator, Node* left, Node* right) {
    return reduceApply(operator, right, left);
}

static Node* reduceCommaPair(Node* operator, Node* left, Node* right) {
    return CommaPair(getTag(operator), left, right);
}

static Node* reduceColonPair(Node* operator, Node* left, Node* right) {
    if (isColonPair(left) || !isValidPattern(left))
        syntaxError("invalid left side of colon", left);
    if (isColonPair(right) || !isValidPattern(right))
        syntaxError("invalid right side of colon", right);
    return ColonPair(getTag(operator), left, right);
}

static Node* reduceAsPattern(Node* operator, Node* left, Node* right) {
    return AsPattern(getTag(operator), left, right);
}

static Node* reduceWhere(Node* operator, Node* left, Node* right) {
    if (!isDefinition(right))
        syntaxError("expected definition to right of", operator);
    return applyDefinition(right, left);
}

static Node* reduceWith(Node* operator, Node* asPattern, Node* withBlock) {
    if (!isAsPattern(asPattern))
        syntaxError("expected as pattern to right of", operator);
    Tag tag = getTag(operator);
    Node* expression = getLeft(asPattern);
    Node* pattern = getRight(asPattern);
    Node* elseBlock = Underscore(tag, 3);
    Node* caseArrow = newCaseArrow(pattern, withBlock);
    Node* fallback = SimpleArrow(Underscore(tag, 0), elseBlock);
    Node* function = combineCases(tag, caseArrow, fallback);
    release(hold(caseArrow));
    return LockedArrow(FixedName(tag, "pass"),
        Juxtaposition(tag, function, expression));
}

static Node* reduceNewline(Node* operator, Node* left, Node* right) {
    // if left is a syntax definition then right is the scope of the definition
    // which has already been parsed, so the scope of the definition is over
    if (isSyntaxDefinition(left))
        popSyntax();

    if (isDefinition(left))
        return applyDefinition(left, right);
    if (isKeyphrase(left, "define"))
        return reduceDefine(getLeft(left), getRight(left), right);
    if (isCase(left) && isCase(right))
        return combineCases(getTag(operator), left, right);
    if (isKeyphrase(left, "case"))
        return newCaseArrow(getRight(left), right);
    if (isKeyphrase(left, "with"))
        return reduceWith(getLeft(left), getRight(left), right);
    return reduceApply(operator, left, right);
}

static Node* reduceAbort(Node* operator, Node* left, Node* right) {
    (void)left;
    Tag tag = getTag(operator);
    Node* exit = FixedName(tag, "(exit)");
    return Juxtaposition(tag, exit, Juxtaposition(tag, Printer(tag),
        Juxtaposition(tag, FixedName(tag, "abort"), right)));
}

static Node* reduceReserved(Node* operator, Node* left, Node* right) {
    (void)left, (void)right;
    syntaxError("reserved operator", operator);
    return NULL;
}

static Node* reduceClose(Node* operator, Node* left, Node* right) {
    (void)operator, (void)left;
    return right;
}

bool isParenthesizedAdfixOperator(Node* node) {
    return isJuxtaposition(node) && isName(getLeft(node)) &&
        (isThisName(getRight(node), "*.") || isThisName(getRight(node), ".*"));
}

bool isParenthesizedInfixOperator(Node* node) {
    return isJuxtaposition(node) && isThisName(getRight(node), "*.") &&
        isJuxtaposition(getLeft(node)) && isThisName(getRight(getLeft(node)),
        ".*") && isName(getLeft(getLeft(node)));
}

static Node* reduceOpenSection(Node* open, Node* before, Node* contents) {
    syntaxErrorIf(before != NULL, "invalid operand before section", open);
    if (isCommaPair(contents))
        syntaxError("comma invalid in section", open);
    if (!isJuxtaposition(contents))
        syntaxError("invalid section", open);
    if (isParenthesizedAdfixOperator(contents))
        return getLeft(contents);
    if (isParenthesizedInfixOperator(contents))
        return getLeft(getLeft(contents));
    return LockedArrow(FixedName(getTag(open), ".*"), contents);
}

static Node* reduceCloseSection(Node* close, Node* before, Node* contents) {
    syntaxErrorIf(before != NULL, "invalid operand before section", close);
    if (isTuple(contents))
        syntaxError("comma invalid in section", close);
    if (!isJuxtaposition(contents) && !isName(contents))
        syntaxError("invalid section", close);
    if (isParenthesizedAdfixOperator(contents))
        return getLeft(contents);
    if (isName(contents))
        return contents;    // parenthesized operator
    return LockedArrow(FixedName(getTag(close), "*."), contents);
}

void initSymbols(void) {
    initSyntax();
    addBracketSyntax("", '\0', 0, OPENFIX, reduceOpenFile);
    addBracketSyntax("\0", '\0', 0, CLOSEFIX, reduceClose);
    addBracketSyntax("(", '(', 95, OPENFIX, reduceOpenParenthesis);
    addBracketSyntax(")", '(', 95, CLOSEFIX, reduceClose);
    addBracketSyntax("( ", '(', 95, OPENFIX, reduceOpenSection);
    addBracketSyntax(" )", '(', 95, CLOSEFIX, reduceCloseSection);
    addBracketSyntax("[", '[', 95, OPENFIX, reduceOpenSquareBracket);
    addBracketSyntax("]", '[', 95, CLOSEFIX, reduceClose);
    addBracketSyntax("{", '{', 95, OPENFIX, reduceOpenBrace);
    addBracketSyntax("}", '{', 95, CLOSEFIX, reduceClose);
    addCoreSyntax("|", 1, 1, INFIX, N, reduceReserved);
    addCoreSyntax(",", 2, 2, INFIX, L, reduceCommaPair);
    addCoreSyntax("\n", 3, 3, INFIX, R, reduceNewline);
    addCoreSyntax(";;", 4, 4, INFIX, R, reduceNewline);
    addCoreSyntax("define", 5, 5, PREFIX, L, reducePrefix);
    addCoreSyntax(":=", 5, 5, INFIX, R, reduceDefine);
    addCoreSyntax("::=", 5, 5, INFIX, R, reduceADTDefinition);
    addCoreSyntax("where", 5, 5, INFIX, R, reduceWhere);
    addCoreSyntax("|>", 6, 6, INFIX, L, reducePipeline);
    addCoreSyntax("<|", 6, 6, INFIX, R, reduceApply);
    addCoreSyntax(";", 8, 8, INFIX, R, reduceNewline);
    addCoreSyntax(":", 9, 9, INFIX, N, reduceColonPair);
    addCoreSyntax("->", 10, 10, INFIX, R, reduceArrow);
    addCoreSyntax("=>", 10, 10, INFIX, R, reduceInfix);
    addCoreSyntax("case", 11, 11, PREFIX, N, reducePrefix);
    addCoreSyntax("with", 11, 11, PREFIX, N, reducePrefix);
    addCoreSyntax("@", 12, 12, INFIX, N, reduceAsPattern);
    addCoreSyntax("as", 12, 12, INFIX, N, reduceAsPattern);
    addCoreSyntax("abort", 15, 15, PREFIX, L, reduceAbort);
    addCoreSyntax(".", 92, 92, INFIX, L, reducePipeline);
    addCoreSyntax("$", 99, 99, PREFIX, L, reduceReserved);
    addCoreSyntax("syntax", 99, 99, PREFIX, L, reducePrefix);
    addCoreSyntax("alias", 99, 99, PREFIX, L, reducePrefix);
    addCoreAlias("\xE2\x89\x94", ":="); // u2254
    addCoreAlias("\xE2\xA9\xB4", "::="); // u2A74
    addCoreAlias("\xE2\x86\xA6", "->"); // u21A6
    addCoreAlias("\xE2\x87\x92", "=>"); // u21D2
    addCoreAlias("\xE2\xA6\x8A", "|>"); // u298A
    addCoreAlias("\xE2\xA6\x89", "<|"); // u2989
}
