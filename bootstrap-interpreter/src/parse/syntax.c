#include "tree.h"
#include "opp/errors.h"
#include "opp/operator.h"
#include "ast.h"
#include "patterns.h"
#include "define.h"
#include "brackets.h"
#include "syntax.h"

static Node* reduceArrow(Tag tag, Node* left, Node* right) {
    (void)tag;
    if (isKeyphrase(left, "case"))
        return newCase(getRight(left), right);
    return newArrow(left, right);
}

static Node* reducePipeline(Tag tag, Node* left, Node* right) {
    return Juxtaposition(tag, right, left);
}

static Node* reduceColonPair(Tag tag, Node* left, Node* right) {
    if (isColonPair(left) || !isValidPattern(left))
        syntaxError("invalid left side of colon", tag);
    syntaxErrorIf(isColonPair(right), "invalid right side of colon", tag);
    return ColonPair(tag, left, right);
}

static Node* reduceWhere(Tag tag, Node* left, Node* right) {
    syntaxErrorIf(!isDefinition(right), "expected definition to right of", tag);
    if (isSyntaxDefinition(right))
        syntaxError("invalid definition to right of", tag);
    return applyDefinition(right, left);
}

static Node* reduceIs(Tag tag, Node* left, Node* right) {
    syntaxErrorIf(!isKeyphrase(left, "if"), "expected 'if' before", tag);
    syntaxErrorIf(!isValidPattern(right), "expected pattern after", tag);
    Node* asPattern = AsPattern(tag, getRight(left), right);
    return Juxtaposition(tag, FixedName(tag, "if is"), asPattern);
}

static Node* reduceIfIs(Tag tag, Node* asPattern, Node* thenBlock) {
    if (!isAsPattern(asPattern))
        syntaxError("expected as pattern to right of", tag);
    Node* expression = getLeft(asPattern);
    Node* pattern = getRight(asPattern);
    Node* elseBlock = Underscore(tag, 3);
    Hold* caseArrow = hold(newCase(pattern, thenBlock));
    Hold* underscore = hold(Underscore(tag, 0));
    Node* fallback = newCase(underscore, elseBlock);
    Node* function = combineCases(tag, caseArrow, fallback);
    release(caseArrow);
    release(underscore);
    return SingleArrow(FixedName(tag, "pass"),
        Juxtaposition(tag, function, expression));
}

static Node* reduceNewline(Tag tag, Node* left, Node* right) {
    // if left is a syntax definition then right is the scope of the definition
    // which has already been parsed, so the scope of the definition is over
    if (isSyntaxDefinition(left))
        popSyntax();

    if (isDefinition(left))
        return applyDefinition(left, right);
    if (isKeyphrase(left, "def"))
        return reduceDefine(tag, getRight(left), right);
    if (isKeyphrase(left, "sig"))
        return right; // ignore sig
    if (isCase(left) && isCase(right))
        return combineCases(tag, left, right);
    if (isKeyphrase(left, "case"))
        return newCase(getRight(left), right);
    if (isKeyphrase(left, "if is"))
        return reduceIfIs(tag, getRight(left), right);
    if (isArrow(left) && isArrow(right))
        syntaxError("consecutive functions must be cases", tag);
    return Juxtaposition(tag, left, right);
}

static Node* reduceAbort(Tag tag, Node* left, Node* right) {
    (void)left;
    Node* exit = FixedName(tag, "(exit)");
    return Juxtaposition(tag, exit, Juxtaposition(tag, Printer(tag),
        Juxtaposition(tag, FixedName(tag, "abort"), right)));
}

static Node* reduceInfer(Tag tag, Node* left, Node* right) {
    (void)left, (void)right;
    return FixedName(tag, "(exit)");
}

static Node* reduceReserved(Tag tag, Node* left, Node* right) {
    (void)left, (void)right;
    syntaxError("reserved operator", tag);
    return NULL;
}

static Node* reduceErased(Tag tag, Node* left, Node* right) {
    (void)left, (void)right;
    // convert to a name that cannot be defined so it will cause an error
    // in bind unless it is erased
    return FixedName(tag, "(_)");
}

static Node* reduceRightIdentity(Tag tag, Node* left, Node* right) {
    (void)tag, (void)left;
    return right;
}

static Node* reduceLeftIdentity(Tag tag, Node* left, Node* right) {
    (void)tag, (void)right;
    return left;
}

static bool isParenthesizedAdfixOperator(Node* node) {
    return isJuxtaposition(node) && isName(getLeft(node)) &&
        (isThisName(getRight(node), "*.") || isThisName(getRight(node), ".*"));
}

static bool isParenthesizedInfixOperator(Node* node) {
    return isJuxtaposition(node) && isThisName(getRight(node), "*.") &&
        isJuxtaposition(getLeft(node)) && isThisName(getRight(getLeft(node)),
        ".*") && isName(getLeft(getLeft(node)));
}

static Node* reduceOpenSection(Tag tag, Node* before, Node* contents) {
    syntaxErrorIf(before != NULL, "invalid operand before section", tag);
    syntaxErrorIf(isCommaPair(contents), "comma invalid in section", tag);
    syntaxErrorIf(!isJuxtaposition(contents), "invalid section", tag);
    if (isParenthesizedAdfixOperator(contents))
        return getLeft(contents);
    if (isParenthesizedInfixOperator(contents))
        return getLeft(getLeft(contents));
    return SingleArrow(FixedName(tag, ".*"), contents);
}

static Node* reduceCloseSection(Tag tag, Node* before, Node* contents) {
    syntaxErrorIf(before != NULL, "invalid operand before section", tag);
    syntaxErrorIf(isTuple(contents), "comma invalid in section", tag);
    if (!isJuxtaposition(contents) && !isName(contents))
        syntaxError("invalid section", tag);
    if (isParenthesizedAdfixOperator(contents))
        return getLeft(contents);
    if (isName(contents))
        return contents;    // parenthesized operator
    return SingleArrow(FixedName(tag, "*."), contents);
}

static Node* reduceReverseArrow(Tag tag, Node* left, Node* right) {
    if (isKeyphrase(left, "try"))
        return Definition(tag, TRYBINDDEFINITION, getRight(left), right);
    return Definition(tag, BINDDEFINITION, left, right);
}

void initSymbols(void) {
    initSyntax();
    addBracketSyntax("", '\0', 0, OPENFIX, reduceOpenFile);
    addBracketSyntax("\0", '\0', 0, CLOSEFIX, reduceRightIdentity);
    addBracketSyntax("(", '(', 95, OPENFIX, reduceOpenParenthesis);
    addBracketSyntax(")", '(', 95, CLOSEFIX, reduceRightIdentity);
    addBracketSyntax("( ", '(', 95, OPENFIX, reduceOpenSection);
    addBracketSyntax(" )", '(', 95, CLOSEFIX, reduceCloseSection);
    addBracketSyntax("[", '[', 95, OPENFIX, reduceOpenSquareBracket);
    addBracketSyntax("]", '[', 95, CLOSEFIX, reduceRightIdentity);
    addBracketSyntax("{", '{', 95, OPENFIX, reduceOpenBrace);
    addBracketSyntax("}", '{', 95, CLOSEFIX, reduceRightIdentity);
    addCoreSyntax("\n", 1, INFIX, R, reduceNewline);
    addCoreSyntax(";", 2, INFIX, R, reduceNewline);
    addCoreSyntax("|", 3, INFIX, N, reduceReserved);
    addCoreSyntax(",", 4, INFIX, L, CommaPair);
    addCoreSyntax(":=", 5, INFIX, R, reduceDefine);
    addCoreSyntax("::=", 5, INFIX, N, reduceADTDefinition);
    addCoreSyntax("def", 5, PREFIX, N, reducePrefix);
    addCoreSyntax("sig", 5, PREFIX, N, reducePrefix);
    addCoreSyntax("where", 5, INFIX, R, reduceWhere);
    addCoreSyntax("for", 5, INFIX, L, reduceLeftIdentity);
    addCoreSyntax(":", 6, INFIX, N, reduceColonPair);
    addCoreSyntax("|>", 7, INFIX, L, reducePipeline);
    addCoreSyntax("<|", 7, INFIX, R, Juxtaposition);
    addCoreSyntax("->", 10, INFIX, R, reduceArrow);
    addCoreSyntax("<-", 10, INFIX, R, reduceReverseArrow);
    addCoreSyntax("=>", 10, INFIX, R, reduceErased);
    addCoreSyntax(">->", 10, INFIX, R, reduceErased);
    addCoreSyntax("case", 11, PREFIX, N, reducePrefix);
    addCoreSyntax("forall", 11, PREFIX, N, reducePrefix);
    addCoreSyntax("@", 12, INFIX, N, AsPattern);
    addCoreSyntax("is", 13, INFIX, L, reduceIs);
    addCoreSyntax("abort", 14, PREFIX, L, reduceAbort);
    addCoreSyntax("infer", 14, NOFIX, N, reduceInfer);
    addCoreSyntax("==", 20, INFIX, N, reduceErased);
    addCoreSyntax(".", 92, INFIX, L, reducePipeline);
    addCoreSyntax("$", 99, PREFIX, L, reduceReserved);
    addCoreSyntax("syntax", 99, PREFIX, L, reducePrefix);
    addCoreSyntax("alias", 99, PREFIX, L, reducePrefix);
    addCoreAlias("\xE2\x89\x94", ":="); // u2254
    addCoreAlias("\xE2\xA9\xB4", "::="); // u2A74
    addCoreAlias("\xE2\x89\xA1", "=="); // u2261
    addCoreAlias("\xE2\x86\xA6", "->"); // u21A6
    addCoreAlias("\xE2\x86\xA4", "<-"); // u21A4
    addCoreAlias("\xE2\x87\x92", "=>"); // u21D2
    addCoreAlias("\xE2\xA6\x8A", "|>"); // u298A
    addCoreAlias("\xE2\xA6\x89", "<|"); // u2989
    addCoreAlias("\xE2\x88\x80", "forall"); // u2200
}
