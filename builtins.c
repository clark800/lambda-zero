#include <stddef.h>
#include <limits.h>
#include "lib/tree.h"
#include "lib/errors.h"
#include "ast.h"
#include "parse.h"
#include "builtins.h"

// first char must be space, second char must be open parentheses
const char* OBJECTS = " "               // offset locations from zero
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
        error("Integer overflow", "addition/subtraction");
    if (left < 0 && right < 0 && left < LLONG_MIN - right)
        error("Integer overflow", "addition/subtraction");
    return left + right;
}

static inline long long multiply(long long left, long long right) {
    if (right != 0) {
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
    divide(left, right); // check for errors
    long long remainder = left % right;
    return remainder >= 0 ? remainder : add(remainder, right);
}

Node* calculate(unsigned long long code, long long left, long long right) {
    switch (code) {
        case PLUS: return newComputedInteger(add(left, right));
        case MINUS: return newComputedInteger(add(left, -right));
        case TIMES: return newComputedInteger(multiply(left, right));
        case DIVIDE: return newComputedInteger(divide(left, right));
        case MODULUS: return newComputedInteger(modulo(left, right));
        case EQUAL: return toBoolean(left == right);
        case NOTEQUAL: return toBoolean(left != right);
        case LESSTHAN: return toBoolean(left < right);
        case GREATERTHAN: return toBoolean(left > right);
        case LESSTHANOREQUAL: return toBoolean(left <= right);
        case GREATERTHANOREQUAL: return toBoolean(left >= right);
        default: assert(false); return NULL;
    }
}

Hold* computeBuiltin(unsigned long long code, long long left, long long right) {
    return hold(calculate(code, left, right));
}
