#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "lib/readfile.h"
#include "ast.h"
#include "closure.h"
#include "serialize.h"
#include "builtins.h"
#include "lex.h"
#include "evaluate.h"

static const bool LAZY = true;     // for debugging
int LOOP_COUNT = 0;
bool IO = false;

typedef struct State State;

// note: cannot use an array for env because it is a cactus stack
struct State {
    Hold* node;
    Stack* stack;
    Stack* env;
};

Hold* evaluateDebruijn(Node* env, unsigned long long debruijn);
Hold* evaluateNode(State* state);

void evaluationErrorIf(bool condition, Node* token, const char* message) {
    if (condition)
        throwTokenError("Evaluation", message, token);
}

static bool isUpdateNext(Stack* stack) {
    return !isEmpty(stack) && isUpdateClosure(peek(stack, 0));
}

bool isUpdatesOnly(Stack* stack) {
    for (Iterator* it = iterate(stack); !end(it); it = next(it))
        if (!isUpdateClosure(cursor(it)))
            return false;
    return true;
}

void initState(State* state, Node* root, Node* env) {
    state->node = hold(root);
    state->stack = newStack(NULL);
    state->env = newStack(env);
}

void deleteState(State* state) {
    release(state->node);
    deleteStack(state->stack);
    deleteStack(state->env);
}

static inline void setNode(State* state, Node* node) {
    state->node = replaceHold(state->node, hold(node));
}

void moveStackItem(Stack* fromStack, Stack* toStack) {
    Hold* item = pop(fromStack);
    push(toStack, getNode(item));
    release(item);
}

void applyUpdate(State* state) {
    Hold* update = pop(state->stack);
    Node* closure = getRight(getNode(update));
    setLeft(closure, getNode(state->node));
    setRight(closure, getHead(state->env));
    release(update);
}

void applyUpdates(State* state) {
    while (isUpdateNext(state->stack))
        applyUpdate(state);
}

long long evaluateToInteger(Node* env, unsigned long long debruijn,
        Node* node) {
    Hold* value = evaluateDebruijn(env, debruijn);
    Node* integer = getClosureTerm(getNode(value));
    evaluationErrorIf(!isInteger(integer), node, "expected integer argument");
    long long result = getInteger(integer);
    release(value);
    return result;
}

static inline Node* getReferencedClosure(Node* reference, Stack* env) {
    return peek(env, getDebruijnIndex(reference) - 1);
}

Node* getArgumentClosure(Node* argument, Stack* env) {
    if (isReference(argument))     // short-circuit optimization
        return getReferencedClosure(argument, env);
    return newClosure(argument, getHead(env));
}

void evaluateApplication(State* state) {
    Node* function = getLeft(getNode(state->node));
    Node* argument = getRight(getNode(state->node));
    push(state->stack, getArgumentClosure(argument, state->env));
    setNode(state, function);
}

void evaluateAbstraction(State* state) {
    if (isUpdateNext(state->stack)) {
        applyUpdate(state);
        return;
    }
    moveStackItem(state->stack, state->env);
    Node* function = getNode(state->node);
    if (isPut(getParameter(function))) {
        long long c = evaluateToInteger(getHead(state->env), 1, function);
        evaluationErrorIf(c < 0 || c >= 256, function, "expected byte value");
        putchar((int)c);
    }
    setNode(state, getBody(function));
}

void evaluateReference(State* state) {
    Node* closure = getReferencedClosure(getNode(state->node), state->env);
    if (LAZY && !isAbstraction(getClosureTerm(closure)))
        push(state->stack, newUpdateClosure(closure));
    setNode(state, getClosureTerm(closure));
    setHead(state->env, getClosureEnv(closure));
}

static inline Hold* getResult(State* state, bool doIO) {
    if (doIO) {
        push(state->stack, newClosure(PRINT, NULL));
        release(evaluateNode(state));
        return NULL;
    }
    evaluationErrorIf(!isUpdatesOnly(state->stack), NULL, "extra arguments");
    return hold(newClosure(getNode(state->node), getHead(state->env)));
}

void evaluateBuiltin(State* state) {
    // simulate evaluating two abstractions
    applyUpdates(state);
    moveStackItem(state->stack, state->env);
    applyUpdates(state);
    moveStackItem(state->stack, state->env);
    Node* operator = getNode(state->node);
    long long left = evaluateToInteger(getHead(state->env), 2, operator);
    long long right = evaluateToInteger(getHead(state->env), 1, operator);
    setNode(state, computeBuiltin(getNode(state->node), left, right));
}

Node* getInputClosure(Node* token) {
    char* input = readfile(stdin);
    Node* string = newRawString(getLocation(token), input);
    free(input);
    return newClosure(string, NULL);
}

Hold* evaluateNode(State* state) {
    bool doIO = false;
    while (true) {
        debugEvalState(getNode(state->node), state->stack, state->env);
        LOOP_COUNT += 1;
        Node* node = getNode(state->node);
        if (isApplication(node)) {
            evaluateApplication(state);
        } else if (isAbstraction(node)) {
            if (isEmpty(state->stack)) {
                if (isThisToken(getParameter(node), "input")) {
                    evaluationErrorIf(IO, node, "input can only be used once");
                    push(state->stack, getInputClosure(getParameter(node)));
                    IO = doIO = true;
                    continue;
                }
                return getResult(state, doIO);
            }
            evaluateAbstraction(state);
        } else if (isInteger(node)) {
            return getResult(state, doIO);
        } else if (isReference(node)) {
            evaluateReference(state);
        } else if (isBuiltin(node)) {
            evaluateBuiltin(state);
        } else {
            assert(false);
        }
    }
}

Hold* evaluateWithEnv(Node* root, Node* env) {
    State state;
    initState(&state, root, env);
    Hold* result = evaluateNode(&state);
    deleteState(&state);
    return result;
}

Hold* evaluateDebruijn(Node* env, unsigned long long debruijn) {
    Hold* reference = hold(newReference(-1, debruijn));
    Hold* result = evaluateWithEnv(getNode(reference), env);
    release(reference);
    return result;
}

Hold* evaluate(Node* root) {
    Hold* result = evaluateWithEnv(root, NULL);
    debugLoopCount(LOOP_COUNT);
    return result;
}
