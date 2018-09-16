#include "lib/tree.h"
#include "lib/array.h"
#include "lib/stack.h"
#include "ast.h"
#include "lex.h"
#include "errors.h"
#include "tokens.h"
#include "operators.h"
#include "syntax.h"
#include "bind.h"
#include "debug.h"
#include "parse.h"

bool TRACE_PARSING = false;

static void shiftOperand(Stack* stack, Node* node) {
    if (isEmpty(stack) || isOperator(peek(stack, 0))) {
        push(stack, node);
    } else {
        Hold* left = pop(stack);
        push(stack, newApplication(getTag(node), getNode(left), node));
        release(left);
    }
}

static void reduceLeft(Stack* stack) {
    Hold* right = pop(stack);
    Hold* op = pop(stack);
    Node* operator = getNode(op);
    if (getFixity(operator) == POST)
        syntaxError("missing left argument to", operator);
    Hold* left = getFixity(operator) == IN ? pop(stack) : NULL;
    shiftOperand(stack,
        reduceOperator(operator, getNode(left), getNode(right)));
    release(right);
    release(op);
    if (left != NULL)
        release(left);
}

static bool shouldReduce(Stack* stack, Node* collapser) {
    // the operator that we are checking to collapse is at stack index 1
    if (isOperator(peek(stack, 0)))
        return false;                   // don't collapse over another operator

    Node* operator = peek(stack, 1);    // must exist because top is not EOF
    if (!isOperator(operator) || isEOF(operator))
        return false;                   // can't collapse non-operator or EOF

    return isHigherPrecedence(operator, collapser);
}

static void reduce(Stack* stack, Node* operator) {
    if (getFixity(operator) != OPEN)
        if (isOperator(peek(stack, 0)) && isSpaceOperator(peek(stack, 0)))
            release(pop(stack));
    if (getFixity(operator) == CLOSE) {
        if (isNewline(peek(stack, 0)))
            release(pop(stack));
        if (isThisLeaf(peek(stack, 0), ";"))
            release(pop(stack));

        Node* top = peek(stack, 0);
        if (isCloseParen(operator) && isOperator(top))
            if (getFixity(top) == IN && !isSpecial(top))
                push(stack, newName(renameTag(getTag(top), "._")));
    }

    // ( a op1 b op2 c op3 d ...
    // opN is guaranteed to be in non-decreasing order of precedence
    // we collapse right-associatively up to the first operator encountered
    // that has lower precedence than token or equal precedence if token
    // is right associative
    while (shouldReduce(stack, operator))
        reduceLeft(stack);
}

static void shiftToken(Stack* stack, Token token) {
    Node* node = parseToken(token, stack);
    if (isOperator(node)) {
        reduce(stack, node);
        shiftOperator(stack, node);
        release(hold(node));    // some operators are never pushed to the stack
    } else
        shiftOperand(stack, node);
}

static Hold* parseString(const char* input, bool trace) {
    Stack* stack = newStack();
    push(stack, parseToken(newStartToken(), stack));
    for (Token token = lex(input); token.type != END; token = lex(skip(token))){
        debugParseState(token.tag, stack, trace);
        shiftToken(stack, token);
    }
    Hold* result = pop(stack);
    deleteStack(stack);
    return result;
}

void deleteProgram(Program program) {
    release(program.root);
    deleteArray(program.globals);
}

Program parse(const char* input) {
    initOperators();
    Hold* result = parseString(input, TRACE_PARSING);
    debugParseStage("parse", getNode(result), TRACE_PARSING);
    Array* globals = bind(result);
    debugParseStage("bind", getNode(result), TRACE_PARSING);
    Node* entry = elementAt(globals, length(globals) - 1);
    return (Program){result, entry, globals};
}
