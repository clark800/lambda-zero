#include "shared/lib/tree.h"
#include "shared/lib/stack.h"
#include "parse/shared/errors.h"
#include "parse/shared/ast.h"
#include "symbols.h"
#include "patterns.h"
#include "define.h"
#include "brackets.h"

static Node* reduceArrow(Node* operator, Node* left, Node* right) {
    (void)operator;
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

static Node* reduceInvalid(Node* operator, Node* left, Node* right) {
    (void)left, (void)right;
    syntaxError("missing operator", operator);
    return NULL;
}

static Node* reduceReserved(Node* operator, Node* left, Node* right) {
    (void)left, (void)right;
    syntaxError("reserved operator", operator);
    return NULL;
}

void initSymbols(void) {
    addCoreSyntax("\0", 0, 0, CLOSEFIX, R, reduceEOF);
    addCoreSyntax("(", 95, 0, OPENFIX, R, reduceUnmatched);
    addCoreSyntax(")", 0, 95, CLOSEFIX, R, reduceParentheses);
    addCoreSyntax("[", 95, 0, OPENFIX, R, reduceUnmatched);
    addCoreSyntax("]", 0, 95, CLOSEFIX, R, reduceSquareBrackets);
    addCoreSyntax("{", 95, 0, OPENFIX, R, reduceUnmatched);
    addCoreSyntax("}", 0, 95, CLOSEFIX, R, reduceCurlyBrackets);
    addCoreSyntax("|", 1, 1, INFIX, N, reduceReserved);
    addCoreSyntax(",", 2, 2, INFIX, L, reduceCommaPair);
    addCoreSyntax("\n", 3, 3, INFIX, RV, reduceNewline);
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
    addCoreSyntax("( )", 99, 99, SPACEFIX, L, reduceInvalid);
    addCoreAlias("\u2254", ":=");
    addCoreAlias("\u2A74", "::=");
    addCoreAlias("\u21A6", "->");
    addCoreAlias("\u21D2", "=>");
    addCoreAlias("\u298A", "|>");
    addCoreAlias("\u2989", "<|");
    addCoreAlias("\u2208", ":");
}
