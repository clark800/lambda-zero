#include "lib/tree.h"
#include "ast.h"

Node *IDENTITY, *UNIT, *TRUE, *FALSE, *YCOMBINATOR, *PRINT, *INPUT;

const char OBJECTS_CODE[] =
    "(_ -> _) \n (,) -> (,) \n"                     // identity, empty tuple
    "(t -> f -> t) \n (t -> f -> f) \n"             // true, false
    "(y -> (x -> y (x x)) (x -> y (x x))) "         // Y combinator
    "(print -> c -> ((put c) (cs -> cs print))) \n" // lazy string printer
    "(get 0) \n 0";                                 // lazy input string

void initObjects(Node* objects) {
    IDENTITY = getListElement(objects, 0);
    UNIT = getListElement(objects, 1);
    TRUE = getListElement(objects, 2);
    FALSE = getListElement(objects, 3);
    PRINT = getListElement(objects, 4);
    YCOMBINATOR = getLeft(PRINT);
    INPUT = getListElement(objects, 5);
}

bool isInternal(String lexeme) {
    const char* start = lexeme.start;
    const char* limit = OBJECTS_CODE + sizeof(OBJECTS_CODE)/sizeof(char);
    return start >= OBJECTS_CODE && start < limit;
}
