#include <stdlib.h>     // exit
#include <stdio.h>      // fputs
#include <limits.h>     // LLONG_MAX
#include "lib/util.h"   // error
#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "errors.h"
#include "closure.h"

static bool STDERR = false;
Stack* INPUT_STACK;

static void printBacktrace(Closure* closure) {
    fputs("\n\nBacktrace:\n", stderr);
    Stack* backtrace = (Stack*)getBacktrace(closure);
    for (Iterator* it = iterate(backtrace); !end(it); it = next(it))
        printTagLine(getTag(cursor(it)), "");
}

static void printRuntimeError(const char* message, Closure* closure) {
    if (!TEST && !isEmpty((Stack*)getBacktrace(closure)))
        printBacktrace(closure);
    printError("\nRuntime", message, getTag(getTerm(closure)));
}

void runtimeError(const char* message, Closure* closure) {
    printRuntimeError(message, closure);
    exit(1);
}

// note: it is important to check for overflow before it occurs because
// undefined behavior occurs immediately after an overflow, which is
// impossible to recover from
static long long add(long long left, long long right, Closure* builtin) {
    if (left > LLONG_MAX - right)
        runtimeError("overflow in", builtin);
    return left + right;
}

static long long subtract(long long left, long long right, Closure* builtin) {
    if (right > left)
        runtimeError("underflow in", builtin);
    return left - right;
}

static long long multiply(long long left, long long right, Closure* builtin) {
    if (right != 0 && left > LLONG_MAX / right)
        runtimeError("overflow in", builtin);
    return left * right;
}

static long long divide(long long left, long long right, Closure* builtin) {
    if (right == 0)
        runtimeError("divide by zero in", builtin);
    return left / right;
}

unsigned int getBuiltinArity(Node* builtin) {
    switch (getBuiltinCode(builtin)) {
        case UNDEFINED: return 0;
        case ERROR: return 1;
        case EXIT: return 1;
        case INCREMENT: return 1;
        case PUT: return 1;
        default: return 2;
    }
}

static long long getNatural(Node* builtin, Node* natural) {
    // NULL is allowed for unary operators like increment
    if (natural != NULL && !isNatural(natural))
        runtimeError("expected natural argument to", builtin);
    return natural == NULL ? 0 : getValue(natural);
}

static Hold* evaluateError(Closure* builtin, Closure* message) {
    if (!TEST) {
        printRuntimeError("hit", builtin);
        fputc((int)'\n', stderr);
    }
    if (message == NULL) {
        fputs("undefined", stderr);
        exit(1);
    }
    STDERR = true;
    return hold(message);
}

static Node* evaluatePut(Closure* builtin, Node* left) {
    long long c = getNatural(builtin, left);
    if (c < 0 || c >= 256)
        runtimeError("expected byte value in list returned from main", builtin);
    fputc((int)c, STDERR ? stderr : stdout);
    Tag tag = getTag(getTerm(builtin));
    return newLambda(tag, newBlank(tag), newBlankReference(tag, 1));
}

static Node* evaluateGet(Closure* builtin, Node* left, Node* right) {
    static long long inputIndex = 0;
    long long index = getNatural(builtin, left);
    assert(index <= inputIndex);
    if (index < inputIndex)
        return peek(INPUT_STACK, (size_t)(inputIndex - index - 1));
    inputIndex += 1;
    int c = fgetc(stdin);
    if (c == EOF) {
        Node* nilGlobal = getRight(getLeft(getBody(right)));
        push(INPUT_STACK, nilGlobal);
    } else {
        // push ((::) c get(n + 1, globals))
        Tag tag = getTag(getTerm(builtin));
        Node* prependGlobal = getRight(getBody(right));
        Node* nextIndex = newNatural(tag, index + 1);
        Node* getIndex = newApplication(tag, getTerm(builtin), nextIndex);
        Node* tail = newApplication(tag, getIndex, right);
        Node* prependC = newApplication(tag, prependGlobal, newNatural(tag, c));
        push(INPUT_STACK, newApplication(tag, prependC, tail));
    }
    return peek(INPUT_STACK, 0);
}

static Node* computeBuiltin(Closure* builtin, long long left, long long right) {
    Tag tag = getTag(getTerm(builtin));
    switch (getBuiltinCode(getTerm(builtin))) {
        case INCREMENT: return newNatural(tag, left + 1);
        case PLUS: return newNatural(tag, add(left, right, builtin));
        case MINUS: return newNatural(tag, subtract(left, right, builtin));
        case TIMES: return newNatural(tag, multiply(left, right, builtin));
        case DIVIDE: return newNatural(tag, divide(left, right, builtin));
        case MODULO: return newNatural(tag, right == 0 ? left : left % right);
        case EQUAL: return newBoolean(tag, left == right);
        case NOTEQUAL: return newBoolean(tag, left != right);
        case LESSTHAN: return newBoolean(tag, left < right);
        case GREATERTHAN: return newBoolean(tag, left > right);
        case LESSTHANOREQUAL: return newBoolean(tag, left <= right);
        case GREATERTHANOREQUAL: return newBoolean(tag, left >= right);
    }
    assert(false);
    return NULL;
}

static Hold* makeResult(Closure* builtin, Node* node) {
    return hold(newClosure(node, VOID, getTrace(builtin)));
}

static Hold* evaluateOperator(Closure* builtin, Node* left, Node* right) {
    long long leftValue = getNatural(builtin, left);
    long long rightValue = getNatural(builtin, right);
    return makeResult(builtin, computeBuiltin(builtin, leftValue, rightValue));
}

static Hold* evaluateBuiltin(Closure* builtin, Node* left, Node* right) {
    switch (getBuiltinCode(getTerm(builtin))) {
        case EXIT: return error("\n");
        case UNDEFINED: return evaluateError(builtin, NULL);
        case PUT: return makeResult(builtin, evaluatePut(builtin, left));
        case GET: return makeResult(builtin, evaluateGet(builtin, left, right));
        default: return evaluateOperator(builtin, left, right);
    }
}

Hold* evaluateBuiltinNode(Closure* builtin, Closure* left, Closure* right) {
    if (getBuiltinCode(getTerm(builtin)) == ERROR)
        return evaluateError(builtin, left);
    switch (getBuiltinArity(getTerm(builtin))) {
        case 0: return evaluateBuiltin(builtin, NULL, NULL);
        case 1: return evaluateBuiltin(builtin, getTerm(left), NULL);
        default: return evaluateBuiltin(builtin, getTerm(left), getTerm(right));
    }
}
