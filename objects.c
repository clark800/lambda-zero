#include "objects.h"

const char* INTERNAL_CODE =
    "(_ -> _) \n (,) -> (,) \n"                     // identity, empty tuple
    "(t -> f -> t) \n (t -> f -> f) \n"             // true, false
    "(y -> (x -> y (x x)) (x -> y (x x))) "         // Y combinator
    "(print -> c -> ((put c) (cs -> cs print))) \n" // lazy string printer
    "(get 0) \n 0";                                 // lazy input string

Program PROGRAM;
Node *IDENTITY, *UNIT, *TRUE, *FALSE, *YCOMBINATOR, *PRINT, *INPUT;
Stack* INPUT_STACK;

void initObjects(Program program) {
    PROGRAM = program;
    Node* objects = getNode(program.root);
    negateLocations(objects);
    IDENTITY = getListElement(objects, 0);
    UNIT = getListElement(objects, 1);
    TRUE = getListElement(objects, 2);
    FALSE = getListElement(objects, 3);
    PRINT = getListElement(objects, 4);
    YCOMBINATOR = getLeft(PRINT);
    INPUT = getListElement(objects, 5);
    INPUT_STACK = newStack(VOID);
}

void deleteObjects() {
    deleteProgram(PROGRAM);
    deleteStack(INPUT_STACK);
}

Node* newNil(int location) {
    return newLambda(location, getParameter(IDENTITY), TRUE);
}

Node* prepend(int location, Node* item, Node* list) {
    return newLambda(location, getParameter(IDENTITY), newApplication(location,
            newApplication(location, getBody(IDENTITY), item), list));
}

Node* newUnit(int location) {
    return newLambda(location, getParameter(UNIT), getBody(UNIT));
}

Node* newSingleton(int location, Node* item) {
    return newLambda(location, getParameter(UNIT),
        newApplication(getLocation(getBody(UNIT)), getBody(UNIT), item));
}
