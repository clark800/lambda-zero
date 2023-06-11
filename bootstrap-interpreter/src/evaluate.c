#include <signal.h>
#include "tree.h"
#include "array.h"
#include "stack.h"
#include "parse/term.h"
#include "closure.h"
#include "exception.h"
#include "operations.h"
#include "evaluate.h"

extern bool isIO;
static volatile bool INTERRUPT = false;

typedef const Array Globals;
static Node* evaluateClosure(Closure* closure, Globals* globals);

static bool isUpdate(Closure* closure) {return getTerm(closure) == VOID;}
static Closure* getUpdateClosure(Closure* update) {return getLocals(update);}

static Closure* newUpdate(Closure* closure) {
    return newClosure(VOID, closure, VOID);
}

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
        // only optimize in IO mode so that term serializations are standardized
        if (isIO && !isValue(getTerm(referee)))
            push(stack, newUpdate(referee));
        setClosure(closure, referee);
    }
}

static void evaluateOperation(Closure* closure, Stack* stack, Globals* globals){
    unsigned int arity = getArity(getTerm(closure));
    setLocals(closure, VOID);
    applyUpdates(closure, stack);
    Hold* left = arity >= 1 && !isEmpty(stack) ? pop(stack) : NULL;
    eraseUpdates(stack);    // partially applied operations are not values
    Hold* right = arity >= 2 && !isEmpty(stack) ? pop(stack) : NULL;

    // save the closure data since evaluate will mutate the closure and we
    // may have to revert if the operation optimization does not work
    Hold* leftTerm = left == NULL ? NULL : hold(getTerm(getNode(left)));
    Hold* leftLocals = left == NULL ? NULL : hold(getLocals(getNode(left)));
    Hold* rightTerm = right == NULL ? NULL : hold(getTerm(getNode(right)));
    Hold* rightLocals = right == NULL ? NULL : hold(getLocals(getNode(right)));

    // left and right may be mutated in evaluateClosure
    Hold* result = evaluateOperationTerm(closure,
        left == NULL ? NULL : evaluateClosure(getNode(left), globals),
        right == NULL ? NULL : evaluateClosure(getNode(right), globals));

    if (result == NULL) {
        // restore stack to it's original state
        if (right != NULL) {
            setTerm(getNode(right), getNode(rightTerm));
            setLocals(getNode(right), getNode(rightLocals));
            push(stack, getNode(right));
        }
        if (left != NULL) {
            setTerm(getNode(left), getNode(leftTerm));
            setLocals(getNode(left), getNode(leftLocals));
            push(stack, getNode(left));
        }
    }
    if (right != NULL) {
        release(rightTerm);
        release(rightLocals);
        release(right);
    }
    if (left != NULL) {
        release(leftTerm);
        release(leftLocals);
        release(left);
    }
    if (result == NULL) {
        Node* fallback = getRight(getTerm(closure));
        if (fallback == NULL)  // pseudo-operation, no definiens term
            runtimeError("missing argument to", closure);
        setTerm(closure, fallback);
    } else {
        setClosure(closure, getNode(result));
        release(result);
    }
}

static Term* expandNumeral(Term* numeral) {
    long long n = getValue(numeral);
    Tag tag = renameTag(getTag(numeral), "_", 0);
    Term* body = n == 0 ? Variable(tag, 2) :
       Application(tag, Variable(tag, 1), Numeral(tag, n - 1));
    return Abstraction(tag, Abstraction(tag, body));
}

static void evaluateNumeral(Closure* closure) {
    setLocals(closure, VOID);
    setTerm(closure, expandNumeral(getTerm(closure)));
}

static Closure* evaluate(Closure* closure, Stack* stack, Globals* globals) {
    while (true) {
        if (INTERRUPT)
            runtimeError("interrupted", closure);
        TermType type = getTermType(getTerm(closure));
        if (isValueType(type)) {
            applyUpdates(closure, stack);
            if (isEmpty(stack))
                return closure;
        }
        switch (type) {
            case VARIABLE: evaluateReference(closure, stack, globals); break;
            case ABSTRACTION: evaluateLambda(closure, stack); break;
            case APPLICATION: evaluateApplication(closure, stack); break;
            case NUMERAL: evaluateNumeral(closure); break;
            case OPERATION: evaluateOperation(closure, stack, globals); break;
        }
    }
}

static Closure* evaluateClosure(Closure* closure, Globals* globals) {
    if (isValue(getTerm(closure)))
        return closure;
    Stack* stack = newStack();
    Node* result = evaluate(closure, stack, globals);
    deleteStack(stack);
    return result;
}

static void interrupt(int parameter) {(void)parameter, INTERRUPT = true;}

Hold* evaluateTerm(Term* term, Globals* globals) {
    INPUT_STACK = newStack();
    Hold* closure = hold(newClosure(term, VOID, VOID));
    signal(SIGINT, interrupt);
    Hold* result = hold(evaluateClosure(getNode(closure), globals));
    signal(SIGINT, SIG_DFL);
    release(closure);
    deleteStack(INPUT_STACK);
    return result;
}
