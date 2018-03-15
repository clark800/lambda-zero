#include <assert.h>
#include <stdbool.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "builtins.h"
#include "lex.h"
#include "serialize.h"
#include "evaluate.h"

static const bool LAZY = true;     // for debugging
bool PROFILE = false;
bool TRACE = false;
int LOOP_COUNT = 0;

typedef struct State State;

// note: cannot use an array for env because it is a cactus stack
struct State {
    Hold* node;
    Stack* stack;
    Stack* env;
};

static inline void errorIf(bool condition, Node* token, const char* message) {
    if (condition)
        throwTokenError("Evaluation", message, token);
}

static inline Node* newClosure(Node* term, Node* env) {
    return newBranchNode(0, term, env);
}

Node* getClosureTerm(Node* closure) {
    return getLeft(closure);
}

Node* getClosureEnv(Node* closure) {
    return getRight(closure);
}

static inline Node* newUpdateClosure(Node* closure) {
    return newClosure(NULL, closure);
}

static inline bool isUpdateClosure(Node* closure) {
    return getLeft(closure) == NULL && getRight(closure) != NULL;
}

static inline bool isUpdateNext(Stack* stack) {
    return !isEmpty(stack) && isUpdateClosure(peek(stack, 0));
}

static inline void initState(State* state, Node* root, Node* env) {
    state->node = hold(root);
    state->stack = newStack(NULL);
    state->env = newStack(env);
}

static inline void deleteState(State* state) {
    release(state->node);
    deleteStack(state->stack);
    deleteStack(state->env);
}

static inline void setNode(State* state, Node* node) {
    state->node = replaceHold(state->node, hold(node));
}

static inline void moveStackItem(Stack* fromStack, Stack* toStack) {
    Hold* item = pop(fromStack);
    push(toStack, getNode(item));
    release(item);
}

static inline void applyUpdate(State* state) {
    Hold* update = pop(state->stack);
    Node* closure = getRight(getNode(update));
    setLeft(closure, getNode(state->node));
    setRight(closure, getHead(state->env));
    release(update);
}

static inline void applyUpdates(State* state) {
    while (isUpdateNext(state->stack))
        applyUpdate(state);
}

static inline Node* getReferencedClosure(Node* reference, Stack* env) {
    return peek(env, getDebruijnIndex(reference) - 1);
}

static inline Node* getArgumentClosure(Node* argument, Stack* env) {
    if (isReference(argument))     // short-circuit optimization
        return getReferencedClosure(argument, env);
    return newClosure(argument, getHead(env));
}

static inline void evaluateApplicationNode(State* state) {
    Node* function = getLeft(getNode(state->node));
    Node* argument = getRight(getNode(state->node));
    push(state->stack, getArgumentClosure(argument, state->env));
    setNode(state, function);
}

static inline void evaluateLambdaNode(State* state) {
    moveStackItem(state->stack, state->env);
    Node* lambda = getNode(state->node);
    setNode(state, getBody(lambda));
}

static inline void evaluateReferenceNode(State* state) {
    Node* closure = getReferencedClosure(getNode(state->node), state->env);
    if (LAZY && !isLambda(getClosureTerm(closure)))
        push(state->stack, newUpdateClosure(closure));
    setNode(state, getClosureTerm(closure));
    setHead(state->env, getClosureEnv(closure));
}

static inline Hold* evaluateClosure(Node* closure) {
    return evaluate(getClosureTerm(closure), getClosureEnv(closure));
}

static inline long long evaluateToInteger(Node* builtin, Hold* termClosure) {
    Hold* valueClosure = evaluateClosure(getNode(termClosure));
    Node* integerNode = getClosureTerm(getNode(valueClosure));
    errorIf(!isInteger(integerNode), builtin, "expected integer argument");
    long long integer = getInteger(integerNode);
    release(valueClosure);
    release(termClosure);
    return integer;
}

static inline Node* evaluateBuiltin(Node* builtin, Stack* stack) {
    int arity = getArity(builtin);
    if (arity == 0)
        return computeBuiltin(builtin, 0, 0);
    errorIf(isEmpty(stack), builtin, "missing first argument");
    long long left = evaluateToInteger(builtin, pop(stack));
    if (arity == 1)
        return computeBuiltin(builtin, left, 0);
    errorIf(isEmpty(stack), builtin, "missing second argument");
    long long right = evaluateToInteger(builtin, pop(stack));
    return computeBuiltin(builtin, left, right);
}

static inline void evaluateBuiltinNode(State* state) {
    applyUpdates(state);
    setNode(state, evaluateBuiltin(getNode(state->node), state->stack));
}

static inline Hold* getResult(State* state) {
    return hold(newClosure(getNode(state->node), getHead(state->env)));
}

static inline Hold* getIntegerResult(State* state) {
    applyUpdates(state);
    if (!isEmpty(state->stack))
        errorIf(true, getClosureTerm(peek(state->stack, 0)), "extra argument");
    return getResult(state);
}

static inline void debugState(Node* node, Stack* stack, Stack* env) {
    if (TRACE) {
        debugLine();
        debug("node: ");
        debugAST(node);
        debug("\nstack: ");
        debugStack(stack, getClosureTerm);
        debug("\nenv: ");
        debugStack(env, getClosureTerm);
        debug("\n");
    }
}

static inline Hold* evaluateNode(State* state) {
    while (true) {
        debugState(getNode(state->node), state->stack, state->env);
        LOOP_COUNT += 1;
        Node* node = getNode(state->node);
        if (isApplication(node)) {
            evaluateApplicationNode(state);
        } else if (isLambda(node)) {
            applyUpdates(state);
            if (isEmpty(state->stack))
                return getResult(state);
            evaluateLambdaNode(state);
        } else if (isInteger(node)) {
            return getIntegerResult(state);
        } else if (isReference(node)) {
            evaluateReferenceNode(state);
        } else if (isBuiltin(node)) {
            evaluateBuiltinNode(state);
        } else {
            assert(false);
        }
    }
}

static inline void debugLoopCount(int loopCount) {
    if (PROFILE) {
        debug("Loops: ");
        debugInteger(loopCount);
        debug("\n");
    }
}

Hold* evaluate(Node* term, Node* env) {
    State state;
    initState(&state, term, env);
    Hold* result = evaluateNode(&state);
    deleteState(&state);
    debugLoopCount(LOOP_COUNT);
    return result;
}
