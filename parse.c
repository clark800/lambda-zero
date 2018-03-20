#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "objects.h"
#include "lex.h"
#include "operators.h"
#include "desugar.h"
#include "bind.h"
#include "serialize.h"
#include "parse.h"

bool DEBUG = false;

static inline bool isArgumentTuple(Node* node) {
    return isBranchNode(node) && isThisToken(node, ",");
}

static inline bool isNewline(Node* node) {
    return isLeafNode(node) && isThisToken(node, "\n");
}

static inline bool isOpenParen(Node* node) {
    return isLeafNode(node) && isThisToken(node, "(");
}

static inline bool isCloseParen(Node* node) {
    return isLeafNode(node) && isThisToken(node, ")");
}

static inline bool isEOF(Node* node) {
    return isLeafNode(node) && isThisToken(node, "\0");
}

bool isOperatorTop(Stack* stack) {
    return isEmpty(stack) || isOperator(peek(stack, 0));
}

bool isSpaceTop(Stack* stack) {
    return isEmpty(stack) || isSpace(peek(stack, 0));
}

static inline bool isOperand(Node* node) {
    return isName(node) || isInteger(node) || isReference(node) ||
        isApplication(node) || isLambda(node);
}

void eraseNewlines(Stack* stack) {
    while (!isEmpty(stack) && isNewline(peek(stack, 0)))
        release(pop(stack));
}

void pushOperand(Stack* stack, Node* node) {
    if (isOperatorTop(stack)) {
        push(stack, node);
    } else {
        Hold* left = pop(stack);
        push(stack, newApplication(getLocation(node), getNode(left), node));
        release(left);
    }
}

void collapseOperator(Stack* stack) {
    Hold* right = pop(stack);
    Hold* operator = pop(stack);
    Operator op = getOperator(getNode(operator), isOperatorTop(stack));
    Hold* left = op.associativity == P ? NULL : pop(stack);
    push(stack, op.collapse(getNode(operator), getNode(left), getNode(right)));
    release(right);
    release(operator);
    if (left != NULL)
        release(left);
}

bool isHigherPrecedence(Node* operator, Node* collapser, bool prefix) {
    Operator op = getOperator(operator, prefix);
    if (prefix && op.associativity != P)      // prefix operator not found
        return false;                         // don't try to collapse sections

    // collapser is preceded by an operand, so if it is an operator like '-'
    // that could be either prefix or infix, we assume that this case is infix
    Operator collapserOp = getOperator(collapser, false);
    syntaxErrorIf(collapserOp.associativity == N && op.rightPrecedence ==
        collapserOp.leftPrecedence, operator, "operator is non-associative");
    if (collapserOp.associativity == L || collapserOp.associativity == P)
        return op.rightPrecedence >= collapserOp.leftPrecedence;
    else
        return op.rightPrecedence > collapserOp.leftPrecedence;
}

bool shouldCollapseOperator(Stack* stack, Node* collapser) {
    // the operator that we are checking to collapse is at stack index 1
    if (isOperatorTop(stack)) {   // can't collapse with operator to the right
        syntaxErrorIf(isAlwaysInfixOperator(collapser)
            && !isCloseParen(collapser) && !isOpenParen(peek(stack, 0)),
            collapser, "missing left argument");
        return false;
    }
    syntaxErrorIf(!isOpenParen(collapser) && isAlwaysPrefixOperator(collapser),
            collapser, "space required before prefix operator");
    Node* operator = peek(stack, 1);    // must exist because top is not EOF
    return isOperator(operator) && !isEOF(operator) &&
        isHigherPrecedence(operator, collapser, isOperator(peek(stack, 2)));
}

void collapseLeftOperand(Stack* stack, Node* token) {
    // ( a op1 b op2 c op3 d ...
    // opN is guaranteed to be in non-decreasing order of precedence
    // we collapse right-associatively up to the first operator encountered
    // that has lower precedence than token or equal precedence if token
    // is right associative
    while (shouldCollapseOperator(stack, token))
        collapseOperator(stack);
}

Node* createSection(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(!isSectionableOperator(operator), operator,
            "invalid operator in section");
    syntaxErrorIf(!isOperand(left), left, "invalid operand in section");
    syntaxErrorIf(!isOperand(right), right, "invalid operand in section");
    return newLambda(getLocation(operator), PARAMETERX,
        getOperator(operator, false).collapse(operator, left, right));
}

Hold* createSectionWithHolds(Hold* operator, Hold* left, Hold* right) {
    Hold* result = hold(createSection(
        getNode(operator), getNode(left), getNode(right)));
    release(operator);
    release(left);
    release(right);
    return result;
}

Node* applyToArgumentTuple(Node* base, Node* arguments) {
    for (; isArgumentTuple(arguments); arguments = getRight(arguments))
        base = newApplication(getLocation(base), base, getLeft(arguments));
    return newApplication(getLocation(base), base, arguments);
}

void pushArgumentTuple(Stack* stack, Node* argumentTuple) {
    if (isSpaceTop(stack))
        release(pop(stack));
    syntaxErrorIf(isOperatorTop(stack), argumentTuple,
        "expected operand before argument tuple");
    Hold* function = pop(stack);
    push(stack, applyToArgumentTuple(getNode(function), argumentTuple));
    release(function);
}

void convertParenthesizedOperator(Node* operator) {
    syntaxErrorIf(!isParenthesizableOperator(operator),
        operator, "operator cannot be parenthesized");
    convertOperatorToName(operator);
}

Hold* popParentheses(Stack* stack, Node* close) {
    collapseLeftOperand(stack, close);
    Hold* right = pop(stack);
    syntaxErrorIf(isEOF(getNode(right)), close, "missing '(' for");
    syntaxErrorIf(isOpenParen(getNode(right)), close, "empty parentheses");
    Hold* left = pop(stack);
    syntaxErrorIf(isEOF(getNode(left)), close, "missing '(' for");
    if (isOpenParen(getNode(left))) {
        release(left);
        return right;
    } // at this point we know that left and right are not parentheses or EOF
    syntaxErrorIf(isOperator(getNode(right)) && isAlwaysPrefixOperator(
            getNode(right)), getNode(right), "invalid operator in section");
    Hold* open = pop(stack);
    syntaxErrorIf(!isOpenParen(getNode(open)), close, "bad section ending at");
    release(open);
    if (isOperator(getNode(right)))         // (1 +) ==> (x -> (1 + x))
        return createSectionWithHolds(right, left, hold(REFERENCEX));
    if (isOperator(getNode(left)))          // (+ 1) ==> (x -> (x + 1))
        return createSectionWithHolds(left, hold(REFERENCEX), right);
    syntaxErrorIf(true, close, "bad section ending at");
    return NULL;
}

void collapseParentheses(Stack* stack, Node* close) {
    eraseNewlines(stack);
    Hold* contents = popParentheses(stack, close);
    if (isOperator(getNode(contents)))
        convertParenthesizedOperator(getNode(contents));
    if (isArgumentTuple(getNode(contents)))
        pushArgumentTuple(stack, getNode(contents));
    else
        pushOperand(stack, getNode(contents));
    release(contents);
}

Hold* collapseEOF(Stack* stack, Hold* token) {
    eraseNewlines(stack);
    collapseLeftOperand(stack, getNode(token));
    syntaxErrorIf(isEOF(peek(stack, 0)), getNode(token), "no input");
    Hold* result = pop(stack);
    Node* end = peek(stack, 0);
    syntaxErrorIf(!isEOF(end), end, "illegal syntax near");
    deleteStack(stack);
    release(token);
    return result;
}

void pushOperator(Stack* stack, Node* operator) {
    if (!isAlwaysPrefixOperator(operator) && isSpaceTop(stack))
        release(pop(stack));
    if ((isSpace(operator) || isNewline(operator)) && isOperatorTop(stack))
        return;   // note: close parens never appear on the stack
    if (isCloseParen(operator)) {
        collapseParentheses(stack, operator);
    } else {
        collapseLeftOperand(stack, operator);
        push(stack, operator);
    }
}

void debugParseState(Node* token, Stack* stack) {
    if (DEBUG) {
        debug("Token: ");
        debugAST(token);
        debug("  Stack: ");
        debugStack(stack, NULL);
        debug("\n");
    }
}

Hold* parseString(const char* input) {
    Stack* stack = newStack(VOID);
    push(stack, newEOF());

    for (Hold* token = getFirstToken(input); true;
               token = replaceHold(token, getNextToken(token))) {
        debugParseState(getNode(token), stack);
        if (isEOF(getNode(token)))
            return collapseEOF(stack, token);
        if (isOperator(getNode(token)))
            pushOperator(stack, getNode(token));
        else
            pushOperand(stack, getNode(token));
    }
}

void debugStage(const char* label, Node* node) {
    if (DEBUG) {
        debugLine();
        debug(label);
        debug(": ");
        debugAST(node);
        debug("\n");
    }
}

Program parse(const char* input, bool optimize) {
    Hold* result = parseString(input);
    debugStage("Parsed", getNode(result));
    result = replaceHold(result, desugar(getNode(result)));
    debugStage("Desugared", getNode(result));
    return bind(result, optimize);
}
