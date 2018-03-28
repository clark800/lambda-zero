#include "lib/tree.h"
#include "ast.h"
#include "objects.h"

// put(c) = id              -- with side effect of printing the character c
// print(c) = (put c) (cs -> cs print)   -- pass this into a string to print it
// print = c -> (put c) (cs -> cs print)                 -- desugar
// print = Y (print -> c -> (put c) (cs -> cs print))    -- desugar

const char* INTERNAL_CODE =
    // identity, true, false
    "('_' -> '_') \n ('t' -> 'f' -> 't') \n ('t' -> 'f' -> 'f') \n"
    // Y combinator
    "('y' -> ('x' -> 'y' ('x' 'x')) ('x' -> 'y' ('x' 'x'))) "
    // lazy string printer
    "('print' -> 'c' -> (('put' 'c') ('cs' -> 'cs' 'print'))) \n"
    // lazy input string
    "('get' 0) \n 0";

Program PROGRAM;
Node *IDENTITY, *TRUE, *FALSE, *YCOMBINATOR, *PRINT, *INPUT;

void initObjects(Program program) {
    PROGRAM = program;
    Node* objects = getNode(program.root);
    negateLocations(objects);
    IDENTITY = getListElement(objects, 0);
    TRUE = getListElement(objects, 1);
    FALSE = getListElement(objects, 2);
    PRINT = getListElement(objects, 3);
    YCOMBINATOR = getLeft(PRINT);
    INPUT = getListElement(objects, 4);
}

void deleteObjects() {
    deleteProgram(PROGRAM);
}

Node* newNil(int location) {
    return newLambda(location, getParameter(IDENTITY), TRUE);
}

Node* prepend(Node* item, Node* list) {
    int location = getLocation(list);
    return newLambda(location, getParameter(IDENTITY), newApplication(location,
            newApplication(location, getBody(IDENTITY), item), list));
}
