#include <assert.h>
#include <stdbool.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "lib/array.h"
#include "ast.h"
#include "closure.h"
#include "builtins.h"
#include "lex.h"
#include "serialize.h"
#include "evaluate.h"

enum {LAZY=1};     // for debugging
bool PROFILE = false;
bool TRACE = false;
int LOOP_COUNT = 0;

typedef struct State State;

// note: cannot use an array for locals because it is a cactus stack
struct State {
    Hold* node;
    Stack* stack;
    Stack* locals;
};

static inline void errorIf(bool condition, Node* token, const char* message) {
    if (condition)
        throwTokenError("Evaluation", message, token);
}

static inline Node* newUpdate(Node* closure) {
    return newClosure(VOID, closure);
}

static inline bool isUpdate(Node* closure) {
    return getLeft(closure) == VOID;
}

static inline Node* getUpdateClosure(Node* update) {
    return getRight(update);
}

static inline bool isUpdateNext(Stack* stack) {
    return !isEmpty(stack) && isUpdate(peek(stack, 0));
}

static inline void initState(State* state, Node* root, Node* locals) {
    state->node = hold(root);
    state->stack = newStack(VOID);
    state->locals = newStack(locals);
}

static inline void deleteState(State* state) {
    release(state->node);
    deleteStack(state->stack);
    deleteStack(state->locals);
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
    Node* closure = getUpdateClosure(getNode(update));
    updateClosure(closure, getNode(state->node), getHead(state->locals));
    release(update);
}

static inline void applyUpdates(State* state) {
    while (isUpdateNext(state->stack))
        applyUpdate(state);
}

static inline Node* getReferencedClosure(Node* reference, Stack* locals) {
    return peek(locals, getDebruijnIndex(reference) - 1);
}

static inline Node* getArgumentClosure(Node* argument, Stack* locals) {
    if (isReference(argument))     // short-circuit optimization
        return getReferencedClosure(argument, locals);
    return newClosure(argument, getHead(locals));
}

static inline void evaluateApplicationNode(State* state) {
    Node* function = getLeft(getNode(state->node));
    Node* argument = getRight(getNode(state->node));
    push(state->stack, getArgumentClosure(argument, state->locals));
    setNode(state, function);
}

static inline void evaluateLambdaNode(State* state) {
    moveStackItem(state->stack, state->locals);
    Node* lambda = getNode(state->node);
    setNode(state, getBody(lambda));
}

static inline void evaluateReferenceNode(State* state) {
    Node* closure = getReferencedClosure(getNode(state->node), state->locals);
    if (LAZY && !isLambda(getTerm(closure)))
        push(state->stack, newUpdate(closure));
    setNode(state, getTerm(closure));
    setHead(state->locals, getLocals(closure));
}

static inline void evaluateGlobalNode(State* state, const Array* globals) {
   setNode(state, elementAt(globals, getGlobalIndex(getNode(state->node))));
   setHead(state->locals, VOID);
}

static inline Hold* evaluateClosure(Node* closure, const Array* globals) {
    return evaluate(getTerm(closure), getLocals(closure), globals);
}

static inline long long evaluateToInteger(Node* builtin, Hold* termClosure,
        const Array* globals) {
    Hold* valueClosure = evaluateClosure(getNode(termClosure), globals);
    Node* integerNode = getTerm(getNode(valueClosure));
    errorIf(!isInteger(integerNode), builtin, "expected integer argument");
    long long integer = getInteger(integerNode);
    release(valueClosure);
    release(termClosure);
    return integer;
}

static inline Node* evaluateBuiltin(Node* builtin, Stack* stack,
        const Array* globals) {
    int arity = getArity(builtin);
    if (arity == 0)
        return computeBuiltin(builtin, 0, 0);
    errorIf(isEmpty(stack), builtin, "missing first argument");
    long long left = evaluateToInteger(builtin, pop(stack), globals);
    if (arity == 1)
        return computeBuiltin(builtin, left, 0);
    errorIf(isEmpty(stack), builtin, "missing second argument");
    long long right = evaluateToInteger(builtin, pop(stack), globals);
    return computeBuiltin(builtin, left, right);
}

static inline void evaluateBuiltinNode(State* state, const Array* globals) {
    applyUpdates(state);
    setNode(state,
        evaluateBuiltin(getNode(state->node), state->stack, globals));
}

static inline Hold* getResult(State* state) {
    return hold(newClosure(getNode(state->node), getHead(state->locals)));
}

static inline Hold* getIntegerResult(State* state) {
    applyUpdates(state);
    if (!isEmpty(state->stack))
        errorIf(true, getTerm(peek(state->stack, 0)), "extra argument");
    return getResult(state);
}

static inline void debugState(Node* node, Stack* stack, Stack* locals) {
    if (TRACE) {
        debugLine();
        debug("node: ");
        debugAST(node);
        debug("\nstack: ");
        debugStack(stack, getTerm);
        debug("\nlocals: ");
        debugStack(locals, getTerm);
        debug("\n");
    }
}

static inline Hold* evaluateNode(State* state, const Array* globals) {
    while (true) {
        debugState(getNode(state->node), state->stack, state->locals);
        LOOP_COUNT += 1;
        switch (getNodeType(getNode(state->node))) {
            case N_APPLICATION: evaluateApplicationNode(state); break;
            case N_REFERENCE: evaluateReferenceNode(state); break;
            case N_BUILTIN: evaluateBuiltinNode(state, globals); break;
            case N_GLOBAL: evaluateGlobalNode(state, globals); break;
            case N_INTEGER: return getIntegerResult(state);
            case N_LAMBDA:
                applyUpdates(state);
                if (isEmpty(state->stack))
                    return getResult(state);
                evaluateLambdaNode(state);
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

Hold* evaluate(Node* term, Node* locals, const Array* globals) {
    State state;
    initState(&state, term, locals);
    Hold* result = evaluateNode(&state, globals);
    deleteState(&state);
    debugLoopCount(LOOP_COUNT);
    return result;
}
