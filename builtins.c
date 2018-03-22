#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "lib/tree.h"
#include "lib/errors.h"
#include "ast.h"
#include "objects.h"
#include "closure.h"
#include "builtins.h"

bool STDERR = false;

static inline Node* toBoolean(int value) {
    return value == 0 ? FALSE : TRUE;
}

// note: it is important to check for overflow before it occurs because
// undefined behavior occurs immediately after an overflow, which is
// impossible to recover from
static inline long long add(long long left, long long right) {
    if (left > 0 && right > 0 && left > LLONG_MAX - right)
        error("Integer overflow", "addition");
    if (left < 0 && right < 0 && left < -LLONG_MAX - right)
        error("Integer overflow", "addition");
    return left + right;
}

static inline long long subtract(long long left, long long right) {
    if (left > 0 && right < 0 && left > LLONG_MAX + right)
        error("Integer overflow", "subtraction");
    if (left < 0 && right > 0 && left < -LLONG_MAX + right)
        error("Integer overflow", "subtraction");
    return left - right;
}

static inline long long multiply(long long left, long long right) {
    if (right != 0 && llabs(left) > llabs(LLONG_MAX / right))
        error("Integer overflow", "multiplication");
    return left * right;
}

static inline long long divide(long long left, long long right) {
    if (right == 0)
        error("Integer arithmetic", "divide by zero");
    return left / right;
}

static inline long long modulo(long long left, long long right) {
    if (right == 0)
        error("Integer arithmetic", "modulo by zero");
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

Hold* evaluateError(Node* builtin, Closure* closure) {
    STDERR = true;
    fputc((int)'\n', stderr);       // start error message on a new line
    Node* exit = newBuiltin(getLocation(builtin), EXIT);
    setTerm(closure, newApplication(getLocation(builtin), exit,
        newApplication(getLocation(builtin), getTerm(closure), PRINT)));
    return hold(closure);
}

Node* evaluatePut(long long c) {
    if (c < 0 || c >= 256)
        error("Runtime", "expected byte value in list returned from main");
    fputc((int)c, STDERR ? stderr : stdout);
    return IDENTITY;
}

Node* evaluateGet(Node* builtin, long long index) {
    static long long inputIndex = 0;
    assert(index == inputIndex);    // ensure lazy evaluation is working
    inputIndex += 1;
    int c = fgetc(stdin);
    int location = getLocation(builtin);
    return c == EOF ? NIL : prepend(newInteger(location, c),
        newApplication(location, GET_BUILTIN, newInteger(location, index + 1)));
}

Node* computeBuiltin(Node* builtin, long long left, long long right) {
    int location = getLocation(builtin);
    switch (getBuiltinCode(builtin)) {
        case PLUS: return newInteger(location, add(left, right));
        case MINUS: return newInteger(location, subtract(left, right));
        case TIMES: return newInteger(location, multiply(left, right));
        case DIVIDE: return newInteger(location, divide(left, right));
        case MODULUS: return newInteger(location, modulo(left, right));
        case EQUAL: return toBoolean(left == right);
        case NOTEQUAL: return toBoolean(left != right);
        case LESSTHAN: return toBoolean(left < right);
        case GREATERTHAN: return toBoolean(left > right);
        case LESSTHANOREQUAL: return toBoolean(left <= right);
        case GREATERTHANOREQUAL: return toBoolean(left >= right);
        case PUT: return evaluatePut(left);
        case GET: return evaluateGet(builtin, left);
        default: assert(false); return NULL;
    }
}

long long getIntegerArgument(Node* builtin, Closure* closure) {
    if (closure == NULL)
        return 0;
    Node* integer = getTerm(closure);
    errorIf(!isInteger(integer), builtin, "expected integer argument");
    return getInteger(integer);
}

Hold* evaluateIntegerBuiltin(Node* builtin, Closure* left, Closure* right) {
    long long leftInteger = getIntegerArgument(builtin, left);
    long long rightInteger = getIntegerArgument(builtin, right);
    Node* term = computeBuiltin(builtin, leftInteger, rightInteger);
    return hold(newClosure(term, VOID));
}

Hold* evaluateBuiltin(Node* builtin, Closure* left, Closure* right) {
    switch (getBuiltinCode(builtin)) {
        case ERROR: return evaluateError(builtin, left);
        case EXIT: throwTokenError("\nRuntime", "hit", builtin); return NULL;
        default: return evaluateIntegerBuiltin(builtin, left, right);
    }
}
