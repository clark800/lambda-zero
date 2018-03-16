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

static inline bool isCommaBranch(Node* node) {
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

static inline bool isValidOperand(Node* node) {
    return isName(node) || isInteger(node) ||
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

void collapseInfixOperator(Stack* stack) {
    Hold* right = pop(stack);
    Hold* operator = pop(stack);
    syntaxErrorIf(!isValidOperand(getNode(right)), getNode(operator),
        "invalid right operand of");
    syntaxErrorIf(isEmpty(stack), getNode(operator), "missing left operand of");
    Hold* left = pop(stack);
    syntaxErrorIf(!isValidOperand(getNode(left)), getNode(operator),
        "missing or invalid left operand of");
    push(stack, getOperator(getNode(operator)).collapse(
        getNode(operator), getNode(left), getNode(right)));
    release(left);
    release(right);
    release(operator);
}

bool shouldCollapseInfixOperator(Stack* stack, Node* collapserNode) {
    Operator collapser = getOperator(collapserNode);
    if (isEmpty(stack) || isOpenParen(peek(stack, 0)))
        return false;       // don't try to collapse parenthesized operators
    Node* operator = peekSafe(stack, 1);
    if (operator == NULL || !isOperator(operator))
        return false;
    syntaxErrorIf(isOpenParen(operator) && isEOF(collapserNode), operator,
        "missing close parenthesis for");
    Node* leftOperand = peekSafe(stack, 2);
    if (leftOperand == NULL || isOpenParen(leftOperand))
        return false;       // don't try to collapse sections
    Operator op = getOperator(operator);
    if (collapser.associativity == N)
        syntaxErrorIf(op.rightPrecedence == collapser.leftPrecedence,
            operator, "operator is non-associative");
    if (collapser.associativity == L)
        return op.rightPrecedence >= collapser.leftPrecedence;
    else
        return op.rightPrecedence > collapser.leftPrecedence;
}

void collapseLeftOperand(Stack* stack, Node* token) {
    // ( a op1 b op2 c op3 d ...
    // opN is guaranteed to be in non-decreasing order of precedence
    // we collapse right-associatively up to the first operator encountered
    // that has lower precedence than token or equal precedence if token
    // is right associative
    while (shouldCollapseInfixOperator(stack, token))
        collapseInfixOperator(stack);
}

void pushSection(Stack* stack, Node* operator, Node* left, Node* right) {
    syntaxErrorIf(isSpecialOperator(operator), operator,
        "invalid operator in section");
    push(stack, newLambda(getLocation(operator), PARAMETERX,
        getOperator(operator).collapse(operator, left, right)));
}

void collapseSection(Stack* stack, Node* closeParen) {
    Hold* right = pop(stack);
    Hold* left = pop(stack);
    if (isOperator(getNode(left)) && isValidOperand(getNode(right)))
        // (+ 1) ==> (x -> (x + 1))
        pushSection(stack, getNode(left), REFERENCEX, getNode(right));
    else if (isValidOperand(getNode(left)) && isOperator(getNode(right)))
        // (1 +) ==> (x -> (1 + x))
        pushSection(stack, getNode(right), getNode(left), REFERENCEX);
    else
        syntaxErrorIf(true, closeParen, "invalid section");
    release(left);
    release(right);
}

Node* applyToCommaTree(int location, Node* base, Node* commaTree) {
    for (; isCommaBranch(commaTree); commaTree = getRight(commaTree))
        base = newApplication(location, base, getLeft(commaTree));
    return newApplication(location, base, commaTree);
}

void collapseCommaTree(Stack* stack, Node* commaTree, int location) {
    syntaxErrorIf(isOperatorTop(stack), commaTree, "expected operand before");
    Hold* function = pop(stack);
    push(stack, applyToCommaTree(location, getNode(function), commaTree));
    release(function);
}

void convertParenthesizedOperator(Node* operator) {
    syntaxErrorIf(isSpecialOperator(operator),
        operator, "operator cannot be parenthesized");
    convertOperatorToName(operator);
}

void collapseParentheses(Stack* stack, Node* close) {
    eraseNewlines(stack);
    syntaxErrorIf(isOpenParen(peek(stack, 0)), close, "empty parentheses");
    collapseLeftOperand(stack, close);
    Node* third = peekSafe(stack, 2);
    if (third != NULL && isOpenParen(third) && !isOpenParen(peekSafe(stack, 1)))
        collapseSection(stack, close);
    Hold* contents = pop(stack);
    syntaxErrorIf(isEmpty(stack), close, "missing '(' for");
    Hold* openParen = pop(stack);
    syntaxErrorIf(!isOpenParen(getNode(openParen)), close, "missing '(' for");
    int location = getLocation(getNode(openParen));
    release(openParen);
    if (isOperator(getNode(contents)))
        convertParenthesizedOperator(getNode(contents));
    if (isCommaBranch(getNode(contents)))
        collapseCommaTree(stack, getNode(contents), location);
    else
        pushOperand(stack, getNode(contents));
    release(contents);
}

Hold* collapseEOF(Stack* stack, Hold* token) {
    eraseNewlines(stack);
    collapseLeftOperand(stack, getNode(token));
    syntaxErrorIf(isEOF(peek(stack, 0)), getNode(token), "no input");
    Hold* result = pop(stack);
    assert(isEOF(peek(stack, 0)));
    deleteStack(stack);
    release(token);
    return result;
}

void pushOperator(Stack* stack, Node* operator) {
    if (!isOpenParen(operator) && isSpaceTop(stack))
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

static inline void pushToken(Stack* stack, Node* token) {
    if (isOperator(token))
        pushOperator(stack, token);
    else
        pushOperand(stack, token);
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
        pushToken(stack, getNode(token));
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

Hold* parse(const char* input) {
    Hold* result = parseString(input);
    debugStage("Parsed", getNode(result));
    result = replaceHold(result, desugar(getNode(result)));
    debugStage("Desugared", getNode(result));
    bind(getNode(result));
    debugStage("Preprocessed", getNode(result));
    return result;
}
