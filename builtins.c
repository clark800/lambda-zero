#include <stddef.h>
#include <limits.h>
#include <stdio.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "lib/errors.h"
#include "ast.h"
#include "closure.h"
#include "parse.h"
#include "evaluate.h"
#include "builtins.h"

// put(c) = id              -- with side effect of printing the character c
// print(c) = (put c) (cs -> cs print)   -- pass this into a string to print it
// print = c -> (put c) (cs -> cs print)                 -- desugar
// print = Y (print -> c -> (put c) (cs -> cs print))    -- desugar

const char* OBJECTS =
    "($x -> $x)\n"                                          // identity
    "($t -> $f -> $f)\n"                                    // false
    "($z -> ($t -> $f -> $t))\n"                            // nil
    "($y -> ($q -> $y ($q $q)) ($q -> $y ($q $q))) "        // Y combinator
    "($print -> $c -> (($put $c) ($cs -> $cs $print)))\n"   // string printer
    "($get 0)\n"                                            // lazy input list
    "($ -> $)";                                             // terminator

Hold* OBJECTS_HOLD = NULL;
Node* IDENTITY = NULL;
Node* NIL = NULL;
Node* TRUE = NULL;
Node* FALSE = NULL;
Node* YCOMBINATOR = NULL;
Node* PARAMETERX = NULL;
Node* REFERENCEX = NULL;
Node* PRINT = NULL;
Node* INPUT = NULL;
Node* GET_BUILTIN = NULL;
long long INPUT_INDEX = 0;

enum {PLUS=BUILTIN, MINUS, TIMES, DIVIDE, MODULUS, EQUAL, NOTEQUAL,
      LESSTHAN, GREATERTHAN, LESSTHANOREQUAL, GREATERTHANOREQUAL, PUT, GET};

const char* BUILTINS[] = {"+", "-", "*", "/", "mod",
    "==", "=/=", "<", ">", "<=", ">=", "$put", "$get"};

Node* newNil(int location) {
    return newLambda(location, PARAMETERX, TRUE);
}

Node* prepend(Node* item, Node* list) {
    int location = getLocation(list);
    return newLambda(location, PARAMETERX, newApplication(location,
            newApplication(location, REFERENCEX, item), list));
}

Node* getElement(Node* node, unsigned int n) {
    for (unsigned int i = 0; i < n; i++)
        node = getRight(node);
    return getLeft(node);
}

void initBuiltins() {
    OBJECTS_HOLD = parse(OBJECTS);
    Node* objects = getNode(OBJECTS_HOLD); 
    negateLocations(objects);
    IDENTITY = getElement(objects, 0);
    PARAMETERX = getParameter(IDENTITY);
    REFERENCEX = getBody(IDENTITY);
    FALSE = getElement(objects, 1);
    NIL = getElement(objects, 2);
    TRUE = getBody(NIL);
    PRINT = getElement(objects, 3);
    YCOMBINATOR = getLeft(PRINT);
    INPUT = getElement(objects, 4);
    GET_BUILTIN = getLeft(INPUT);
}

void deleteBuiltins() {
    release(OBJECTS_HOLD);
}

void evaluationErrorIf(bool condition, Node* token, const char* message) {
    if (condition)
        throwTokenError("Evaluation", message, token);
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

int getArity(Node* builtin) {
    switch (getBuiltinCode(builtin)) {
        case GET: return 1;
        case PUT: return 1;
        default: return 2;
    }
}

Node* evaluatePut(Node* builtin, long long c) {
    evaluationErrorIf(c < 0 || c >= 256, builtin, "expected byte value");
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
        case PUT: return evaluatePut(builtin, left);
        case GET: return evaluateGet(builtin, left);
        default: assert(false); return NULL;
    }
}

long long evaluateToInteger(Node* builtin, Hold* termClosure) {
    Hold* valueClosure = evaluate(getNode(termClosure));
    Node* integerNode = getClosureTerm(getNode(valueClosure));
    evaluationErrorIf(!isInteger(integerNode), builtin,
            "expected integer first parameter");
    long long integer = getInteger(integerNode);
    release(valueClosure);
    release(termClosure);
    return integer;
}

Node* evaluateBuiltin(Node* builtin, Stack* stack) {
    int arity = getArity(builtin);
    if (arity == 0)
        return computeBuiltin(builtin, 0, 0);
    evaluationErrorIf(isEmpty(stack), builtin, "missing first argument");
    long long left = evaluateToInteger(builtin, pop(stack));
    if (arity == 1)
        return computeBuiltin(builtin, left, 0);
    evaluationErrorIf(isEmpty(stack), builtin, "missing second argument");
    long long right = evaluateToInteger(builtin, pop(stack));
    return computeBuiltin(builtin, left, right);
}
