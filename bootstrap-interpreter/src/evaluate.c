#include "lib/tree.h"
#include "lib/array.h"
#include "lib/stack.h"
#include "ast.h"
#include "closure.h"
#include "builtins.h"

typedef const Array Globals;
static Hold* evaluateClosure(Closure* closure, Globals* globals);

static void eraseUpdates(Stack* stack) {
    while (!isEmpty(stack) && isUpdate(peek(stack, 0)))
        release(pop(stack));
}

static void applyUpdates(Closure* evaluatedClosure, Stack* stack) {
    while (!isEmpty(stack) && isUpdate(peek(stack, 0))) {
        Hold* update = pop(stack);
        Closure* closureToUpdate = getUpdateClosure(getNode(update));
        updateClosure(closureToUpdate, evaluatedClosure);
        release(update);
    }
}

static Closure* getReferee(Node* reference, Node* locals) {
    return getListElement(locals, getDebruijnIndex(reference));
}

static Node* getGlobalValue(Node* global, Globals* globals) {
    return elementAt(globals, (size_t)getGlobalIndex(global));
}

static Closure* optimizeClosure(Node* node, Node* locals, Node* trace) {
    // the default case works for all node types;
    // the other cases are short-circuit optmizations
    switch (getNodeType(node)) {
        case BUILTIN:
        case NATURAL: return newClosure(node, VOID, trace);
        case SYMBOL: return isGlobalReference(node) ?
            newClosure(node, VOID, trace) : getReferee(node, locals);
        default: return newClosure(node, locals, trace);
    }
}

static void evaluateApplication(Closure* closure, Stack* stack) {
    // push right side of application onto stack and step into left side
    Node* application = getTerm(closure);
    push(stack, optimizeClosure(
        getRight(application), getLocals(closure), getTrace(closure)));
    setTerm(closure, getLeft(application));
}

static void evaluateLambda(Closure* closure, Stack* stack) {
    // move argument from stack to local environment and step into body
    Hold* argument = pop(stack);
    push((Stack*)closure, getNode(argument));
    release(argument);
    setTerm(closure, getBody(getTerm(closure)));
}

static bool isValue(Node* node) {
    return isLambda(node) || isNatural(node) || isBuiltin(node);
}

static void evaluateReference(Closure* closure, Stack* stack, Globals* globals){
    Node* reference = getTerm(closure);
    if (isGlobalReference(reference)) {
        setTerm(closure, getGlobalValue(reference, globals));
        if (!TEST)
            push((Stack*)getBacktrace(closure), reference);
        setLocals(closure, VOID);
    } else {
        // lookup referenced closure in the local environment and switch to it
        Closure* referee = getReferee(reference, getLocals(closure));
        if (!isValue(getTerm(referee)))
            push(stack, newUpdate(referee));
        setClosure(closure, referee);
    }
}

static Hold* popArgument(Closure* closure, Stack* stack, Globals* globals) {
    eraseUpdates(stack);    // partially applied builtins are not values
    if (isEmpty(stack))
        runtimeError("missing argument to", closure);
    Hold* expression = pop(stack);
    Hold* result = evaluateClosure(getNode(expression), globals);
    release(expression);
    return result;
}

static void evaluateBuiltin(Closure* closure, Stack* stack, Globals* globals) {
    unsigned int arity = getBuiltinArity(getTerm(closure));
    Hold* left = arity > 0 ? popArgument(closure, stack, globals) : NULL;
    Hold* right = arity > 1 ? popArgument(closure, stack, globals) : NULL;
    Hold* result = evaluateBuiltinNode(closure, getNode(left), getNode(right));
    setClosure(closure, getNode(result));
    release(result);
    if (left != NULL)
        release(left);
    if (right != NULL)
        release(right);
}

static Node* expandNatural(Node* natural) {
    long long n = getValue(natural);
    Tag tag = getTag(natural);
    Node* underscore = newUnderscore(tag, 0);
    Node* body = n == 0 ? newUnderscore(tag, 2) :
       newApplication(tag, newUnderscore(tag, 1), newNatural(tag, n - 1));
    return newLambda(tag, underscore, newLambda(tag, underscore, body));
}

static Hold* evaluate(Closure* closure, Stack* stack, Globals* globals) {
    while (true) {
        //#include "debug.h"
        //extern void debugState(Closure* closure, Stack* stack);
        //debugState(closure, stack);
        switch (getNodeType(getTerm(closure))) {
            case APPLICATION: evaluateApplication(closure, stack); break;
            case SYMBOL: evaluateReference(closure, stack, globals); break;
            case BUILTIN: evaluateBuiltin(closure, stack, globals); break;
            case NATURAL:
                applyUpdates(closure, stack);
                if (isEmpty(stack))
                    return hold(closure);
                setTerm(closure, expandNatural(getTerm(closure))); break;
            case LAMBDA:
                applyUpdates(closure, stack);
                if (isEmpty(stack))
                    return hold(closure);
                evaluateLambda(closure, stack); break;
            default: runtimeError("internal error", closure); break;
        }
    }
}

static Hold* evaluateClosure(Closure* closure, Globals* globals) {
    Stack* stack = newStack();
    Hold* result = evaluate(closure, stack, globals);
    deleteStack(stack);
    return result;
}

Hold* evaluateTerm(Node* term, Globals* globals) {
    INPUT_STACK = newStack();
    Hold* closure = hold(newClosure(term, VOID, VOID));
    Hold* result = evaluateClosure(getNode(closure), globals);
    release(closure);
    deleteStack(INPUT_STACK);
    return result;
}
