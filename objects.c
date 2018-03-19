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

Program PROGRAM;
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

void initObjects(Program program) {
    PROGRAM = program;
    Node* objects = getNode(program.root);
    negateLocations(objects);
    IDENTITY = getListElement(objects, 0);
    PARAMETERX = getParameter(IDENTITY);
    REFERENCEX = getBody(IDENTITY);
    FALSE = getListElement(objects, 1);
    NIL = getListElement(objects, 2);
    TRUE = getBody(NIL);
    PRINT = getListElement(objects, 3);
    YCOMBINATOR = getLeft(PRINT);
    INPUT = getListElement(objects, 4);
    GET_BUILTIN = getLeft(INPUT);
}

void deleteObjects() {
    deleteProgram(PROGRAM);
}

Node* newNil(int location) {
    return newLambda(location, PARAMETERX, TRUE);
}

Node* prepend(Node* item, Node* list) {
    int location = getLocation(list);
    return newLambda(location, PARAMETERX, newApplication(location,
            newApplication(location, REFERENCEX, item), list));
}
