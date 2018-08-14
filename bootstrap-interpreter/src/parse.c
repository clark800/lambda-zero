#include "lib/tree.h"
#include "lib/array.h"
#include "lib/stack.h"
#include "ast.h"
#include "errors.h"
#include "lex.h"
#include "quotes.h"
#include "operators.h"
#include "desugar.h"
#include "bind.h"
#include "debug.h"
#include "parse.h"

bool TRACE_PARSING = false;

bool isOperatorTop(Stack* stack) {
    return isOperator(peek(stack, 0));
}

bool isOpenOperator(Node* token) {
    return isOperator(token) &&
        (getFixity(token) == OPEN || getFixity(token) == OPENCALL);
}

void eraseNewlines(Stack* stack) {
    while (isNewline(peek(stack, 0)))
        release(pop(stack));
}

void pushOperand(Stack* stack, Node* node) {
    if (isOperatorTop(stack)) {
        push(stack, node);
        return;
    }
    Hold* left = pop(stack);
    push(stack, newApplication(getTag(node), getNode(left), node));
    release(left);
}

void collapseOperator(Stack* stack) {
    Hold* right = pop(stack);
    Hold* op = pop(stack);
    Node* operator = getNode(op);
    Hold* left = getFixity(operator) == IN ? pop(stack) : NULL;
    pushOperand(stack, applyOperator(operator, getNode(left), getNode(right)));
    release(right);
    release(op);
    if (left != NULL)
        release(left);
}

void validateConsecutiveOperators(Node* left, Node* right) {
    if (isOpenOperator(left) && isEOF(right))
        syntaxError("missing close for", left);
    if (getFixity(right) == CLOSE &&
            !isCloseParen(right) && !isOpenOperator(left))
        syntaxError("missing right argument to", left);
    if (getFixity(right) == IN && !isOpenParen(left))
        syntaxError("missing left argument to", right);   // e.g. "5 - * 2"
}

void validateOperator(Stack* stack, Node* operator) {
    if (isOperatorTop(stack))
        validateConsecutiveOperators(peek(stack, 0), operator);
}

bool shouldCollapseOperator(Stack* stack, Node* collapser) {
    // the operator that we are checking to collapse is at stack index 1
    if (isOperator(peek(stack, 0)))
        return false;                   // don't collapse over another operator

    Node* operator = peek(stack, 1);    // must exist because top is not EOF
    if (!isOperator(operator) || isEOF(operator))
        return false;                   // can't collapse non-operator or EOF

    if (getFixity(operator) == IN && isOperator(peek(stack, 2)))
        return false;                   // don't collapse section operators

    return isHigherPrecedence(operator, collapser);
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
    if (getFixity(operator) == PRE || isSpecialOperator(operator))
        syntaxError("invalid operator in section", operator);
    Node* body = applyOperator(operator, left, right);
    return newLambda(getTag(operator), newBlank(getTag(operator)), body);
}

Node* constructSection(Node* left, Node* right) {
    if (isOperator(left) == isOperator(right))   // e.g. right is prefix
        syntaxError("invalid operator in section", right);
    return isOperator(left) ?
        newSection(left, newBlankReference(getTag(left), 1), right) :
        newSection(right, left, newBlankReference(getTag(right), 1));
}

Hold* collapseSection(Stack* stack, Node* right) {
    Hold* left = pop(stack);
    Hold* result = hold(constructSection(getNode(left), right));
    release(left);
    return result;
}

void collapseBracket(Stack* stack, Node* close) {
    syntaxErrorIf(isEOF(peek(stack, 0)), "missing open for", close);
    Hold* contents = pop(stack);
    if (isOpenOperator(getNode(contents))) {
        pushOperand(stack, applyOperator(close, getNode(contents), NULL));
    } else {
        syntaxErrorIf(isEOF(peek(stack, 0)), "missing open for", close);
        Node* right = getNode(contents);
        if (isCloseParen(close) && !isOpenParen(peek(stack, 0)))
            contents = replaceHold(contents, collapseSection(stack, right));
        Hold* open = pop(stack);
        pushOperand(stack,
            applyOperator(close, getNode(open), getNode(contents)));
        release(open);
    }
    release(contents);
}

void pushOperator(Stack* stack, Node* operator) {
    // ignore spaces before operators except open operators
    if (isSpace(peek(stack, 0))) {
        release(pop(stack));
        if (getFixity(operator) == PRE)
            setRules(operator, isOperatorTop(stack));
    }

    // ignore spaces and newlines after operators
    // note: close parens never appear on the stack
    if ((isNewline(operator) || isSpace(operator)) && isOperatorTop(stack))
        return;

    if (getFixity(operator) == CLOSE) {
        eraseNewlines(stack);       // ignore newlines before close operators
        if (isComma(peek(stack, 0)) && !isOpenOperator(peek(stack, 1)))
            release(pop(stack));        // ignore commas before close operators
    }

    collapseLeftOperand(stack, operator);

    if (getFixity(operator) == OPENCALL) {
        Hold* top = pop(stack);
        push(stack, operator);
        push(stack, getNode(top));
        push(stack, setRules(newComma(getTag(operator)), false));
        release(top);
    } else if (getFixity(operator) == CLOSE)
        collapseBracket(stack, operator);
    else
        push(stack, operator);
}

Hold* collapseEOF(Stack* stack, Hold* token) {
    eraseNewlines(stack);
    syntaxErrorIf(isEOF(peek(stack, 0)), "no input", getNode(token));
    collapseLeftOperand(stack, getNode(token));
    Hold* result = pop(stack);
    if (isCommaList(getNode(result)))
        syntaxError("comma not inside brackets", getNode(result));
    Node* end = peek(stack, 0);
    syntaxErrorIf(!isEOF(end), "unexpected syntax error near", end);
    deleteStack(stack);
    release(token);
    return result;
}

void debugParseState(Node* token, Stack* stack, bool trace) {
    if (trace) {
        debug("Token: '");
        debugAST(token);
        debug("'  Stack: ");
        debugStack(stack, NULL);
        debug("\n");
    }
}

Node* parseOperand(Node* token) {
    switch (getTag(token).lexeme.start[0]) {
        case '"': return newStringLiteral(getTag(token));
        case '\'': return newCharacterLiteral(getTag(token));
        default: return token;
    }
}

Hold* parseString(const char* input, bool trace) {
    Stack* stack = newStack();
    push(stack, setRules(newEOF(), false));
    Hold* token = getFirstToken(input);
    for (;; token = replaceHold(token, getNextToken(token))) {
        debugParseState(getNode(token), stack, trace);
        if (isOperator(getNode(token))) {
            setRules(getNode(token), isOperatorTop(stack));
            if (isEOF(getNode(token)))
                return collapseEOF(stack, token);
            pushOperator(stack, getNode(token));
        } else {
            pushOperand(stack, parseOperand(getNode(token)));
        }
    }
}

void debugStage(const char* label, Node* node, bool trace) {
    if (trace) {
        debugLine();
        debug(label);
        debug(": ");
        debugAST(node);
        debug("\n");
    }
}

bool isIO(Program program) {
    return isApplication(program.entry) &&
        isApplication(getLeft(program.entry)) &&
        isGlobal(getLeft(getLeft(program.entry))) &&
        isThisToken(getLeft(getLeft(program.entry)), "main");
}

void deleteProgram(Program program) {
    release(program.root);
    deleteArray(program.globals);
}

Program parse(const char* input) {
    Hold* result = parseString(input, TRACE_PARSING);
    debugStage("Parsed", getNode(result), TRACE_PARSING);
    result = replaceHold(result, desugar(getNode(result)));
    debugStage("Desugared", getNode(result), TRACE_PARSING);
    Array* globals = bind(result);
    Node* entry = elementAt(globals, length(globals) - 1);
    return (Program){result, entry, globals};
}
