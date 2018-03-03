#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "lex.h"
#include "operators.h"
#include "builtins.h"
#include "desugar.h"
#include "serialize.h"

void syntaxErrorIf(bool condition, Node* token, const char* message) {
    if (condition)
        throwTokenError("Syntax", message, token);
}

static inline bool isValidOperand(Node* node) {
    return isName(node) || isInteger(node) ||
        isApplication(node) || isAbstraction(node);
}

void eraseNewlines(Stack* stack) {
    while (!isEmpty(stack) && isNewline(peek(stack, 0)))
        release(pop(stack));
}

void pushLeftAssociative(Stack* stack, Node* node) {
    if (isEmpty(stack) || isOperator(peek(stack, 0))) {
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

bool shouldCollapseInfixOperator(Stack* stack, Operator collapser) {
    if (isEmpty(stack) || isOpenParen(peek(stack, 0)))
        return false;       // don't try to collapse parenthesized operators
    Node* operator = peekSafe(stack, 1);
    if (operator == NULL || !isOperator(operator))
        return false;
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
    Operator collapser = getOperator(token);
    while (shouldCollapseInfixOperator(stack, collapser))
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
    if (isEmpty(stack) || isOperator(peek(stack, 0))) {
        // create tuple
        push(stack, newLambda(location, PARAMETERX,
            applyToCommaTree(location, REFERENCEX, commaTree)));
    } else {
        // create function call
        Hold* function = pop(stack);
        push(stack, applyToCommaTree(location, getNode(function), commaTree));
        release(function);
    }
}

void convertParenthesizedOperator(Node* operator) {
    syntaxErrorIf(isSpecialOperator(operator) && !isComma(operator),
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
        pushLeftAssociative(stack, getNode(contents));
    release(contents);
}

bool isNewBlock(Stack* stack) {
    return isEmpty(stack) || isBlockOpener(peek(stack, 0));
}

Hold* parseString(const char* input) {
    Stack* stack = newStack(NULL);
    push(stack, newEOF());

    for (Hold* tokenHold = getFirstToken(input); true;
               tokenHold = replaceHold(tokenHold, getNextToken(tokenHold))) {
        Node* token = getNode(tokenHold);
        debugParseState(token, stack);
        if (isOperator(token)) {
            if (isOpenParen(token)) {
                push(stack, token);
            } else if (isCloseParen(token)) {
                collapseParentheses(stack, token);
            } else if (isEOF(token)) {
                eraseNewlines(stack);
                collapseLeftOperand(stack, token);
                syntaxErrorIf(isEOF(peek(stack, 0)), token, "no input");
                Hold* result = pop(stack);
                assert(isEOF(peek(stack, 0)));
                deleteStack(stack);
                release(tokenHold);
                return result;
            } else if (isDot(token)) {
                Hold* leftHold = pop(stack);
                Node* left = getNode(leftHold);
                syntaxErrorIf(!isValidOperand(left), left, "expected operand");
                tokenHold = replaceHold(tokenHold, getNextToken(tokenHold));
                Node* right = getNode(tokenHold);
                syntaxErrorIf(!isName(right), right, "expected function name");
                push(stack, newApplication(getLocation(token), right, left));
                release(leftHold);
            } else if (isBacktick(token)) {
                tokenHold = replaceHold(tokenHold, getNextToken(tokenHold));
                collapseLeftOperand(stack, getNode(tokenHold));
                push(stack, newOperator(getLocation(getNode(tokenHold))));
                tokenHold = replaceHold(tokenHold, getNextToken(tokenHold));
                syntaxErrorIf(!isBacktick(getNode(tokenHold)),
                    getNode(tokenHold), "expected backtick but got");
            } else if (!(isNewline(token) && isNewBlock(stack))) {
                collapseLeftOperand(stack, token);
                push(stack, token);
            } // ignore newlines at the beginning of a block
        } else {
            pushLeftAssociative(stack, token);
        }
    }
}

static unsigned long long findDebruijnIndex(Node* symbol, Stack* parameters) {
    unsigned long long i = 0;
    for (Iterator* it = iterate(parameters); !end(it); it = next(it), i++)
        if (isSameToken(cursor(it), symbol))
            return i + 1;
    return 0;
}

static void processSymbol(Node* symbol, Stack* parameterStack) {
    unsigned long long code = lookupBuiltinCode(symbol);
    if (code > 0) {
        convertSymbolToBuiltin(symbol, code);
        return;
    }
    unsigned long long index = findDebruijnIndex(symbol, parameterStack);
    syntaxErrorIf(index == 0, symbol, "undefined symbol");
    convertSymbolToReference(symbol, index);
}

static bool isDefined(Node* symbol, Stack* parameterStack) {
    // PARAMETERX is always considered to be a fresh variable so that we
    // can allow tuples inside tuples
    return (PARAMETERX == NULL || !isSameToken(symbol, PARAMETERX)) &&
        (lookupBuiltinCode(symbol) != 0 ||
        findDebruijnIndex(symbol, parameterStack) != 0);
}

static void process(Node* node, Stack* parameterStack) {
    if (isSymbol(node)) {
        processSymbol(node, parameterStack);
    } else if (isAbstraction(node)) {
        syntaxErrorIf(isDefined(getParameter(node), parameterStack),
            getParameter(node), "symbol already defined");
        push(parameterStack, getParameter(node));
        process(getBody(node), parameterStack);
        release(pop(parameterStack));
    } else if (isApplication(node)) {
        process(getLeft(node), parameterStack);
        process(getRight(node), parameterStack);
    } else {
        // allow references so we can paste parsed trees in with sugars
        assert(isReference(node) || isInteger(node));
    }
}

void preprocess(Node* root) {
    Stack* parameterStack = newStack(NULL);
    process(root, parameterStack);
    deleteStack(parameterStack);
}

Hold* parse(const char* input) {
    Hold* result = parseString(input);
    debugAST("Parsed", getNode(result));
    desugar(getNode(result));
    debugAST("Desugared", getNode(result));
    preprocess(getNode(result));
    debugAST("Preprocessed", getNode(result));
    return result;
}
