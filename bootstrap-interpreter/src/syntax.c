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
        convertOperator(operator), left), right);
}

static Node* reducePrefix(Node* operator, Node* left, Node* right) {
    (void)left;
    return reduceApply(operator, convertOperator(operator), right);
}

static Node* reducePostfix(Node* operator, Node* left, Node* right) {
    (void)right;
    return reduceApply(operator, convertOperator(operator), left);
}

static Node* reduceNegate(Node* operator, Node* left, Node* right) {
    (void)left;
    Tag tag = getTag(operator);
    return newApplication(tag, newName(renameTag(tag, "negate")), right);
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
    if (!isSpecial(operator) && isOpenOperator(peek(stack, 0)))
        push(stack, newName(renameTag(getTag(operator), "_.")));
    if (isOperator(peek(stack, 0)))
        syntaxError("missing left operand for", operator);
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
    (void)left, (void)right;
    Tag tag = getTag(operator);
    Node* print = newPrinter(tag);
    Node* exit = newBuiltin(renameTag(tag, "exit"), EXIT);
    Node* error = newBuiltin(renameTag(tag, "error"), ERROR);
    Node* blank = newBlankReference(tag, 1);
    // error(message) ~> (_ -> exit(print(error(_))))(message)
    return newLambda(tag, newBlank(tag),
        newApplication(tag, exit, newApplication(tag, print,
        newApplication(tag, error, blank))));
}

void shiftOperand(Stack* stack, Node* node) {
    Node* operand = reduce(node, NULL, NULL);
    if (isEmpty(stack) || isOperator(peek(stack, 0))) {
        push(stack, operand);
    } else {
        Hold* left = pop(stack);
        push(stack, newApplication(getTag(node), getNode(left), operand));
        release(left);
    }
}

static Node* reduceOperand(Node* operand, Node* left, Node* right) {
    (void)left, (void)right;
    return operand;
}

void initSymbols(void) {
    addSymbol("(_)", 0, 0, NOFIX, N, shiftOperand, reduceOperand);
    addSymbol("error", 0, 0, NOFIX, N, shiftOperand, reduceError);

    // syntactic operators
    addSymbol("\0", 0, 0, CLOSEFIX, L, shiftClose, reduceEOF);
    addSymbol("(", 23, 0, OPENFIX, L, shiftOpen, reduceUnmatched);
    addSymbol(")", 0, 23, CLOSEFIX, R, shiftClose, reduceParentheses);
    addSymbol("[", 23, 0, OPENFIX, L, shiftOpen, reduceUnmatched);
    addSymbol("]", 0, 23, CLOSEFIX, R, shiftClose, reduceSquareBrackets);
    addSymbol("{", 23, 0, OPENFIX, L, shiftOpenCurly, reduceUnmatched);
    addSymbol("}", 0, 23, CLOSEFIX, R, shiftClose, reduceCurlyBrackets);
    addSymbol("|", 1, 1, INFIX, N, shiftInfix, reduceReserved);
    addSymbol(",", 2, 2, INFIX, L, shiftInfix, reduceApply);
    addSymbol("\n", 3, 3, INFIX, R, shiftWhitespace, reduceApply);
    addSymbol(":=", 3, 3, INFIX, R, shiftInfix, reduceDefine);
    addSymbol("::=", 3, 3, INFIX, R, shiftInfix, reduceADTDefinition);
    addSymbol(";", 4, 4, INFIX, L, shiftInfix, reducePatternLambda);
    addSymbol("->", 5, 5, INFIX, R, shiftInfix, reduceLambda);

    // conditional operators
    addSymbol("||", 6, 6, INFIX, R, shiftInfix, reduceInfix);
    addSymbol("?", 7, 7, INFIX, R, shiftInfix, reduceInfix);
    addSymbol("?||", 7, 7, INFIX, R, shiftInfix, reduceInfix);

    // monadic operators
    addSymbol(">>=", 8, 8, INFIX, L, shiftInfix, reduceInfix);

    // logical operators
    addSymbol("<=>", 9, 9, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("=>", 10, 10, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("\\/", 11, 11, INFIX, R, shiftInfix, reduceInfix);
    addSymbol("/\\", 12, 12, INFIX, R, shiftInfix, reduceInfix);

    // comparison/test operators
    addSymbol("=", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("!=", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("=*=", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("=?=", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("<", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol(">", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("<=", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol(">=", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("=<", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("~<", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("<:", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("<*=", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol(":", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("!:", 13, 13, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("~", 13, 13, INFIX, N, shiftInfix, reduceInfix);

    // precedence 14: default
    // arithmetic/list operators
    addSymbol("|:", 14, 14, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("..", 15, 15, INFIX, N, shiftInfix, reduceInfix);
    addSymbol("::", 16, 16, INFIX, R, shiftInfix, reduceInfix);
    addSymbol("&", 16, 16, INFIX, L, shiftInfix, reduceInfix);
    addSymbol("+", 16, 16, INFIX, L, shiftInfix, reduceInfix);
    addSymbol("\\./", 16, 16, INFIX, L, shiftInfix, reduceInfix);
    addSymbol("++", 16, 16, INFIX, R, shiftInfix, reduceInfix);
    addSymbol("-", 16, 16, INFIX, L, shiftInfix, reduceInfix);
    addSymbol("\\", 16, 16, INFIX, L, shiftInfix, reduceInfix);
    addSymbol("*", 17, 17, INFIX, L, shiftInfix, reduceInfix);
    addSymbol("**", 17, 17, INFIX, R, shiftInfix, reduceInfix);
    addSymbol("/", 17, 17, INFIX, L, shiftInfix, reduceInfix);
    addSymbol("%", 17, 17, INFIX, L, shiftInfix, reduceInfix);

    // functional operators
    addSymbol("<>", 19, 19, INFIX, R, shiftInfix, reduceInfix);

    // prefix/postfix operators
    addSymbol("-", 20, 20, PREFIX, L, shiftPrefix, reduceNegate);
    addSymbol("--", 20, 20, PREFIX, L, shiftPrefix, reducePrefix);
    addSymbol("!", 20, 20, PREFIX, L, shiftPrefix, reducePrefix);
    addSymbol("#", 20, 20, PREFIX, L, shiftPrefix, reducePrefix);
    addSymbol("*", 20, 20, PREFIX, L, shiftPrefix, reduceAsterisk);
    addSymbol("...", 20, 20, POSTFIX, L, shiftPostfix, reducePostfix);

    addSymbol("^", 21, 21, INFIX, R, shiftInfix, reduceInfix);

    // space operator
    addSymbol(" ", 22, 22, INFIX, L, shiftWhitespace, reduceApply);

    // precedence 23: parentheses/brackets
    addSymbol("^^", 24, 24, INFIX, L, shiftInfix, reduceInfix);
    addSymbol(".", 25, 25, INFIX, L, shiftInfix, reduceInfix);
    addSymbol("`", 26, 26, PREFIX, L, shiftPrefix, reducePrefix);
}
