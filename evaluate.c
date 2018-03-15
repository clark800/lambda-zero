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

void evaluationErrorIf(bool condition, Node* token, const char* message) {
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

Hold* evaluateClosure(Node* closure) {
    return evaluate(getClosureTerm(closure), getClosureEnv(closure));
}

long long evaluateToInteger(Node* builtin, Hold* termClosure) {
    Hold* valueClosure = evaluateClosure(getNode(termClosure));
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

void evaluateBuiltinNode(State* state) {
    applyUpdates(state);
    setNode(state, evaluateBuiltin(getNode(state->node), state->stack));
}

static inline Hold* getResult(State* state) {
    evaluationErrorIf(!isUpdatesOnly(state->stack), NULL, "extra arguments");
    return hold(newClosure(getNode(state->node), getHead(state->env)));
}

void debugState(Node* node, Stack* stack, Stack* env) {
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

Hold* evaluateNode(State* state) {
    while (true) {
        debugState(getNode(state->node), state->stack, state->env);
        LOOP_COUNT += 1;
        Node* node = getNode(state->node);
        if (isApplication(node)) {
            evaluateApplicationNode(state);
        } else if (isAbstraction(node)) {
            if (isEmpty(state->stack))
                return getResult(state);
            evaluateAbstractionNode(state);
        } else if (isInteger(node)) {
            return getResult(state);
        } else if (isReference(node)) {
            evaluateReferenceNode(state);
        } else if (isBuiltin(node)) {
            evaluateBuiltinNode(state);
        } else {
            assert(false);
        }
    }
}

void debugLoopCount(int loopCount) {
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
