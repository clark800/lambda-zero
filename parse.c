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

void pushLeftAssociative(Stack* stack, Node* node) {
    if (isEmpty(stack)) {
        push(stack, node);
    } else {
        if (isOperator(peek(stack, 0))) {
            push(stack, node);
        } else {
            Hold* left = pop(stack);
            push(stack, newApplication(getLocation(node), getNode(left), node));
            release(left);
        }
    }
}

void collapseOperator(Stack* stack, Node* operator, Node* left, Node* right) {
    push(stack, getOperator(operator).collapse(operator, left, right));
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
    collapseOperator(stack, getNode(operator), getNode(left), getNode(right));
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
    collapseOperator(stack, operator, left, right);
    Hold* body = pop(stack);
    push(stack, newLambda(getLocation(operator), PARAMETERX, getNode(body)));
    release(body);
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

int collapseParentheses(Stack* stack, Node* token) {
    if (!isEmpty(stack) && isNewline(peek(stack, 0)))
        release(pop(stack));    // ignore newline before close paren
    syntaxErrorIf(isEmpty(stack), token, "missing '(' for");
    syntaxErrorIf(isOpenParen(peek(stack, 0)), token, "empty parentheses");
    collapseLeftOperand(stack, token);
    Node* third = peekSafe(stack, 2);
    if (third != NULL && isOpenParen(third) && !isOpenParen(peekSafe(stack, 1)))
        collapseSection(stack, token);
    Hold* result = pop(stack);
    syntaxErrorIf(isEmpty(stack), token, "missing '(' for");
    Hold* openParen = pop(stack);
    syntaxErrorIf(!isOpenParen(getNode(openParen)), token, "missing '(' for");
    int location = getLocation(getNode(openParen));
    release(openParen);
    if (isOperator(getNode(result))) {
        syntaxErrorIf(isSpecialOperator(getNode(result)), getNode(result),
            "operator cannot be parenthesized");
        convertOperatorToName(getNode(result));
    }
    pushLeftAssociative(stack, getNode(result));
    release(result);
    return location;
}

bool isNewBlock(Stack* stack) {
    return isEmpty(stack) || isBlockOpener(peek(stack, 0));
}

Hold* parseString(const char* input) {
    Stack* stack = newStack(NULL);
    push(stack, newOperator(-1));   // open parenthesis

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
                collapseParentheses(stack, token);
                release(tokenHold);
                break;
            } else if (isComma(token)) {
                int openLocation = collapseParentheses(stack, token);
                push(stack, newOperator(openLocation));
            } else if (isBacktick(token)) {
                tokenHold = replaceHold(tokenHold, getNextToken(tokenHold));
                token = getNode(tokenHold);
                collapseLeftOperand(stack, token);
                push(stack, newOperator(getLocation(token)));
                tokenHold = replaceHold(tokenHold, getNextToken(tokenHold));
                token = getNode(tokenHold);
                syntaxErrorIf(!isBacktick(token), token,
                    "expected backtick but got");
            } else if (isNewline(token) && isNewBlock(stack)) {
                continue;   // ignore newlines at start or following open paren
            } else {
                collapseLeftOperand(stack, token);
                push(stack, token);
            }
        } else {
            pushLeftAssociative(stack, token);
        }
    }

    syntaxErrorIf(isEmpty(stack), NULL, "no input");
    Hold* result = pop(stack);
    syntaxErrorIf(!isEmpty(stack), NULL, "extra '('");
    deleteStack(stack);
    return result;
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
    } else {
        unsigned long long debruijn = findDebruijnIndex(symbol, parameterStack);
        syntaxErrorIf(debruijn == 0, symbol, "undefined symbol");
        convertSymbolToReference(symbol, debruijn);
    }
}

static void process(Node* node, Stack* parameterStack) {
    if (isSymbol(node)) {
        processSymbol(node, parameterStack);
    } else if (isAbstraction(node)) {
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
