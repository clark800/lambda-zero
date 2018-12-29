#include "shared/lib/tree.h"
#include "shared/lib/stack.h"
#include "parse/shared/errors.h"
#include "parse/shared/ast.h"
#include "symbols.h"
#include "patterns.h"
#include "define.h"
#include "brackets.h"

static Node* validatePrior(Node* operator, Node* node) {
    String prior = getPrior(operator);
    if (prior.length > 0)
        if (!isJuxtaposition(node) || !isSameString(getLexeme(node), prior))
            syntaxError("operator syntax error", operator);
    return operator;
}

static Node* reduceApply(Node* operator, Node* left, Node* right) {
    return Juxtaposition(getTag(operator), left, right);
}

static Node* reduceAdfix(Node* operator, Node* argument) {
    return reduceApply(operator, Name(getTag(operator)), argument);
}

static Node* reduceInfix(Node* operator, Node* left, Node* right) {
    return reduceApply(operator, reduceAdfix(operator, left), right);
}

static Node* reduceInfixL(Node* operator, Node* left, Node* right) {
    return reduceInfix(validatePrior(operator, left), left, right);
}

static Node* reduceInfixR(Node* operator, Node* left, Node* right) {
    return reduceInfix(validatePrior(operator, right), left, right);
}

static Node* reducePrefix(Node* operator, Node* left, Node* right) {
    (void)left;
    return reduceAdfix(validatePrior(operator, right), right);
}

static Node* reducePostfix(Node* operator, Node* left, Node* right) {
    (void)right;
    return reduceAdfix(validatePrior(operator, left), left);
}

static Node* reduceArrow(Node* operator, Node* left, Node* right) {
    return newStrictArrow(getTag(operator), left, right);
}

static Node* reduceNewline(Node* operator, Node* left, Node* right) {
    if (isDefinition(left))
        return applyDefinition(left, right);
    if (isKeyphrase(left, "define"))
        return reduceDefine(getLeft(left), getRight(left), right);
    if (isCase(left) && isCase(right))
        return combineCases(getTag(operator), left, right);
    if (isKeyphrase(left, "case"))
        return reduceArrow(operator, getRight(left), right);
    return reduceApply(operator, left, right);
}

static Node* reduceInterfix(Node* operator, Node* left, Node* right) {
    return reduceNewline(validatePrior(operator, left), left, right);
}

static Node* reduceWhere(Node* operator, Node* left, Node* right) {
    if (!isDefinition(right))
        syntaxError("expected definition to right of", operator);
    return applyDefinition(right, left);
}

static Node* reducePipeline(Node* operator, Node* left, Node* right) {
    return reduceApply(operator, right, left);
}

static Node* reduceAsPattern(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(!isName(left), "expected name to left of" , operator);
    return AsPattern(getTag(operator), left, right);
}

static Node* reduceCommaPair(Node* operator, Node* left, Node* right) {
    return CommaPair(getTag(operator), left, right);
}

static Node* reduceReserved(Node* operator, Node* left, Node* right) {
    (void)left, (void)right;
    syntaxError("reserved operator", operator);
    return NULL;
}

static void shiftSpace(Stack* stack, Node* operator) {
    reduceLeft(stack, operator);
    // ignore space after operators (note: close and postfix are never
    // pushed onto the stack, so the operator must be expecting a right operand)
    if (!isOperator(peek(stack, 0)))
        push(stack, operator);
}

static void shiftPrefix(Stack* stack, Node* operator) {
    reduceLeft(stack, operator);
    push(stack, operator);
}

static void shiftInfix(Stack* stack, Node* operator) {
    erase(stack, " ");      // if we erase newlines it would break associativity
    reduceLeft(stack, operator);
    if (isOperator(peek(stack, 0))) {
        if (isThisOperator(operator, "+"))
            operator = parseSymbol(renameTag(getTag(operator), "(+)"), 0);
        else if (isThisOperator(operator, "-"))
            operator = parseSymbol(renameTag(getTag(operator), "(-)"), 0);
        else if (!isSpecial(operator) && isOpenOperator(peek(stack, 0)))
            push(stack, LeftPlaceholder(getTag(operator)));
        else syntaxError("missing left operand for", operator);
    }
    push(stack, operator);
}

static void shiftPostfix(Stack* stack, Node* operator) {
    erase(stack, " ");      // if we erase newlines it would break associativity
    reduceLeft(stack, operator);
    if (!isSpecial(operator) && isOpenOperator(peek(stack, 0)))
        push(stack, LeftPlaceholder(getTag(operator)));
    if (isOperator(peek(stack, 0)))
        syntaxError("missing left operand for", operator);
    Hold* operand = pop(stack);
    push(stack, reduce(operator, getNode(operand), VOID));
    release(operand);
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

static Precedence parsePrecedence(Node* node) {
    Precedence precedence = isNumber(node) ?
        (Precedence)getValue(node) : findPrecedence(node);
    syntaxErrorIf(precedence > 99, "invalid precedence", node);
    return precedence;
}

static void defineSyntax(Node* definition, Node* left, Node* right) {
    syntaxErrorIf(!isJuxtaposition(left), "invalid left operand", left);
    Node* name = getRight(left);
    syntaxErrorIf(!isName(name), "expected symbol operand to", getLeft(left));
    if (!isJuxtaposition(right))
        syntaxError("invalid syntax definition", definition);

    Tag tag = getTag(name);
    Node* fixity = getLeft(right);
    if (isThisName(fixity, "alias")) {
        addSyntaxCopy(getLexeme(name), getRight(right), true);
        return;
    }
    if (isThisName(fixity, "syntax")) {
        addSyntaxCopy(getLexeme(name), getRight(right), false);
        return;
    }

    Node* argument = getRight(right);
    Precedence p = parsePrecedence(argument);
    String prior = isNumber(argument) ? newString("", 0) : getLexeme(argument);

    if (isThisName(name, "()")) {
        tag = renameTag(tag, " ");
        if (isThisName(fixity, "infixL"))
            addSyntax(tag, prior, p, p, INFIX, L, shiftSpace, reduceInfixL);
        else if (isThisName(fixity, "interfix"))
            addSyntax(tag, prior, p, p, INFIX, L, shiftSpace, reduceInterfix);
        else syntaxError("syntax must be infixL or interfix", fixity);
    }
    else if (isThisName(fixity, "infix"))
        addSyntax(tag, prior, p, p, INFIX, N, shiftInfix, reduceInfix);
    else if (isThisName(fixity, "infixL"))
        addSyntax(tag, prior, p, p, INFIX, L, shiftInfix, reduceInfixL);
    else if (isThisName(fixity, "infixR"))
        addSyntax(tag, prior, p, p, INFIX, R, shiftInfix, reduceInfixR);
    else if (isThisName(fixity, "interfix"))
        addSyntax(tag, prior, p, p, INFIX, L, shiftInfix, reduceInterfix);
    else if (isThisName(fixity, "prefix"))
        addSyntax(tag, prior, p, p, PREFIX, L, shiftPrefix, reducePrefix);
    else if (isThisName(fixity, "postfix"))
        addSyntax(tag, prior, p, p, POSTFIX, L, shiftPostfix, reducePostfix);
    else syntaxError("invalid fixity", fixity);

    // add special case prefix operators for "+" and "-"
    // done here to avoid hard-coding a precedence for them
    if (isThisName(name, "+"))
        addCoreSyntax("(+)", p, p, PREFIX, L, shiftPrefix, reducePrefix);
    if (isThisName(name, "-"))
        addCoreSyntax("(-)", p, p, PREFIX, L, shiftPrefix, reducePrefix);
}

static void shiftNewline(Stack* stack, Node* operator) {
    erase(stack, " ");
    reduceLeft(stack, operator);
    Node* top = peek(stack, 0);
    if (isSyntaxDefinition(top))
        defineSyntax(top, getLeft(top), getRight(top));
    // ignore newlines after operators for line continuations
    if (!isOperator(peek(stack, 0)))
        push(stack, operator);
}

void initSymbols(void) {
    addCoreSyntax("\0", 0, 0, CLOSEFIX, R, shiftClose, reduceEOF);
    addCoreSyntax("(", 95, 0, OPENFIX, R, shiftOpen, reduceUnmatched);
    addCoreSyntax(")", 0, 95, CLOSEFIX, R, shiftClose, reduceParentheses);
    addCoreSyntax("[", 95, 0, OPENFIX, R, shiftOpen, reduceUnmatched);
    addCoreSyntax("]", 0, 95, CLOSEFIX, R, shiftClose, reduceSquareBrackets);
    addCoreSyntax("{", 95, 0, OPENFIX, R, shiftOpen, reduceUnmatched);
    addCoreSyntax("}", 0, 95, CLOSEFIX, R, shiftClose, reduceCurlyBrackets);
    addCoreSyntax("|", 1, 1, INFIX, N, shiftInfix, reduceReserved);
    addCoreSyntax(",", 2, 2, INFIX, L, shiftInfix, reduceCommaPair);
    addCoreSyntax("\n", 3, 3, INFIX, RV, shiftNewline, reduceNewline);
    addCoreSyntax(";;", 4, 4, INFIX, R, shiftInfix, reduceNewline);
    addCoreSyntax("define", 5, 5, PREFIX, L, shiftPrefix, reducePrefix);
    addCoreSyntax(":=", 5, 5, INFIX, R, shiftInfix, reduceDefine);
    addCoreSyntax("::=", 5, 5, INFIX, R, shiftInfix, reduceADTDefinition);
    addCoreSyntax("where", 5, 5, INFIX, R, shiftInfix, reduceWhere);
    addCoreSyntax("|>", 6, 6, INFIX, L, shiftInfix, reducePipeline);
    addCoreSyntax("<|", 6, 6, INFIX, R, shiftInfix, reduceApply);
    addCoreSyntax(";", 8, 8, INFIX, R, shiftInfix, reduceNewline);
    addCoreSyntax("->", 9, 9, INFIX, R, shiftInfix, reduceArrow);
    addCoreSyntax("=>", 9, 9, INFIX, R, shiftInfix, reduceInfix);
    addCoreSyntax("case", 10, 10, PREFIX, N, shiftPrefix, reducePrefix);
    addCoreSyntax("@", 12, 12, INFIX, N, shiftInfix, reduceAsPattern);
    addCoreSyntax("abort", 15, 15, PREFIX, L, shiftPrefix, reduceAbort);
    addCoreSyntax(".", 92, 92, INFIX, L, shiftInfix, reducePipeline);
    addCoreSyntax("$", 99, 99, PREFIX, L, shiftPrefix, reduceReserved);
    addCoreSyntax("( )", 99, 99, INFIX, L, shiftSpace, reduceInvalid);
    addCoreAlias("\u2254", ":=");
    addCoreAlias("\u2A74", "::=");
    addCoreAlias("\u21A6", "->");
    addCoreAlias("\u21D2", "=>");
    addCoreAlias("\u298A", "|>");
    addCoreAlias("\u2989", "<|");
}
