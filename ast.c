#include "lib/tree.h"
#include "scan.h"
#include "ast.h"

Node *IDENTITY, *UNIT, *TRUE, *FALSE, *YCOMBINATOR, *PRINT, *INPUT;

const char* OBJECTS_CODE =
    "(_ -> _) \n (,) -> (,) \n"                     // identity, empty tuple
    "(t -> f -> t) \n (t -> f -> f) \n"             // true, false
    "(y -> (x -> y (x x)) (x -> y (x x))) "         // Y combinator
    "(print -> c -> ((put c) (cs -> cs print))) \n" // lazy string printer
    "(get 0) \n 0";                                 // lazy input string

void initObjects(Node* objects) {
    negateLocations(objects);
    IDENTITY = getListElement(objects, 0);
    UNIT = getListElement(objects, 1);
    TRUE = getListElement(objects, 2);
    FALSE = getListElement(objects, 3);
    PRINT = getListElement(objects, 4);
    YCOMBINATOR = getLeft(PRINT);
    INPUT = getListElement(objects, 5);
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

const char* getLexeme(Node* node) {
    return getLexemeByLocation(getLocation(node));
}

bool isSameToken(Node* tokenA, Node* tokenB) {
    return isSameLexeme(getLexeme(tokenA), getLexeme(tokenB));
}

bool isThisToken(Node* token, const char* lexeme) {
    return isSameLexeme(getLexeme(token), lexeme);
}

bool isSpace(Node* token) {
    return isLeaf(token) && isSpaceCharacter(getLexeme(token)[0]);
}
