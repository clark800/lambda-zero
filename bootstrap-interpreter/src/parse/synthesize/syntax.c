#include "shared/lib/tree.h"
#include "shared/lib/stack.h"
#include "parse/shared/errors.h"
#include "parse/shared/ast.h"
#include "symbols.h"
#include "patterns.h"
#include "define.h"
#include "brackets.h"

static Node* reduceApply(Node* operator, Node* left, Node* right) {
    return Juxtaposition(getTag(operator), left, right);
}

static Node* reduceInfix(Node* operator, Node* left, Node* right) {
    return reduceApply(operator, reduceApply(operator,
        Name(getTag(operator), 0), left), right);
}

static Node* reducePrefix(Node* operator, Node* left, Node* right) {
    (void)left;
    return reduceApply(operator, Name(getTag(operator), 0), right);
}

static Node* reducePostfix(Node* operator, Node* left, Node* right) {
    (void)right;
    return reduceApply(operator, Name(getTag(operator), 0), left);
}

static Node* reducePeriod(Node* operator, Node* left, Node* right) {
    return reduceApply(operator, right, left);
}

static Node* reduceArrow(Node* operator, Node* left, Node* right) {
    if (isKeyphrase(left, "case"))
        return reduceCase(operator, getRight(left), right);
    return newPatternLambda(getTag(operator), left, right);
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
    Node* exit = Name(renameTag(tag, "(exit)"), 0);
    return Juxtaposition(tag, exit, Juxtaposition(tag, Printer(tag),
        Juxtaposition(tag, Name(renameTag(tag, "abort"), 0), right)));
}

static Node* reduceInvalid(Node* operator, Node* left, Node* right) {
    (void)left, (void)right;
    syntaxError("operator syntax undeclared", operator);
    return NULL;
}

static void defineSyntax(Node* definition, Node* left, Node* right) {
    syntaxErrorIf(!isJuxtaposition(left), "invalid left operand", left);
    Node* name = getRight(left);
    syntaxErrorIf(!isName(name), "expected symbol operand to", getLeft(left));
    if (contains(getLexeme(name), '_'))
       syntaxError("invalid underscore in operator name", name);
    if (!isJuxtaposition(right))
        syntaxError("invalid syntax definition", definition);

    Tag tag = getTag(name);
    Node* fixity = getLeft(right);
    if (isThisName(fixity, "mixfix")) {
        addMixfixSyntax(tag, getRight(right), shiftInfix);
        return;
    }
    if (isThisName(fixity, "alias")) {
        addSyntaxCopy(getLexeme(name), getRight(right), true);
        return;
    }
    if (isThisName(fixity, "syntax")) {
        addSyntaxCopy(getLexeme(name), getRight(right), false);
        return;
    }

    if (!isNumber(getRight(right)))
        syntaxError("invalid syntax definition", definition);
    long long precedence = getValue(getRight(right));
    if (precedence < 0 || precedence > 99)
        syntaxError("invalid precedence", getRight(right));
    Precedence p = (Precedence)precedence;

    if (isThisName(name, "()")) {
        tag = renameTag(tag, " ");
        if (!isThisName(fixity, "infixL"))
            syntaxError("syntax must be infixL", fixity);
        addSyntax(tag, p, p, INFIX, L, shiftSpace, reduceInfix);
    }
    else if (isThisName(fixity, "infix"))
        addSyntax(tag, p, p, INFIX, N, shiftInfix, reduceInfix);
    else if (isThisName(fixity, "infixL"))
        addSyntax(tag, p, p, INFIX, L, shiftInfix, reduceInfix);
    else if (isThisName(fixity, "infixR"))
        addSyntax(tag, p, p, INFIX, R, shiftInfix, reduceInfix);
    else if (isThisName(fixity, "prefix"))
        addSyntax(tag, p, p, PREFIX, L, shiftPrefix, reducePrefix);
    else if (isThisName(fixity, "postfix"))
        addSyntax(tag, p, p, POSTFIX, L, shiftPostfix, reducePostfix);
    else syntaxError("invalid fixity", fixity);

    // add special case prefix operators for "+" and "-"
    // done here to avoid hard-coding a precedence for them
    if (isThisName(name, "+"))
        addCoreSyntax("(+)", p, p, PREFIX, L, shiftPrefix, reducePrefix);
    if (isThisName(name, "-"))
        addCoreSyntax("(-)", p, p, PREFIX, L, shiftPrefix, reducePrefix);
}

static Node* reduceNewline(Node* operator, Node* left, Node* right) {
    if (isDefinition(left))
        return applyDefinition(left, right);
    if (isKeyphrase(left, "define"))
        return reduceDefine(getLeft(left), getRight(left), right);
    if (isCase(left) && isCase(right))
        return combineCases(left, right);
    if (isKeyphrase(left, "case"))
        return reduceArrow(operator, left, right);
    return reduceApply(operator, left, right);
}

static Node* reduceWhere(Node* operator, Node* left, Node* right) {
    return reduceNewline(operator, right, left);
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
    addCoreSyntax("(", 90, 0, OPENFIX, R, shiftOpen, reduceUnmatched);
    addCoreSyntax(")", 0, 90, CLOSEFIX, R, shiftClose, reduceParentheses);
    addCoreSyntax("[", 90, 0, OPENFIX, R, shiftOpen, reduceUnmatched);
    addCoreSyntax("]", 0, 90, CLOSEFIX, R, shiftClose, reduceSquareBrackets);
    addCoreSyntax("{", 90, 0, OPENFIX, R, shiftOpen, reduceUnmatched);
    addCoreSyntax("}", 0, 90, CLOSEFIX, R, shiftClose, reduceCurlyBrackets);
    addCoreSyntax("||", 1, 1, INFIX, N, shiftInfix, reduceReserved);
    addCoreSyntax(",", 2, 2, INFIX, L, shiftInfix, reduceCommaPair);
    addCoreSyntax("\n", 3, 3, INFIX, RV, shiftNewline, reduceNewline);
    addCoreSyntax(";;", 4, 4, INFIX, R, shiftInfix, reduceNewline);
    addCoreSyntax("define", 6, 6, PREFIX, N, shiftPrefix, reducePrefix);
    addCoreSyntax(":=", 6, 6, INFIX, N, shiftInfix, reduceDefine);
    addCoreSyntax("::=", 6, 6, INFIX, N, shiftInfix, reduceADTDefinition);
    addCoreSyntax(";", 7, 7, INFIX, R, shiftInfix, reduceNewline);
    addCoreSyntax("|", 9, 5, INFIX, L, shiftInfix, reduceWhere);
    addCoreSyntax("where", 9, 5, INFIX, L, shiftInfix, reduceWhere);
    addCoreSyntax("->", 11, 8, INFIX, R, shiftInfix, reduceArrow); // 10 is try
    addCoreSyntax("case", 12, 12, PREFIX, N, shiftPrefix, reducePrefix);
    addCoreSyntax("maybe", 12, 12, PREFIX, N, shiftPrefix, reducePrefix);
    addCoreSyntax("@", 13, 13, INFIX, N, shiftInfix, reduceAsPattern);
    addCoreSyntax("abort", 16, 16, PREFIX, L, shiftPrefix, reduceAbort);
    addCoreSyntax("syntax", 90, 90, PREFIX, L, shiftPrefix, reducePrefix);
    addCoreSyntax(".", 95, 95, INFIX, L, shiftInfix, reducePeriod);
    addCoreSyntax("( )", 99, 99, INFIX, L, shiftSpace, reduceInvalid);
    addCoreSyntax("$", 99, 99, PREFIX, L, shiftPrefix, reduceReserved);
    addCoreAlias("\u2254", ":=");
    addCoreAlias("\u2A74", "::=");
    addCoreAlias("\u21A6", "->");
    addCoreAlias("\u2016", "||");
}
