#include <stddef.h>
#include <limits.h>
#include "lib/tree.h"
#include "lib/errors.h"
#include "ast.h"
#include "parse.h"
#include "builtins.h"

const char* OBJECTS =
    "($x -> $x)\n"                      // identity
    "($x -> $y -> $y)\n"                // false
    "($z -> ($x -> $y -> $x))\n"        // nil
    "($f -> ($x -> $f ($x $x)) ($x -> $f ($x $x)))\n"  // Y combinator
    "($print -> ($f -> $f $print))\n"   // print
    "($ -> $)";                         // terminator

Hold* OBJECTS_HOLD = NULL;
Node* NIL = NULL;
Node* TRUE = NULL;
Node* FALSE = NULL;
Node* YCOMBINATOR = NULL;
Node* PARAMETERX = NULL;
Node* REFERENCEX = NULL;
Node* PRINT = NULL;
Node* PRINTRETURN = NULL;

enum {PLUS=BUILTIN, MINUS, TIMES, DIVIDE, MODULUS, EQUAL, NOTEQUAL,
      LESSTHAN, GREATERTHAN, LESSTHANOREQUAL, GREATERTHANOREQUAL};

const char* BUILTINS[] = {"+", "-", "*", "/", "mod",
    "==", "=/=", "<", ">", "<=", ">="};

Node* getElement(Node* node, unsigned int n) {
    for (unsigned int i = 0; i < n; i++)
        node = getRight(node);
    return getLeft(node);
}

void initBuiltins() {
    OBJECTS_HOLD = parse(OBJECTS);
    Node* objects = getNode(OBJECTS_HOLD); 
    negateLocations(objects);
    Node* id = getElement(objects, 0);
    PARAMETERX = getParameter(id);
    REFERENCEX = getBody(id);
    FALSE = getElement(objects, 1);
    NIL = getElement(objects, 2);
    TRUE = getBody(NIL);
    YCOMBINATOR = getElement(objects, 3);
    PRINTRETURN = getBody(getElement(objects, 4));
    PRINT = getRight(getBody(PRINTRETURN));
}

void deleteBuiltins() {
    release(OBJECTS_HOLD);
}

unsigned long long lookupBuiltinCode(Node* token) {
    for (unsigned long long i = 0; i < sizeof(BUILTINS)/sizeof(char*); i++)
        if (isThisToken(token, BUILTINS[i]))
            return i + BUILTIN;
    return 0;
}

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

Node* computeBuiltin(Node* builtin, long long left, long long right) {
    int location = getLocation(builtin);
    unsigned long long code = getBuiltinCode(builtin);
    switch (code) {
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
        default: assert(false); return NULL;
    }
}
