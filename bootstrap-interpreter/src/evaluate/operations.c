#include <stdlib.h>     // exit
#include <stdio.h>      // fputs
#include <limits.h>     // LLONG_MAX
#include "shared/lib/util.h"   // error
#include "shared/lib/tree.h"
#include "shared/lib/stack.h"
#include "shared/term.h"
#include "closure.h"
#include "exception.h"

static bool STDERR = false;
Stack* INPUT_STACK;

static Term* newBoolean(Tag tag, bool value) {
    return Abstraction(tag, Abstraction(tag,
        value ? Variable(tag, 1) : Variable(tag, 2)));
}

static long long increment(long long n, Closure* operation) {
    if (n >= LLONG_MAX)
        runtimeError("overflow in", operation);
    return n + 1;
}

// note: it is important to check for overflow before it occurs because
// undefined behavior occurs immediately after an overflow, which is
// impossible to recover from
static long long add(long long left, long long right, Closure* operation) {
    if (left > LLONG_MAX - right)
        runtimeError("overflow in", operation);
    return left + right;
}

static long long subtract(long long left, long long right, Closure* operation) {
    if (right > left)
        runtimeError("underflow in", operation);
    return left - right;
}

static long long multiply(long long left, long long right, Closure* operation) {
    if (right != 0 && left > LLONG_MAX / right)
        runtimeError("overflow in", operation);
    return left * right;
}

static long long divide(long long left, long long right, Closure* operation) {
    if (right == 0)
        runtimeError("divide by zero in", operation);
    return left / right;
}

unsigned int getArity(Term* operation) {
    switch (getOperationCode(operation)) {
        case UNDEFINED: return 0;
        case ERROR: return 1;
        case EXIT: return 1;
        case INCREMENT: return 1;
        case PUT: return 1;
        default: return 2;
    }
}

static long long getNumericValue(Term* operation, Term* numeral) {
    // NULL is allowed for unary operators like increment
    if (numeral != NULL && !isNumeral(numeral))
        runtimeError("expected numeric argument to", operation);
    return numeral == NULL ? 0 : getValue(numeral);
}

static Hold* evaluateError(Closure* operation, Closure* message) {
    if (!TEST) {
        printRuntimeError("hit", operation);
        fputc((int)'\n', stderr);
    }
    if (message == NULL) {
        fputs("undefined", stderr);
        exit(1);
    }
    STDERR = true;
    return hold(message);
}

static Term* evaluatePut(Closure* operation, Term* left) {
    long long c = getNumericValue(operation, left);
    if (c < 0 || c >= 256)
        runtimeError("non-byte value in string returned from main", operation);
    fputc((int)c, STDERR ? stderr : stdout);
    Tag tag = getTag(getTerm(operation));
    return Abstraction(tag, Variable(tag, 1));
}

static Term* evaluateGet(Closure* operation, Term* left, Term* right) {
    static long long inputIndex = 0;
    long long index = getNumericValue(operation, left);
    assert(index <= inputIndex);
    if (index < inputIndex)
        return peek(INPUT_STACK, (size_t)(inputIndex - index - 1));
    inputIndex += 1;
    int c = fgetc(stdin);
    if (c == EOF) {
        Term* nilGlobal = getRight(getLeft(getBody(right)));
        push(INPUT_STACK, nilGlobal);
    } else {
        // push ((::) c get(n + 1, globals))
        Tag tag = getTag(getTerm(operation));
        Term* prependGlobal = getRight(getBody(right));
        Term* nextIndex = Numeral(tag, index + 1);
        Term* getIndex = Application(tag, getTerm(operation), nextIndex);
        Term* tail = Application(tag, getIndex, right);
        Term* prependC = Application(tag, prependGlobal, Numeral(tag, c));
        push(INPUT_STACK, Application(tag, prependC, tail));
    }
    return peek(INPUT_STACK, 0);
}

static Term* computeOperation(Closure* operation,
        long long left, long long right) {
    Tag tag = getTag(getTerm(operation));
    switch (getOperationCode(getTerm(operation))) {
        case INCREMENT: return Numeral(tag, increment(left, operation));
        case PLUS: return Numeral(tag, add(left, right, operation));
        case MINUS: return Numeral(tag, subtract(left, right, operation));
        case TIMES: return Numeral(tag, multiply(left, right, operation));
        case DIVIDE: return Numeral(tag, divide(left, right, operation));
        case MODULO: return Numeral(tag, right == 0 ? left : left % right);
        case EQUAL: return newBoolean(tag, left == right);
        case NOTEQUAL: return newBoolean(tag, left != right);
        case LESSTHAN: return newBoolean(tag, left < right);
        case GREATERTHAN: return newBoolean(tag, left > right);
        case LESSTHANOREQUAL: return newBoolean(tag, left <= right);
        case GREATERTHANOREQUAL: return newBoolean(tag, left >= right);
        default: assert(false);
            return NULL;
    }
}

static Hold* makeResult(Closure* operation, Term* node) {
    return hold(newClosure(node, VOID, getTrace(operation)));
}

static Hold* evaluateOperator(Closure* operation, Term* left, Term* right) {
    long long leftValue = getNumericValue(operation, left);
    long long rightValue = getNumericValue(operation, right);
    Term* result = computeOperation(operation, leftValue, rightValue);
    return makeResult(operation, result);
}

static Hold* evaluateOperation(Closure* operation, Term* left, Term* right) {
    switch (getOperationCode(getTerm(operation))) {
        case EXIT: return error("\n");
        case UNDEFINED: return evaluateError(operation, NULL);
        case PUT: return makeResult(operation, evaluatePut(operation, left));
        case GET: return makeResult(operation,
                            evaluateGet(operation, left, right));
        default: return evaluateOperator(operation, left, right);
    }
}

Hold* evaluateOperationTerm(Closure* operation, Closure* left, Closure* right) {
    if (getOperationCode(getTerm(operation)) == ERROR)
        return evaluateError(operation, left);
    switch (getArity(getTerm(operation))) {
        case 0: return evaluateOperation(operation, NULL, NULL);
        case 1: return evaluateOperation(operation, getTerm(left), NULL);
        default:
            return evaluateOperation(operation, getTerm(left), getTerm(right));
    }
}
