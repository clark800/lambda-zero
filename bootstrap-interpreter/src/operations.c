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

static Node* newBoolean(Tag tag, bool value) {
    Node* underscore = Underscore(tag, 0);
    return Abstraction(tag, underscore, Abstraction(tag, underscore,
        value ? Underscore(tag, 1) : Underscore(tag, 2)));
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

unsigned int getArity(Node* operation) {
    switch (getOperationCode(operation)) {
        case UNDEFINED: return 0;
        case ERROR: return 1;
        case EXIT: return 1;
        case INCREMENT: return 1;
        case PUT: return 1;
        default: return 2;
    }
}

static long long getNumericValue(Node* operation, Node* numeral) {
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

static Node* evaluatePut(Closure* operation, Node* left) {
    long long c = getNumericValue(operation, left);
    if (c < 0 || c >= 256)
        runtimeError("non-byte value in string returned from main", operation);
    fputc((int)c, STDERR ? stderr : stdout);
    Tag tag = getTag(getTerm(operation));
    return Abstraction(tag, Underscore(tag, 0), Underscore(tag, 1));
}

static Node* evaluateGet(Closure* operation, Node* left, Node* right) {
    static long long inputIndex = 0;
    long long index = getNumericValue(operation, left);
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
        Tag tag = getTag(getTerm(operation));
        Node* prependGlobal = getRight(getBody(right));
        Node* nextIndex = Numeral(tag, index + 1);
        Node* getIndex = Application(tag, getTerm(operation), nextIndex);
        Node* tail = Application(tag, getIndex, right);
        Node* prependC = Application(tag, prependGlobal, Numeral(tag, c));
        push(INPUT_STACK, Application(tag, prependC, tail));
    }
    return peek(INPUT_STACK, 0);
}

static Node* computeOperation(Closure* operation,
        long long left, long long right) {
    Tag tag = getTag(getTerm(operation));
    switch (getOperationCode(getTerm(operation))) {
        case INCREMENT: return Numeral(tag, left + 1);
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
    }
    assert(false);
    return NULL;
}

static Hold* makeResult(Closure* operation, Node* node) {
    return hold(newClosure(node, VOID, getTrace(operation)));
}

static Hold* evaluateOperator(Closure* operation, Node* left, Node* right) {
    long long leftValue = getNumericValue(operation, left);
    long long rightValue = getNumericValue(operation, right);
    Node* result = computeOperation(operation, leftValue, rightValue);
    return makeResult(operation, result);
}

static Hold* evaluateOperation(Closure* operation, Node* left, Node* right) {
    switch (getOperationCode(getTerm(operation))) {
        case EXIT: return error("\n");
        case UNDEFINED: return evaluateError(operation, NULL);
        case PUT: return makeResult(operation, evaluatePut(operation, left));
        case GET: return makeResult(operation,
                            evaluateGet(operation, left, right));
        default: return evaluateOperator(operation, left, right);
    }
}

Hold* evaluateOperationNode(Closure* operation, Closure* left, Closure* right) {
    if (getOperationCode(getTerm(operation)) == ERROR)
        return evaluateError(operation, left);
    switch (getArity(getTerm(operation))) {
        case 0: return evaluateOperation(operation, NULL, NULL);
        case 1: return evaluateOperation(operation, getTerm(left), NULL);
        default:
            return evaluateOperation(operation, getTerm(left), getTerm(right));
    }
}
