#include <stddef.h>
#include <stdbool.h>
#include "lib/tree.h"
#include "ast.h"
#include "objects.h"

bool IO = false;

// put(c) = id              -- with side effect of printing the character c
// print(c) = (put c) (cs -> cs print)   -- pass this into a string to print it
// print = c -> (put c) (cs -> cs print)                 -- desugar
// print = Y (print -> c -> (put c) (cs -> cs print))    -- desugar

const char* OBJECTS =
    "($x -> $x)\n"                                          // identity
    "($t -> $f -> $f)\n"                                    // false
    "($z -> ($t -> $f -> $t))\n"                            // nil
    "($y -> ($q -> $y ($q $q)) ($q -> $y ($q $q))) "        // Y combinator
    "($print -> $c -> (($put $c) ($cs -> $cs $print)))\n"   // string printer
    "($get 0)\n"                                            // lazy input list
    "($ -> $)";                                             // terminator

Hold* OBJECTS_HOLD = NULL;
Node* IDENTITY = NULL;
Node* NIL = NULL;
Node* TRUE = NULL;
Node* FALSE = NULL;
Node* YCOMBINATOR = NULL;
Node* PARAMETERX = NULL;
Node* REFERENCEX = NULL;
Node* PRINT = NULL;
Node* INPUT = NULL;
Node* GET_BUILTIN = NULL;

Node* getElement(Node* node, unsigned int n) {
    for (unsigned int i = 0; i < n; i++)
        node = getRight(node);
    return getLeft(node);
}

void initObjects(Hold* objectsHold) {
    OBJECTS_HOLD = objectsHold;
    Node* objects = getNode(OBJECTS_HOLD);
    negateLocations(objects);
    IDENTITY = getElement(objects, 0);
    PARAMETERX = getParameter(IDENTITY);
    REFERENCEX = getBody(IDENTITY);
    FALSE = getElement(objects, 1);
    NIL = getElement(objects, 2);
    TRUE = getBody(NIL);
    PRINT = getElement(objects, 3);
    YCOMBINATOR = getLeft(PRINT);
    INPUT = getElement(objects, 4);
    GET_BUILTIN = getLeft(INPUT);
}

void deleteObjects() {
    release(OBJECTS_HOLD);
}

Node* newNil(int location) {
    return newLambda(location, PARAMETERX, TRUE);
}

Node* prepend(Node* item, Node* list) {
    int location = getLocation(list);
    return newLambda(location, PARAMETERX, newApplication(location,
            newApplication(location, REFERENCEX, item), list));
}
