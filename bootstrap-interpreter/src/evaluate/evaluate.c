#include "lib/tree.h"
#include "lib/array.h"
#include "lib/stack.h"
#include "term.h"
#include "closure.h"
#include "operations.h"

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

static Closure* getReferee(Term* reference, Node* locals) {
    return getListElement(locals, getDebruijnIndex(reference) - 1);
}

static Term* getGlobalValue(Term* global, Globals* globals) {
    return elementAt(globals, (size_t)getGlobalIndex(global));
}

static Closure* optimizeClosure(Term* term, Node* locals, Node* trace) {
    // the default case works for all term types;
    // the other cases are short-circuit optmizations
    switch (getTermType(term)) {
        case OPERATION:
        case NUMERAL: return newClosure(term, VOID, trace);
        case VARIABLE: return isGlobal(term) ?
            newClosure(term, VOID, trace) : getReferee(term, locals);
        default: return newClosure(term, locals, trace);
    }
}

static void evaluateApplication(Closure* closure, Stack* stack) {
    // push right side of application onto stack and step into left side
    Term* application = getTerm(closure);
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

static bool isValue(Term* term) {
    return isAbstraction(term) || isNumeral(term) || isOperation(term);
}

static void evaluateReference(Closure* closure, Stack* stack, Globals* globals){
    Term* reference = getTerm(closure);
    if (isGlobal(reference)) {
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
    eraseUpdates(stack);    // partially applied operations are not values
    if (isEmpty(stack))
        runtimeError("missing argument to", closure);
    Hold* expression = pop(stack);
    Hold* result = evaluateClosure(getNode(expression), globals);
    release(expression);
    return result;
}

static void evaluateOperation(Closure* closure, Stack* stack, Globals* globals){
    unsigned int arity = getArity(getTerm(closure));
    Hold* left = arity > 0 ? popArgument(closure, stack, globals) : NULL;
    Hold* right = arity > 1 ? popArgument(closure, stack, globals) : NULL;
    Hold* result = evaluateOperationNode(closure,
        getNode(left), getNode(right));
    setClosure(closure, getNode(result));
    release(result);
    if (left != NULL)
        release(left);
    if (right != NULL)
        release(right);
}

static Term* expandNumeral(Term* numeral) {
    long long n = getValue(numeral);
    Tag tag = getTag(numeral);
    Term* body = n == 0 ? Variable(tag, 2) :
       Application(tag, Variable(tag, 1), Numeral(tag, n - 1));
    return Abstraction(tag, Abstraction(tag, body));
}

static Hold* evaluate(Closure* closure, Stack* stack, Globals* globals) {
    while (true) {
        //#include "debug.h"
        //extern void debugState(Closure* closure, Stack* stack);
        //debugState(closure, stack);
        switch (getTermType(getTerm(closure))) {
            case APPLICATION: evaluateApplication(closure, stack); break;
            case VARIABLE: evaluateReference(closure, stack, globals); break;
            case OPERATION: evaluateOperation(closure, stack, globals); break;
            case NUMERAL:
                applyUpdates(closure, stack);
                if (isEmpty(stack))
                    return hold(closure);
                setTerm(closure, expandNumeral(getTerm(closure))); break;
            case ABSTRACTION:
                applyUpdates(closure, stack);
                if (isEmpty(stack))
                    return hold(closure);
                evaluateLambda(closure, stack); break;
            default:
                assert(false); break;
        }
    }
}

static Hold* evaluateClosure(Closure* closure, Globals* globals) {
    Stack* stack = newStack();
    Hold* result = evaluate(closure, stack, globals);
    deleteStack(stack);
    return result;
}

Hold* evaluateTerm(Term* term, Globals* globals) {
    INPUT_STACK = newStack();
    Hold* closure = hold(newClosure(term, VOID, VOID));
    Hold* result = evaluateClosure(getNode(closure), globals);
    release(closure);
    deleteStack(INPUT_STACK);
    return result;
}
