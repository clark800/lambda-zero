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
        printTokenAndLocationLine(cursor(it), "");
}

void printRuntimeError(const char* message, Closure* closure) {
    if (!TEST && !isEmpty((Stack*)getBacktrace(closure)))
        printBacktrace(closure);
    printTokenError("\nRuntime", message, getTerm(closure));
}

void runtimeError(const char* message, Closure* closure) {
    printRuntimeError(message, closure);
    exit(1);
}

static inline Node* toBoolean(int value) {
    return value == 0 ? FALSE : TRUE;
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
    switch (getBuiltinCode(builtin)) {
        case ERROR: return 1;
        case EXIT: return 1;
        case GET: return 1;
        case PUT: return 1;
        default: return 2;
    }
}

bool isStrictArgument(Node* builtin, unsigned int i) {
    return !(getBuiltinCode(builtin) == ERROR && i == 0);
}

Hold* evaluateError(Closure* builtin, Closure* message) {
    STDERR = true;
    if (!TEST) {
        printRuntimeError("hit", builtin);
        fputc((int)'\n', stderr);
    }
    String lexeme = getLexeme(getTerm(builtin));
    Node* exit = newBuiltin(lexeme, EXIT);
    Node* print = newApplication(lexeme, getTerm(message), PRINT);
    setTerm(builtin, newApplication(lexeme, exit, print));
    return hold(builtin);
}

Node* evaluatePut(Closure* builtin, long long c) {
    if (c < 0 || c >= 256)
        runtimeError("expected byte value in list returned from main", builtin);
    fputc((int)c, STDERR ? stderr : stdout);
    return IDENTITY;
}

Node* evaluateGet(Closure* builtin, long long index) {
    static long long inputIndex = 0;
    assert(index <= inputIndex);
    if (index < inputIndex)
        return peek(INPUT_STACK, (size_t)(inputIndex - index - 1));
    inputIndex += 1;
    int c = fgetc(stdin);
    String lexeme = getLexeme(getTerm(builtin));
    push(INPUT_STACK, c == EOF ? newNil(lexeme) : prepend(lexeme,
        newInteger(lexeme, c), newApplication(lexeme, getLeft(INPUT),
            newInteger(lexeme, index + 1))));
    return peek(INPUT_STACK, 0);
}

Node* computeBuiltin(Closure* builtin, long long left, long long right) {
    String lexeme = getLexeme(getTerm(builtin));
    switch (getBuiltinCode(getTerm(builtin))) {
        case PLUS: return newInteger(lexeme, add(left, right, builtin));
        case MINUS: return newInteger(lexeme, subtract(left, right, builtin));
        case TIMES: return newInteger(lexeme, multiply(left, right, builtin));
        case DIVIDE: return newInteger(lexeme, divide(left, right, builtin));
        case MODULUS: return newInteger(lexeme, modulo(left, right, builtin));
        case EQUAL: return toBoolean(left == right);
        case NOTEQUAL: return toBoolean(left != right);
        case LESSTHAN: return toBoolean(left < right);
        case GREATERTHAN: return toBoolean(left > right);
        case LESSTHANOREQUAL: return toBoolean(left <= right);
        case GREATERTHANOREQUAL: return toBoolean(left >= right);
        case PUT: return evaluatePut(builtin, left);
        case GET: return evaluateGet(builtin, left);
        default: assert(false); return NULL;
    }
}

long long getIntegerArgument(Node* builtin, Closure* closure) {
    if (closure == NULL)
        return 0;
    Node* integer = getTerm(closure);
    if (!isInteger(integer))
       runtimeError("expected integer argument to", builtin);
    return getInteger(integer);
}

Hold* evaluateIntegerBuiltin(Closure* builtin, Closure* left, Closure* right) {
    long long leftInteger = getIntegerArgument(builtin, left);
    long long rightInteger = getIntegerArgument(builtin, right);
    Node* term = computeBuiltin(builtin, leftInteger, rightInteger);
    return hold(newClosure(term, VOID, getTrace(builtin)));
}

Hold* evaluateBuiltin(Closure* builtin, Closure* left, Closure* right) {
    switch (getBuiltinCode(getTerm(builtin))) {
        case ERROR: return evaluateError(builtin, left);
        case EXIT: return error("\n");
        default: return evaluateIntegerBuiltin(builtin, left, right);
    }
}
