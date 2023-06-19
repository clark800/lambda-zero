#include <stdlib.h>     // exit
#include <stdio.h>      // fputs
#include <limits.h>     // LLONG_MAX
#include "util.h"       // error
#include "tree.h"
#include "stack.h"
#include "array.h"
#include "parse/term.h"
#include "closure.h"
#include "exception.h"
#include "operations.h"

static bool STDERR = false;
Stack* INPUT_STACK;
extern Term *TRUE, *FALSE;

static Term* Boolean(bool value) {return value ? TRUE : FALSE;}

static void operationError(const char* message) {
    fputs("\nRuntime error: ", stderr);
    fputs(message, stderr);
    fputs("\n", stderr);
    exit(1);
}

static long long increment(long long n) {
    if (n >= LLONG_MAX)
        operationError("overflow in 'up'");
    return n + 1;
}

// note: it is important to check for overflow before it occurs because
// undefined behavior occurs immediately after an overflow, which is
// impossible to recover from
static long long add(long long left, long long right) {
    if (left > LLONG_MAX - right)
        operationError("overflow in '+'");
    return left + right;
}

static long long multiply(long long left, long long right) {
    if (right != 0 && left > LLONG_MAX / right)
        operationError("overflow in '*'");
    return left * right;
}

unsigned int getArity(Term* operation) {
    switch (getOperationCode(operation)) {
        case ABORT: return 1;
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

static Hold* evaluateAbort(Closure* operation, Closure* message) {
    if (TRACE) {
        printRuntimeError("hit", operation);
        fputc((int)'\n', stderr);
    }
    STDERR = true;
    return message == NULL ? NULL : hold(message);
}

static Term* evaluatePut(Closure* operation, Term* left) {
    long long c = getNumericValue(operation, left);
    if (c >= 0 && c < 256)
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
        case INCREMENT: return Numeral(tag, increment(left));
        case PLUS: return Numeral(tag, add(left, right));
        case MONUS: return Numeral(tag, right >= left ? 0 : left - right);
        case TIMES: return Numeral(tag, multiply(left, right));
        case DIVIDE: return Numeral(tag, right == 0 ? 0 : left / right);
        case MODULO: return Numeral(tag, right == 0 ? left : left % right);
        case EQUAL: return Boolean(left == right);
        case NOTEQUAL: return Boolean(left != right);
        case LESSTHAN: return Boolean(left < right);
        case GREATERTHAN: return Boolean(left > right);
        case LESSTHANOREQUAL: return Boolean(left <= right);
        case GREATERTHANOREQUAL: return Boolean(left >= right);
        default: assert(false); return NULL;
    }
}

static Hold* makeResult(Closure* operation, Term* node) {
    return hold(newClosure(node, NULL, getTrace(operation)));
}

static Hold* evaluateOperator(Closure* operation, Term* left, Term* right) {
    if ((left != NULL && !isNumeral(left)) ||
        (right != NULL && !isNumeral(right)))
        return NULL;
    long long leftValue = left == NULL ? 0 : getValue(left);
    long long rightValue = right == NULL ? 0 : getValue(right);
    Term* result = computeOperation(operation, leftValue, rightValue);
    return makeResult(operation, result);
}

static Hold* evaluateOperation(Closure* operation, Term* left, Term* right) {
    switch (getOperationCode(getTerm(operation))) {
        case EXIT: return error("\n");
        case PUT: return makeResult(operation, evaluatePut(operation, left));
        case GET: return makeResult(operation,
                            evaluateGet(operation, left, right));
        default: return evaluateOperator(operation, left, right);
    }
}

Hold* evaluateOperationTerm(Closure* operation, Closure* left, Closure* right) {
    if (getOperationCode(getTerm(operation)) == ABORT)
        return evaluateAbort(operation, left);
    switch (getArity(getTerm(operation))) {
        case 0: return evaluateOperation(operation, NULL, NULL);
        case 1: return left == NULL ? NULL :
            evaluateOperation(operation, getTerm(left), NULL);
        default: return left == NULL || right == NULL ? NULL :
            evaluateOperation(operation, getTerm(left), getTerm(right));
    }
}
