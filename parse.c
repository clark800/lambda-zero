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
    return isOperator(peek(stack, 0));
}

bool isOpen(Node* token) {
    return isOperator(token) && isOpenOperator(getOperator(token, false));
}

void eraseNewlines(Stack* stack) {
    while (isNewline(peek(stack, 0)))
        release(pop(stack));
}

Node* applyToCommaTuple(Node* base, Node* arguments) {
    for (; isCommaTuple(arguments); arguments = getRight(arguments))
        base = newApplication(getLocation(base), base, getLeft(arguments));
    return newApplication(getLocation(base), base, arguments);
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

void pushBracketOperand(Stack* stack, Node* node) {
    if (isCommaTuple(node)) {
        if (isOperatorTop(stack))
            syntaxError("expected operand before arguments", peek(stack, 0));
        Hold* function = pop(stack);
        push(stack, applyToCommaTuple(getNode(function), node));
        release(function);
    } else {
        pushOperand(stack, node);
    }
}

void collapseOperator(Stack* stack) {
    Hold* right = pop(stack);
    Hold* operator = pop(stack);
    Operator op = getOperator(getNode(operator), isOperatorTop(stack));
    Hold* left = getOperatorArity(op) > 1 ? pop(stack) : NULL;
    pushOperand(stack, applyOperator(op, getNode(left), getNode(right)));
    release(right);
    release(operator);
    if (left != NULL)
        release(left);
}

void validateConsecutiveOperators(Node* left, Node* right) {
    syntaxErrorIf(isOpen(left) && isEOF(right), left, "missing close for");
    Operator op = getOperator(right, isOperator(left));
    if (isCloseOperator(op) && !isCloseParen(right) && !isOpen(left))
        syntaxError("missing right argument", left);
    if (isInfixOperator(op) && !isOpenParen(left))
        syntaxError("missing left argument", right);   // e.g. "5 - * 2"
}

void validateOperator(Stack* stack, Node* operator) {
    if (isOperatorTop(stack))
        validateConsecutiveOperators(peek(stack, 0), operator);
}

bool shouldCollapseOperator(Stack* stack, Node* collapser) {
    // the operator that we are checking to collapse is at stack index 1
    Node* right = peek(stack, 0);
    if (isOperator(right))
        return false;

    Node* operator = peek(stack, 1);    // must exist because top is not EOF
    if (!isOperator(operator) || isEOF(operator))
        return false;                   // can't collapse non-operator or EOF

    Node* left = peek(stack, 2);
    Operator op = getOperator(operator, isOperator(left));
    if (isOperator(left) && !isOpenOperator(op) && !isPrefixOperator(op))
        return false;

    return isHigherPrecedence(op, getOperator(collapser, false));
}

void collapseLeftOperand(Stack* stack, Node* collapser) {
    // ( a op1 b op2 c op3 d ...
    // opN is guaranteed to be in non-decreasing order of precedence
    // we collapse right-associatively up to the first operator encountered
    // that has lower precedence than token or equal precedence if token
    // is right associative
    validateOperator(stack, collapser);
    while (shouldCollapseOperator(stack, collapser))
        collapseOperator(stack);
}

Node* newSection(Node* operator, Node* left, Node* right) {
    Operator op = getOperator(operator, false);
    if (isPrefixOperator(op) || isSpecialOperator(op))
        syntaxError("invalid operator in section", operator);
    Node* body = applyOperator(op, left, right);
    return newLambda(getLocation(operator), getParameter(IDENTITY), body);
}

Node* constructSection(Node* left, Node* right) {
    if (isOperator(left) == isOperator(right))   // e.g. right is prefix
        syntaxError("invalid operator in section", right);
    return isOperator(left) ? newSection(left, getBody(IDENTITY), right) :
                              newSection(right, left, getBody(IDENTITY));
}

Hold* collapseSection(Stack* stack, Node* right) {
    Hold* left = pop(stack);
    Hold* result = hold(constructSection(getNode(left), right));
    release(left);
    return result;
}

void collapseBracket(Stack* stack, Operator op) {
    Node* close = op.token;
    syntaxErrorIf(isEOF(peek(stack, 0)), close, "missing open for");
    Hold* contents = pop(stack);
    if (isOpen(getNode(contents))) {
        pushBracketOperand(stack, applyOperator(op, getNode(contents), NULL));
    } else {
        syntaxErrorIf(isEOF(peek(stack, 0)), close, "missing open for");
        Node* right = getNode(contents);
        if (isCloseParen(close) && !isOpenParen(peek(stack, 0)))
            contents = replaceHold(contents, collapseSection(stack, right));
        Hold* open = pop(stack);
        pushBracketOperand(stack,
            applyOperator(op, getNode(open), getNode(contents)));
        release(open);
    }
    release(contents);
}

void pushOperator(Stack* stack, Node* operator) {
    if (isNewline(operator) && isOperatorTop(stack))
        return;   // note: close parens never appear on the stack
    Operator op = getOperator(operator, isOperatorTop(stack));
    if (isCloseOperator(op))
        eraseNewlines(stack);

    collapseLeftOperand(stack, operator);
    if (isCloseOperator(op))
        collapseBracket(stack, op);
    else
        push(stack, operator);
}

Hold* collapseEOF(Stack* stack, Hold* token) {
    eraseNewlines(stack);
    collapseLeftOperand(stack, getNode(token));
    syntaxErrorIf(isEOF(peek(stack, 0)), getNode(token), "no input");
    Hold* result = pop(stack);
    Node* end = peek(stack, 0);
    syntaxErrorIf(!isEOF(end), end, "unexpected syntax error near");
    deleteStack(stack);
    release(token);
    return result;
}

void debugParseState(Node* token, Stack* stack, bool showDebug) {
    if (showDebug) {
        debug("Token: '");
        debugAST(token);
        debug("'  Stack: ");
        debugStack(stack, NULL);
        debug("\n");
    }
}

Hold* parseString(const char* input, bool showDebug) {
    Stack* stack = newStack(VOID);
    push(stack, newEOF());
    Hold* token = getFirstToken(input);
    for (;; token = replaceHold(token, getNextToken(token))) {
        debugParseState(getNode(token), stack, showDebug);
        if (isEOF(getNode(token)))
            return collapseEOF(stack, token);
        if (isOperator(getNode(token)))
            pushOperator(stack, getNode(token));
        else
            pushOperand(stack, getNode(token));
    }
}

void debugStage(const char* label, Node* node, bool showDebug) {
    if (showDebug) {
        debugLine();
        debug(label);
        debug(": ");
        debugAST(node);
        debug("\n");
    }
}

Program parse(const char* input, bool optimize, bool showDebug) {
    Hold* result = parseString(input, showDebug);
    debugStage("Parsed", getNode(result), showDebug);
    result = replaceHold(result, desugar(getNode(result)));
    debugStage("Desugared", getNode(result), showDebug);
    return bind(result, optimize);
}
