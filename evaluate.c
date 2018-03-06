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

Hold* evaluateNode(State* state);

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

static inline Node* getReferencedClosure(Node* reference, Stack* env) {
    return peek(env, getDebruijnIndex(reference) - 1);
}

Node* getArgumentClosure(Node* argument, Stack* env) {
    if (isReference(argument))     // short-circuit optimization
        return getReferencedClosure(argument, env);
    return newClosure(argument, getHead(env));
}

void evaluateApplicationNode(State* state) {
    Node* function = getLeft(getNode(state->node));
    Node* argument = getRight(getNode(state->node));
    push(state->stack, getArgumentClosure(argument, state->env));
    setNode(state, function);
}

void evaluateAbstractionNode(State* state) {
    if (isUpdateNext(state->stack)) {
        applyUpdate(state);
        return;
    }
    moveStackItem(state->stack, state->env);
    Node* function = getNode(state->node);
    setNode(state, getBody(function));
}

void evaluateReferenceNode(State* state) {
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

void evaluateBuiltinNode(State* state) {
    Node* builtin = getNode(state->node);
    Stack* env = newStack(NULL);
    for (int i = 0; i < getArity(builtin); i++) {
        applyUpdates(state);
        moveStackItem(state->stack, env);
    }
    setNode(state, evaluateBuiltin(builtin, env));
    deleteStack(env);
}

Hold* evaluateNode(State* state) {
    bool doIO = false;
    while (true) {
        debugEvalState(getNode(state->node), state->stack, state->env);
        LOOP_COUNT += 1;
        Node* node = getNode(state->node);
        if (isApplication(node)) {
            evaluateApplicationNode(state);
        } else if (isAbstraction(node)) {
            if (isEmpty(state->stack)) {
                if (isThisToken(getParameter(node), "input")) {
                    evaluationErrorIf(IO, node, "input can only be used once");
                    push(state->stack, newClosure(INPUT, NULL));
                    IO = doIO = true;
                    continue;
                }
                return getResult(state, doIO);
            }
            evaluateAbstractionNode(state);
        } else if (isInteger(node)) {
            return getResult(state, doIO);
        } else if (isReference(node)) {
            evaluateReferenceNode(state);
        } else if (isBuiltin(node)) {
            evaluateBuiltinNode(state);
        } else {
            assert(false);
        }
    }
}

Hold* evaluate(Node* closure) {
    State state;
    initState(&state, getClosureTerm(closure), getClosureEnv(closure));
    Hold* result = evaluateNode(&state);
    deleteState(&state);
    debugLoopCount(LOOP_COUNT);
    return result;
}
