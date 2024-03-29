#include <signal.h>
#include "tree.h"
#include "stack.h"
#include "array.h"
#include "parse/term.h"
#include "closure.h"
#include "exception.h"
#include "operations.h"
#include "evaluate.h"

extern bool isIO;
static volatile bool INTERRUPT = false;

static Node* evaluateClosure(Closure* closure, Array* globals);

static bool isUpdate(Closure* closure) {
    return getVariety(closure) == 1;
}

static Closure* setUpdate(Closure* closure, bool update) {
    setVariety(closure, update ? 1 : 0);
    return closure;
}

static void eraseUpdates(Stack* stack) {
    while (!isEmpty(stack) && isUpdate(peek(stack, 0)))
        release(setUpdate(pop(stack), false));
}

static void applyUpdates(Closure* evaluatedClosure, Stack* stack) {
    while (!isEmpty(stack) && isUpdate(peek(stack, 0))) {
        Hold* update = pop(stack);
        updateClosure(setUpdate(update, false), evaluatedClosure);
        release(update);
    }
}

static Closure* getLocalReferent(Term* variable, Node* locals) {
    return getListElement(locals, getDebruijnIndex(variable) - 1);
}

static Closure* optimizeClosure(Term* term, Node* locals, Node* trace) {
    // the default case works for all term types;
    // the other cases are short-circuit optmizations
    switch (getTermType(term)) {
        case OPERATION:
        case NUMERAL: return newClosure(term, NULL, trace);
        case VARIABLE: return isGlobal(term) ?
            newClosure(term, NULL, trace) : getLocalReferent(term, locals);
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

static void evaluateAbstraction(Closure* closure, Stack* stack) {
    // move argument from stack to local environment and step into body
    Hold* argument = pop(stack);
    push((Stack*)closure, argument);
    release(argument);
    setTerm(closure, getBody(getTerm(closure)));
}

static void evaluateVariable(Closure* closure, Stack* stack, Array* globals) {
    Term* variable = getTerm(closure);
    if (isGlobal(variable)) {
        setTerm(closure, getGlobalReferent(variable, globals));
        if (TRACE)
            push((Stack*)getBacktrace(closure), variable);
        setLocals(closure, NULL);
    } else {
        // lookup referenced closure in the local environment and switch to it
        Closure* referent = getLocalReferent(variable, getLocals(closure));
        // only optimize in IO mode so that term serializations are standardized
        if (isIO && !isValue(getTerm(referent)) && !isUpdate(referent))
            push(stack, setUpdate(referent, true));
        setClosure(closure, referent);
    }
}

static void evaluateOperation(Closure* closure, Stack* stack, Array* globals) {
    unsigned int arity = getArity(getTerm(closure));
    setLocals(closure, NULL);
    applyUpdates(closure, stack);
    Hold* left = arity >= 1 && !isEmpty(stack) ? pop(stack) : NULL;
    eraseUpdates(stack);    // partially applied operations are not values
    Hold* right = arity >= 2 && !isEmpty(stack) ? pop(stack) : NULL;

    // save the closure data since evaluate will mutate the closure and we
    // may have to revert if the operation optimization does not work
    Hold* leftTerm = left == NULL ? NULL : hold(getTerm(left));
    Hold* leftLocals = left == NULL ? NULL : hold(getLocals(left));
    Hold* rightTerm = right == NULL ? NULL : hold(getTerm(right));
    Hold* rightLocals = right == NULL ? NULL : hold(getLocals(right));

    // left and right may be mutated in evaluateClosure
    Hold* result = evaluateOperationTerm(closure,
        left == NULL ? NULL : evaluateClosure(left, globals),
        right == NULL ? NULL : evaluateClosure(right, globals));

    if (result == NULL) {
        // restore stack to it's original state
        if (right != NULL) {
            setTerm(right, rightTerm);
            setLocals(right, rightLocals);
            push(stack, right);
        }
        if (left != NULL) {
            setTerm(left, leftTerm);
            setLocals(left, leftLocals);
            push(stack, left);
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
        setClosure(closure, result);
        release(result);
    }
}

static Term* expandNumeral(Term* numeral) {
    long long n = getValue(numeral);
    Tag tag = newLiteralTag("_", getLexeme(getTag(numeral)).location, 0);
    Term* body = n == 0 ? Variable(tag, 2) :
       Application(tag, Variable(tag, 1), Numeral(tag, n - 1));
    return Abstraction(tag, Abstraction(tag, body));
}

static void evaluateNumeral(Closure* closure) {
    setLocals(closure, NULL);
    setTerm(closure, expandNumeral(getTerm(closure)));
}

static Closure* evaluate(Closure* closure, Stack* stack, Array* globals) {
    while (true) {
        assert(INTERRUPT ? (runtimeError("interrupted", closure), 0) : 1);
        TermType type = getTermType(getTerm(closure));
        if (isValueType(type)) {
            applyUpdates(closure, stack);
            if (isEmpty(stack))
                return closure;
        }
        switch (type) {
            case VARIABLE: evaluateVariable(closure, stack, globals); break;
            case ABSTRACTION: evaluateAbstraction(closure, stack); break;
            case APPLICATION: evaluateApplication(closure, stack); break;
            case NUMERAL: evaluateNumeral(closure); break;
            case OPERATION: evaluateOperation(closure, stack, globals); break;
        }
    }
}

static Closure* evaluateClosure(Closure* closure, Array* globals) {
    if (isValue(getTerm(closure)))
        return closure;
    Stack* stack = newStack();
    Node* result = evaluate(closure, stack, globals);
    deleteStack(stack);
    return result;
}

static void interrupt(int parameter) {(void)parameter; INTERRUPT = true;}

Hold* evaluateTerm(Term* term, Array* globals) {
    (void)interrupt;
    INPUT_STACK = newStack();
    Hold* closure = hold(newClosure(term, NULL, NULL));
    assert(signal(SIGINT, interrupt) != SIG_ERR);
    Hold* result = hold(evaluateClosure(closure, globals));
    assert(signal(SIGINT, SIG_DFL) != SIG_ERR);
    release(closure);
    deleteStack(INPUT_STACK);
    return result;
}
