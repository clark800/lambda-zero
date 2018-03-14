#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "lib/tree.h"
#include "lib/errors.h"
#include "ast.h"
#include "objects.h"
#include "builtins.h"

long long INPUT_INDEX = 0;

static inline Node* toBoolean(long long value) {
    return value == 0 ? FALSE : TRUE;
}

static inline int sgn(long long n) {
    return n == 0 ? 0 : (n > 0 ? 1 : -1);
}

// note: it is important to check for overflow before it occurs because
// undefined behavior occurs immediately after an overflow, which is
// impossible to recover from
static inline long long add(long long left, long long right) {
    if (left > 0 && right > 0 && left > LLONG_MAX - right)
        error("Integer overflow", "addition");
    if (left < 0 && right < 0 && left < LLONG_MIN - right)
        error("Integer overflow", "addition");
    return left + right;
}

static inline long long subtract(long long left, long long right) {
    if (left > 0 && right < 0 && left > LLONG_MAX + right)
        error("Integer overflow", "subtraction");
    if (left < 0 && right > 0 && left < LLONG_MIN + right)
        error("Integer overflow", "subtraction");
    return left - right;
}

static inline long long multiply(long long left, long long right) {
    if (right == -1 && left == LLONG_MIN)
        error("Integer overflow", "multiplication");
    if (right != 0 && right != 1 && right != -1) {
        // todo: fix this
        if (sgn(left) == sgn(right) && llabs(left) > llabs(LLONG_MAX / right))
            error("Integer overflow", "multiplication");
        if (sgn(left) != sgn(right) && llabs(left) > llabs(LLONG_MIN / right))
            error("Integer overflow", "multiplication");
    }
    return left * right;
}

static inline long long divide(long long left, long long right) {
    if (right == 0)
        error("Integer arithmetic", "divide by zero");
    if (left == LLONG_MIN && right == -1)
        error("Integer overflow", "division");
    return left / right;
}

static inline long long modulo(long long left, long long right) {
    if (right == 0)
        error("Integer arithmetic", "modulo by zero");
    if (left == LLONG_MIN && right == -1)
        return 0;
    return left % right;
}

int getArity(Node* builtin) {
    switch (getBuiltinCode(builtin)) {
        case GET: return 1;
        case PUT: return 1;
        default: return 2;
    }
}

Node* evaluatePut(long long c) {
    if (c < 0 || c >= 256)
        error("Runtime", "expected byte value in list returned from main");
    putchar((int)c);
    return IDENTITY;
}

Node* evaluateGet(Node* builtin, long long index) {
    assert(index == INPUT_INDEX);    // ensure lazy evaluation is working
    INPUT_INDEX += 1;
    int c = getchar();
    int location = getLocation(builtin);
    return c == EOF ? NIL : prepend(newInteger(location, c), newApplication(
                location, GET_BUILTIN, newInteger(location, index + 1)));
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
