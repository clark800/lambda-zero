#include <stddef.h>
#include <stdbool.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "objects.h"
#include "errors.h"
#include "lex.h"
#include "operators.h"
#include "desugar.h"
#include "bind.h"
#include "serialize.h"
#include "parse.h"

bool isOperatorTop(Stack* stack) {
    return isOperator(peek(stack, 0));
}

bool isOpenOperator(Node* token) {
    return isOperator(token) && getFixity(getOperator(token, false)) == OPEN;
}

void eraseNewlines(Stack* stack) {
    while (isNewline(peek(stack, 0)))
        release(pop(stack));
}

Node* applyToCommaList(Node* base, Node* arguments) {
    if (!isCommaList(getLeft(arguments)))
        return newApplication(getLocation(base), base, getRight(arguments));
    return newApplication(getLocation(base),
            applyToCommaList(base, getLeft(arguments)), getRight(arguments));
}

void pushOperand(Stack* stack, Node* node) {
    if (isOperatorTop(stack)) {
        push(stack, node);
        return;
    }
    Hold* left = pop(stack);
    if (isTuple(node) && isApplication(getBody(node)))
        push(stack, applyToCommaList(getNode(left), getBody(node)));
    else if (isList(node)) {
        if (!isApplication(getBody(node)))
            syntaxError("missing argument to", node);
        if (isApplication(getBody(getRight(getBody(node)))))
            syntaxError("too many arguments to", node);
        push(stack, infix(newName(getLocation(node)), getNode(left),
            getRight(getLeft(getBody(node)))));
    } else
        push(stack, newApplication(getLocation(node), getNode(left), node));
    release(hold(node));
    release(left);
}

void collapseOperator(Stack* stack) {
    Hold* right = pop(stack);
    Hold* operator = pop(stack);
    Operator op = getOperator(getNode(operator), isOperatorTop(stack));
    Hold* left = getFixity(op) == IN ? pop(stack) : NULL;
    pushOperand(stack, applyOperator(op, getNode(left), getNode(right)));
    release(right);
    release(operator);
    if (left != NULL)
        release(left);
}

void validateConsecutiveOperators(Node* left, Node* right) {
    if (isOpenOperator(left) && isEOF(right))
        syntaxError("missing close for", left);
    Operator op = getOperator(right, isOperator(left));
    if (getFixity(op) == CLOSE && !isCloseParen(right) && !isOpenOperator(left))
        syntaxError("missing right argument to", left);
    if (getFixity(op) == IN && !isOpenParen(left))
        syntaxError("missing left argument to", right);   // e.g. "5 - * 2"
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
    if (isOperator(left) && getFixity(op) != OPEN && getFixity(op) != PRE)
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
    if (getFixity(op) == PRE || isSpecialOperator(op))
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
    syntaxErrorIf(isEOF(peek(stack, 0)), "missing open for", close);
    Hold* contents = pop(stack);
    if (isOpenOperator(getNode(contents))) {
        pushOperand(stack, applyOperator(op, getNode(contents), NULL));
    } else {
        syntaxErrorIf(isEOF(peek(stack, 0)), "missing open for", close);
        Node* right = getNode(contents);
        if (isCloseParen(close) && !isOpenParen(peek(stack, 0)))
            contents = replaceHold(contents, collapseSection(stack, right));
        Hold* open = pop(stack);
        pushOperand(stack, applyOperator(op, getNode(open), getNode(contents)));
        release(open);
    }
    release(contents);
}

void pushOperator(Stack* stack, Node* operator) {
    // ignore spaces before operators
    if (isSpace(peek(stack, 0)) && !isOpenOperator(operator))
        release(pop(stack));
    // ignore spaces and newlines after operators
    // note: close parens never appear on the stack
    if ((isNewline(operator) || isSpace(operator)) && isOperatorTop(stack))
        return;
    Operator op = getOperator(operator, isOperatorTop(stack));
    if (getFixity(op) == CLOSE)
        eraseNewlines(stack);

    collapseLeftOperand(stack, operator);
    if (getFixity(op) == CLOSE)
        collapseBracket(stack, op);
    else
        push(stack, operator);
}

Hold* collapseEOF(Stack* stack, Hold* token) {
    eraseNewlines(stack);
    syntaxErrorIf(isEOF(peek(stack, 0)), "no input", getNode(token));
    collapseLeftOperand(stack, getNode(token));
    Hold* result = pop(stack);
    Node* end = peek(stack, 0);
    syntaxErrorIf(!isEOF(end), "unexpected syntax error near", end);
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
