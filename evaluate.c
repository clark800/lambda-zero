#include <assert.h>
#include <stdbool.h>
#include "lib/tree.h"
#include "lib/stack.h"
#include "lib/array.h"
#include "ast.h"
#include "closure.h"
#include "builtins.h"
#include "serialize.h"
#include "evaluate.h"

enum {LAZY=1};     // for debugging
bool PROFILE = false;
bool TRACE = false;
int LOOP_COUNT = 0;

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

static inline Hold* popBuiltinArgument(Closure* closure, Stack* stack,
        const Array* globals, unsigned int position) {
    applyUpdates(closure, stack);
    errorIf(isEmpty(stack), getTerm(closure), "missing argument to");
    Hold* expression = pop(stack);
    if (!isStrictArgument(getTerm(closure), position))
        return expression;
    Hold* result = evaluateClosure(getNode(expression), globals);
    release(expression);
    return result;
}

static inline void evaluateBuiltinNode(
        Closure* closure, Stack* stack, const Array* globals) {
    Node* builtin = getTerm(closure);
    unsigned int arity = getBuiltinArity(builtin);
    Hold* left = arity > 0 ?
        popBuiltinArgument(closure, stack, globals, 0) : NULL;
    Hold* right = arity > 1 ?
        popBuiltinArgument(closure, stack, globals, 1) : NULL;
    Hold* result = evaluateBuiltin(builtin, getNode(left), getNode(right));
    setClosure(closure, getNode(result));
    release(result);
    if (left != NULL)
        release(left);
    if (right != NULL)
        release(right);
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
