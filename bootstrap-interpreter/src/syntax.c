#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "errors.h"
#include "symbols.h"
#include "patterns.h"
#include "define.h"
#include "brackets.h"

static Node* reduceApply(Node* operator, Node* left, Node* right) {
    return newApplication(getTag(operator), left, right);
}

static Node* reduceInfix(Node* operator, Node* left, Node* right) {
    return reduceApply(operator, reduceApply(operator,
        convertOperator(getTag(operator)), left), right);
}

static Node* reducePrefix(Node* operator, Node* left, Node* right) {
    (void)left;
    return reduceApply(operator, convertOperator(getTag(operator)), right);
}

static Node* reducePostfix(Node* operator, Node* left, Node* right) {
    (void)right;
    return reduceApply(operator, convertOperator(getTag(operator)), left);
}

static Node* reduceReserved(Node* operator, Node* left, Node* right) {
    syntaxError("reserved operator", operator);
    return left == NULL ? right : left; // suppress unused parameter warning
}

static void shiftPrefix(Stack* stack, Node* operator) {
    if (isSpaceOperator(peek(stack, 0)))
        release(pop(stack));
    reduceLeft(stack, operator);
    push(stack, operator);
}

static void shiftInfix(Stack* stack, Node* operator) {
    if (isSpaceOperator(peek(stack, 0)))
        release(pop(stack));
    reduceLeft(stack, operator);
    if (isOperator(peek(stack, 0))) {
        if (isThisLeaf(operator, "+"))
            operator = parseSymbol(renameTag(getTag(operator), "(+)"));
        else if (isThisLeaf(operator, "-"))
            operator = parseSymbol(renameTag(getTag(operator), "(-)"));
        else if (!isSpecial(operator) && isOpenOperator(peek(stack, 0)))
            push(stack, newName(renameTag(getTag(operator), "_.")));
        else syntaxError("missing left operand for", operator);
    }
    if (isThisLeaf(operator, ":=") && isThisLexeme(peek(stack, 0), "syntax"))
        operator = parseSymbol(renameTag(getTag(operator), "(:=)"));
    push(stack, operator);
}

static void shiftPostfix(Stack* stack, Node* operator) {
    if (isSpaceOperator(peek(stack, 0)))
        release(pop(stack));
    reduceLeft(stack, operator);
    if (!isSpecial(operator) && isOpenOperator(peek(stack, 0)))
        push(stack, newName(renameTag(getTag(operator), "_.")));
    if (isOperator(peek(stack, 0)))
        syntaxError("missing left operand for", operator);
    Hold* operand = pop(stack);
    push(stack, reduce(operator, getNode(operand), NULL));
    release(operand);
}

static void shiftWhitespace(Stack* stack, Node* operator) {
    if (isSpaceOperator(peek(stack, 0)))
        release(pop(stack));
    reduceLeft(stack, operator);
    // ignore whitespace after operators
    if (!isOperator(peek(stack, 0)))
        push(stack, operator);
}

static Node* reduceError(Node* operator, Node* left, Node* right) {
    (void)left;
    Tag tag = getTag(operator);
    if (isThisLeaf(right, "[]"))
        return newBuiltin(tag, UNDEFINED);
    Node* exit = newBuiltin(renameTag(tag, "exit"), EXIT);
    return newApplication(tag, exit, newApplication(tag, newPrinter(tag),
        newApplication(tag, newBuiltin(tag, ERROR), right)));
}

static Node* reduceSyntax(Node* operator, Node* left, Node* right) {
    Node* name = getRight(left);
    syntaxErrorIf(!isLeaf(name), "expected symbol operand to", getLeft(left));
    if (!isApplication(right) || !isInteger(getRight(right)))
        syntaxError("invalid syntax definition", operator);
    long long precedence = getValue(getRight(right));
    if (precedence < 0 || precedence > 99)
        syntaxError("invalid precedence", getRight(right));
    Precedence p = (Precedence)precedence;
    Node* fixity = getLeft(right);

    if (isThisLeaf(fixity, "infix"))
        addSyntax(name, p, p, INFIX, N, shiftInfix, reduceInfix);
    else if (isThisLeaf(fixity, "infixL"))
        addSyntax(name, p, p, INFIX, L, shiftInfix, reduceInfix);
    else if (isThisLeaf(fixity, "infixR"))
        addSyntax(name, p, p, INFIX, R, shiftInfix, reduceInfix);
    else if (isThisLeaf(fixity, "prefix"))
        addSyntax(name, p, p, PREFIX, L, shiftPrefix, reducePrefix);
    else if (isThisLeaf(fixity, "postfix"))
        addSyntax(name, p, p, POSTFIX, L, shiftPostfix, reducePostfix);
    else syntaxError("invalid fixity", fixity);

    // add special case prefix operators for "+" and "-"
    // done here to avoid hard-coding a precedence for them
    if (isThisLeaf(name, "+") && isThisLeaf(fixity, "infixL"))
        addBuiltinSyntax("(+)", p, p, PREFIX, L, shiftPrefix, reducePrefix);
    if (isThisLeaf(name, "-") && isThisLeaf(fixity, "infixL"))
        addBuiltinSyntax("(-)", p, p, PREFIX, L, shiftPrefix, reducePrefix);

    // reduce to an identity function with the operator symbol as parameter name
    // so that if the operator symbol is already defined it will be a syntax
    // error. this ensures that operator symbols can't be used before their
    // syntax is declared because use must follow definition and definition
    // must follow syntax declaration. builtin operators don't need to be
    // defined, but they can only be accessed as operators, which again means
    // that use must follow syntax declaration.
    Node* symbol = newName(getTag(name));
    return newLambda(getTag(operator), symbol, symbol);
}

void initSymbols(void) {
    addBuiltinSyntax("\0", 0, 0, CLOSEFIX, L, shiftClose, reduceEOF);
    addBuiltinSyntax("(", 90, 0, OPENFIX, L, shiftOpen, reduceUnmatched);
    addBuiltinSyntax(")", 0, 90, CLOSEFIX, R, shiftClose, reduceParentheses);
    addBuiltinSyntax("[", 90, 0, OPENFIX, L, shiftOpen, reduceUnmatched);
    addBuiltinSyntax("]", 0, 90, CLOSEFIX, R, shiftClose, reduceSquareBrackets);
    addBuiltinSyntax("{", 90, 0, OPENFIX, L, shiftOpenCurly, reduceUnmatched);
    addBuiltinSyntax("}", 0, 90, CLOSEFIX, R, shiftClose, reduceCurlyBrackets);
    addBuiltinSyntax("|", 1, 1, INFIX, N, shiftInfix, reduceReserved);
    addBuiltinSyntax(",", 2, 2, INFIX, L, shiftInfix, reduceApply);
    addBuiltinSyntax("\n", 3, 3, INFIX, R, shiftWhitespace, reduceApply);
    addBuiltinSyntax(":=", 3, 3, INFIX, R, shiftInfix, reduceDefine);
    addBuiltinSyntax("::=", 3, 3, INFIX, R, shiftInfix, reduceADTDefinition);
    addBuiltinSyntax("(:=)", 4, 4, INFIX, N, shiftInfix, reduceSyntax);
    addBuiltinSyntax(";", 4, 4, INFIX, L, shiftInfix, reducePatternLambda);
    addBuiltinSyntax("syntax", 5, 5, PREFIX, N, shiftPrefix, reducePrefix);
    addBuiltinSyntax("->", 5, 5, INFIX, R, shiftInfix, reduceLambda);
    addBuiltinSyntax("error", 80, 80, PREFIX, L, shiftPrefix, reduceError);
    addBuiltinSyntax(" ", 80, 80, INFIX, L, shiftWhitespace, reduceApply);
}
