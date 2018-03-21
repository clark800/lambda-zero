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
    return isOperator(peek(stack, 0));
}

bool isAlwaysPrefixOperator(Node* token) {
    return isOperator(token) && isPrefixOperator(getOperator(token, false));
}

void eraseNewlines(Stack* stack) {
    while (isNewline(peek(stack, 0)))
        release(pop(stack));
}

void eraseSpaces(Stack* stack) {
    while (isSpace(peek(stack, 0)))
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
    Hold* left = getOperatorArity(op) > 1 ? pop(stack) : NULL;
    push(stack, applyOperator(op, getNode(left), getNode(right)));
    release(right);
    release(operator);
    if (left != NULL)
        release(left);
}

bool shouldCollapseOperator(Stack* stack, Node* collapser) {
    // the operator that we are checking to collapse is at stack index 1
    if (isOperatorTop(stack)) {   // can't collapse with operator to the right
        syntaxErrorIf(!isPrefixOperator(getOperator(collapser, true))
            && !isCloseParen(collapser) && !isOpenParen(peek(stack, 0)),
            collapser, "missing left argument");
        return false;
    }

    syntaxErrorIf(!isOpenParen(collapser) && isAlwaysPrefixOperator(collapser),
        collapser, "space required before prefix operator");

    Node* operator = peek(stack, 1);    // must exist because top is not EOF
    if (!isOperator(operator) || isEOF(operator))
        return false;                   // can't collapse non-operator or EOF

    bool prefixOnly = isOperator(peek(stack, 2));
    Operator op = getOperator(operator, prefixOnly);
    if (prefixOnly && !isPrefixOperator(op))
        return false;   // default to false (don't collapse) to allow sections

    return isHigherPrecedence(op, getOperator(collapser, false));
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
    Operator op = getOperator(operator, false);
    syntaxErrorIf(isPrefixOperator(op) || isSpecialOperator(op),
        operator, "invalid operator in section");
    syntaxErrorIf(isOperator(left), left, "invalid operand in section");
    syntaxErrorIf(isOperator(right), right, "invalid operand in section");
    Node* body = applyOperator(op, left, right);
    return newLambda(getLocation(operator), PARAMETERX, body);
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
    eraseSpaces(stack);
    syntaxErrorIf(isOperatorTop(stack), argumentTuple,
        "expected operand before argument tuple");
    Hold* function = pop(stack);
    push(stack, applyToArgumentTuple(getNode(function), argumentTuple));
    release(function);
}

void convertParenthesizedOperator(Node* operator) {
    syntaxErrorIf(isSpecialOperator(getOperator(operator, false)),
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
    syntaxErrorIf(isAlwaysPrefixOperator(getNode(right)),
        getNode(right), "missing right argument");
    Hold* open = pop(stack);
    syntaxErrorIf(!isOpenParen(getNode(open)), close, "missing '(' for");
    release(open);
    if (isOperator(getNode(right)))         // (1 +) ==> (x -> (1 + x))
        return createSectionWithHolds(right, left, hold(REFERENCEX));
    if (isOperator(getNode(left)))          // (+ 1) ==> (x -> (x + 1))
        return createSectionWithHolds(left, hold(REFERENCEX), right);
    syntaxErrorIf(true, close, "unexpected syntax error before");
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
    syntaxErrorIf(!isEOF(end), end, "unexpected syntax error near");
    deleteStack(stack);
    release(token);
    return result;
}

void pushOperator(Stack* stack, Node* operator) {
    if (!isAlwaysPrefixOperator(operator))
        eraseSpaces(stack);
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
