#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "lib/util.h"
#include "ast.h"
#include "errors.h"
#include "closure.h"
#include "builtins.h"

bool STDERR = false;
Stack* INPUT_STACK;

void printBacktrace(Closure* closure) {
    fputs("\n\nBacktrace:\n", stderr);
    Stack* backtrace = (Stack*)getBacktrace(closure);
    for (Iterator* it = iterate(backtrace); !end(it); it = next(it))
        printTagLine(getTag(cursor(it)), "");
}

void printRuntimeError(const char* message, Closure* closure) {
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
long long add(long long left, long long right, Closure* builtin) {
    if (left > 0 && right > 0 && left > LLONG_MAX - right)
        runtimeError("integer overflow in", builtin);
    if (left < 0 && right < 0 && left < -LLONG_MAX - right)
        runtimeError("integer overflow in", builtin);
    return left + right;
}

long long subtract(long long left, long long right, Closure* builtin) {
    if (left > 0 && right < 0 && left > LLONG_MAX + right)
        runtimeError("integer overflow in", builtin);
    if (left < 0 && right > 0 && left < -LLONG_MAX + right)
        runtimeError("integer overflow in", builtin);
    return left - right;
}

long long multiply(long long left, long long right, Closure* builtin) {
    if (right != 0 && llabs(left) > llabs(LLONG_MAX / right))
        runtimeError("integer overflow in", builtin);
    return left * right;
}

long long divide(long long left, long long right, Closure* builtin) {
    if (right == 0)
        runtimeError("divide by zero in", builtin);
    return left / right;
}

long long modulo(long long left, long long right, Closure* builtin) {
    if (right == 0)
        runtimeError("divide by zero in", builtin);
    return left % right;
}

unsigned int getBuiltinArity(Node* builtin) {
    switch (getValue(builtin)) {
        case ERROR: return 1;
        case EXIT: return 1;
        case PUT: return 1;
        default: return 2;
    }
}

bool isStrictArgument(Node* builtin, unsigned int i) {
    return !(getValue(builtin) == ERROR && i == 0);
}

Hold* evaluateError(Closure* builtin, Closure* message) {
    STDERR = true;
    if (!TEST) {
        printRuntimeError("hit", builtin);
        fputc((int)'\n', stderr);
    }
    Tag tag = getTag(getTerm(builtin));
    Node* exit = newBuiltin(tag, EXIT);
    Node* print = newApplication(tag, getTerm(message), newPrinter(tag));
    setTerm(builtin, newApplication(tag, exit, print));
    return hold(builtin);
}

Node* evaluatePut(Closure* builtin, long long c) {
    if (c < 0 || c >= 256)
        runtimeError("expected byte value in list returned from main", builtin);
    fputc((int)c, STDERR ? stderr : stdout);
    Tag tag = getTag(getTerm(builtin));
    return newLambda(tag, newBlank(tag), newBlankReference(tag, 1));
}

long long getIntegerArgument(Node* builtin, Closure* closure) {
    if (closure == NULL)
        return 0;       // this happens for single argument builtins like GET
    Node* integer = getTerm(closure);
    if (!isInteger(integer))
       runtimeError("expected integer argument to", builtin);
    return getValue(integer);
}

Hold* makeResult(Closure* builtin, Node* node) {
    return hold(newClosure(node, VOID, getTrace(builtin)));
}

Node* evaluateGet(Closure* builtin, Closure* left, Closure* right) {
    static long long inputIndex = 0;
    long long index = getIntegerArgument(builtin, left);
    assert(index <= inputIndex);
    if (index < inputIndex)
        return peek(INPUT_STACK, (size_t)(inputIndex - index - 1));
    inputIndex += 1;
    int c = fgetc(stdin);
    if (c == EOF) {
        Node* nilGlobal = getRight(getLeft(getBody(getTerm(right))));
        push(INPUT_STACK, nilGlobal);
    } else {
        Tag tag = getTag(getTerm(builtin));
        Node* prependGlobal = getRight(getBody(getTerm(right)));
        Node* nextIndex = newInteger(tag, index + 1);
        Node* getIndex = newApplication(tag, newBuiltin(tag, GET), nextIndex);
        Node* tail = newApplication(tag, getIndex, getTerm(right));
        Node* prependC = newApplication(tag, prependGlobal, newInteger(tag, c));
        push(INPUT_STACK, newApplication(tag, prependC, tail));
    }
    return peek(INPUT_STACK, 0);
}

Node* computeBuiltin(Closure* builtin, long long left, long long right) {
    Tag tag = getTag(getTerm(builtin));
    switch (getValue(getTerm(builtin))) {
        case PLUS: return newInteger(tag, add(left, right, builtin));
        case MINUS: return newInteger(tag, subtract(left, right, builtin));
        case TIMES: return newInteger(tag, multiply(left, right, builtin));
        case DIVIDE: return newInteger(tag, divide(left, right, builtin));
        case MODULUS: return newInteger(tag, modulo(left, right, builtin));
        case EQUAL: return newBoolean(tag, left == right);
        case NOTEQUAL: return newBoolean(tag, left != right);
        case LESSTHAN: return newBoolean(tag, left < right);
        case GREATERTHAN: return newBoolean(tag, left > right);
        case LESSTHANOREQUAL: return newBoolean(tag, left <= right);
        case GREATERTHANOREQUAL: return newBoolean(tag, left >= right);
        case PUT: return evaluatePut(builtin, left);
        default: assert(false); return NULL;
    }
}

Hold* evaluateIntegerBuiltin(Closure* builtin, Closure* left, Closure* right) {
    long long leftInteger = getIntegerArgument(builtin, left);
    long long rightInteger = getIntegerArgument(builtin, right);
    Node* term = computeBuiltin(builtin, leftInteger, rightInteger);
    return makeResult(builtin, term);
}

Hold* evaluateBuiltin(Closure* builtin, Closure* left, Closure* right) {
    switch (getValue(getTerm(builtin))) {
        case GET: return makeResult(builtin, evaluateGet(builtin, left, right));
        case ERROR: return evaluateError(builtin, left);
        case EXIT: return error("\n");
        default: return evaluateIntegerBuiltin(builtin, left, right);
    }
}
