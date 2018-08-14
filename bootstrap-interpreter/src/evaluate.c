#include "lib/tree.h"
#include "lib/array.h"
#include "lib/stack.h"
#include "ast.h"
#include "errors.h"
#include "debug.h"
#include "closure.h"
#include "builtins.h"
#include "evaluate.h"

bool TRACE_EVALUATION = false;

void applyUpdates(Closure* evaluatedClosure, Stack* stack) {
    while (!isEmpty(stack) && isUpdate(peek(stack, 0))) {
        Hold* update = pop(stack);
        Closure* closureToUpdate = getUpdateClosure(getNode(update));
        updateClosure(closureToUpdate, evaluatedClosure);
        release(update);
    }
}

Closure* getReferencedClosure(Node* reference, Node* locals) {
    return getListElement(locals, getValue(reference) - 1);
}

Node* getGlobalValue(Node* global, const Array* globals) {
    return elementAt(globals, (size_t)getValue(global));
}

Closure* optimizeClosure(Node* node, Node* locals, Node* trace) {
    // the default case works for all node types;
    // the other cases are short-circuit optmizations
    // don't short-circuit globals so that backtraces work
    switch (getNodeType(node)) {
        case BUILTIN:
        case INTEGER: return newClosure(node, VOID, trace);
        case REFERENCE: return getReferencedClosure(node, locals);
        default: return newClosure(node, locals, trace);
    }
}

void evaluateApplicationNode(Closure* closure, Stack* stack) {
    // push right side of application onto stack and step into left side
    Node* application = getTerm(closure);
    Node* lambda = getLeft(application);
    Node* argument = getRight(application);
    push(stack, optimizeClosure(argument, getLocals(closure),
        getTrace(closure)));
    setTerm(closure, lambda);
}

void evaluateLambdaNode(Closure* closure, Stack* stack) {
    // move argument from stack to local environment and step into body
    Hold* argument = pop(stack);
    push((Stack*)closure, getNode(argument));
    release(argument);
    setTerm(closure, getBody(getTerm(closure)));
}

bool isValue(Node* node) {
    return (isLambda(node) || isInteger(node) ||
            isBuiltin(node) || isGlobal(node));
}

void evaluateReferenceNode(Closure* closure, Stack* stack) {
    // lookup referenced closure in the local environment and switch to it
    Closure* referencedClosure = getReferencedClosure(
            getTerm(closure), getLocals(closure));
    if (!isValue(getTerm(referencedClosure)))
        push(stack, newUpdate(referencedClosure));
    setClosure(closure, referencedClosure);
}

void evaluateGlobalNode(Closure* closure, const Array* globals) {
    Node* global = getTerm(closure);
    setTerm(closure, getGlobalValue(global, globals));
    if (!TEST)     // backtraces are not shown in test mode
        push((Stack*)getBacktrace(closure), global);
    setLocals(closure, VOID);
}

Hold* popBuiltinArgument(Closure* closure, Stack* stack,
        const Array* globals, unsigned int position) {
    applyUpdates(closure, stack);
    if (isEmpty(stack))
        runtimeError("missing argument to", closure);
    Hold* expression = pop(stack);
    if (!isStrictArgument(getTerm(closure), position))
        return expression;
    Hold* result = evaluateClosure(getNode(expression), globals);
    release(expression);
    return result;
}

void evaluateBuiltinNode(Closure* closure, Stack* stack, const Array* globals) {
    Node* builtin = getTerm(closure);
    unsigned int arity = getBuiltinArity(builtin);
    Hold* left = arity > 0 ?
        popBuiltinArgument(closure, stack, globals, 0) : NULL;
    Hold* right = arity > 1 ?
        popBuiltinArgument(closure, stack, globals, 1) : NULL;
    Hold* result = evaluateBuiltin(closure, getNode(left), getNode(right));
    setClosure(closure, getNode(result));
    release(result);
    if (left != NULL)
        release(left);
    if (right != NULL)
        release(right);
}

void debugState(Closure* closure, Stack* stack) {
    if (TRACE_EVALUATION) {
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

Hold* evaluateNode(Closure* closure, Stack* stack, const Array* globals) {
    while (true) {
        debugState(closure, stack);
        switch (getNodeType(getTerm(closure))) {
            case APPLICATION: evaluateApplicationNode(closure, stack); break;
            case REFERENCE: evaluateReferenceNode(closure, stack); break;
            case BUILTIN: evaluateBuiltinNode(closure, stack, globals); break;
            case GLOBAL: evaluateGlobalNode(closure, globals); break;
            case INTEGER:
                applyUpdates(closure, stack);
                if (isEmpty(stack))
                    return hold(closure);
                runtimeError("extra argument", peek(stack, 0)); break;
            case LAMBDA:
                applyUpdates(closure, stack);
                if (isEmpty(stack))
                    return hold(closure);
                evaluateLambdaNode(closure, stack); break;
            default: assert(false);
        }
    }
}

Hold* evaluateClosure(Closure* closure, const Array* globals) {
    Stack* stack = newStack();
    Hold* result = evaluateNode(closure, stack, globals);
    deleteStack(stack);
    return result;
}

Hold* evaluateTerm(Node* term, const Array* globals) {
    INPUT_STACK = newStack();
    Hold* closure = hold(newClosure(term, VOID, VOID));
    Hold* result = evaluateClosure(getNode(closure), globals);
    release(closure);
    deleteStack(INPUT_STACK);
    return result;
}
