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

static inline void errorIf(bool condition, Node* token, const char* message) {
    if (condition)
        throwTokenError("Evaluation", message, token);
}

static inline void moveStackItem(Stack* fromStack, Stack* toStack) {
    Hold* item = pop(fromStack);
    push(toStack, getNode(item));
    release(item);
}

static inline bool isUpdateNext(Stack* stack) {
    return !isEmpty(stack) && isUpdate(peek(stack, 0));
}

static inline void applyUpdates(Closure* evaluatedClosure, Stack* stack) {
    while (isUpdateNext(stack)) {
        Hold* update = pop(stack);
        Closure* closureToUpdate = getUpdateClosure(getNode(update));
        setClosure(closureToUpdate, evaluatedClosure);
        release(update);
    }
}

static inline Closure* getReferencedClosure(Node* reference, Node* locals) {
    return getListElement(locals, getDebruijnIndex(reference) - 1);
}

static inline Node* getGlobalValue(Node* global, const Array* globals) {
    return elementAt(globals, getGlobalIndex(global));
}

static inline Closure* optimizeClosure(Node* node, Node* locals,
        const Array* globals) {
    // the default case works for all node types;
    // the other cases are short-circuit optmizations
    switch (getNodeType(node)) {
        case N_REFERENCE: return getReferencedClosure(node, locals);
        case N_BUILTIN:
        case N_INTEGER: return newClosure(node, VOID);
        case N_GLOBAL: return newClosure(getGlobalValue(node, globals), VOID);
        default: return newClosure(node, locals);
    }
}

static inline void evaluateApplicationNode(Closure* closure, Stack* stack,
        const Array* globals) {
    Node* application = getTerm(closure);
    Node* lambda = getLeft(application);
    Node* argument = getRight(application);
    push(stack, optimizeClosure(argument, getLocals(closure), globals));
    setTerm(closure, lambda);
}

static inline void evaluateLambdaNode(Closure* closure, Stack* stack) {
    moveStackItem(stack, (Stack*)closure);
    setTerm(closure, getBody(getTerm(closure)));
}

static inline bool isValue(Node* node) {
    return (isLambda(node) || isInteger(node) ||
            isBuiltin(node) || isGlobal(node));
}

static inline void evaluateReferenceNode(Closure* closure, Stack* stack) {
    Closure* referencedClosure = getReferencedClosure(
            getTerm(closure), getLocals(closure));
    if (LAZY && !isValue(getTerm(referencedClosure)))
        push(stack, newUpdate(referencedClosure));
    setClosure(closure, referencedClosure);
}

static inline void evaluateGlobalNode(Closure* closure, const Array* globals) {
   setTerm(closure, getGlobalValue(getTerm(closure), globals));
   setLocals(closure, VOID);
}

static inline Hold* evaluateClosure(Closure* closure, const Array* globals) {
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

static inline void evaluateBuiltinNode(Closure* closure, Stack* stack,
        const Array* globals) {
    applyUpdates(closure, stack);
    setTerm(closure, evaluateBuiltin(getTerm(closure), stack, globals));
}

static inline Hold* getIntegerResult(Closure* closure, Stack* stack) {
    applyUpdates(closure, stack);
    if (!isEmpty(stack))
        errorIf(true, getTerm(peek(stack, 0)), "extra argument");
    return hold(closure);
}

static inline void debugState(Closure* closure, Stack* stack) {
    if (TRACE) {
        debugLine();
        debug("term: ");
        debugAST(getTerm(closure));
        debug("\nstack: ");
        debugStack(stack, (Node* (*)(Node*))getTerm);
        debug("\nlocals: ");
        debugStack((Stack*)closure, (Node* (*)(Node*))getTerm);
        debug("\n");
    }
}

static inline Hold* evaluateNode(
        Closure* closure, Stack* stack, const Array* globals) {
    while (true) {
        debugState(closure, stack);
        LOOP_COUNT += 1;
        switch (getNodeType(getTerm(closure))) {
            case N_APPLICATION:
                evaluateApplicationNode(closure, stack, globals); break;
            case N_REFERENCE: evaluateReferenceNode(closure, stack); break;
            case N_BUILTIN: evaluateBuiltinNode(closure, stack, globals); break;
            case N_GLOBAL: evaluateGlobalNode(closure, globals); break;
            case N_INTEGER: return getIntegerResult(closure, stack);
            case N_LAMBDA:
                applyUpdates(closure, stack);
                if (isEmpty(stack))
                    return hold(closure);
                evaluateLambdaNode(closure, stack);
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
    Hold* closure = hold(newClosure(term, locals));
    Stack* stack = newStack(VOID);
    Hold* result = evaluateNode(getNode(closure), stack, globals);
    release(closure);
    deleteStack(stack);
    debugLoopCount(LOOP_COUNT);
    return result;
}
