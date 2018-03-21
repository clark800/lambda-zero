#ifndef AST_H
#define AST_H

#include <assert.h>
#include <stdbool.h>
#include "lib/tree.h"
#include "lib/array.h"

// a symbol identifies a thing e.g. the parameter *named* x
// constructed from (x -> y) is not a symbol
// but the parameter name x is a symbol
// a parameter is not considered an atom because it is semantically
// a part of the lambda, not an object in it's own right

// node = branch | leaf
// branch = application | lambda
// during parsing:
// symbol = name | operator
// leaf = symbol | parameter
// after parsing symbols are converted to atoms:
// atom = reference | integer | builtin
// leaf = atom | parameter

// INTEGER must be zero and BUILTIN must be last
enum LeafType {INTEGER=0, NAME, OPERATOR, PARAMETER, BUILTIN};

// GLOBAL is not a builtin; it's a marker so that we can store global indices
// without colliding with builtin codes
enum BuiltinCode {PLUS=BUILTIN, MINUS, TIMES, DIVIDE, MODULUS,
      EQUAL, NOTEQUAL, LESSTHAN, GREATERTHAN, LESSTHANOREQUAL,
      GREATERTHANOREQUAL, PUT, GET, GLOBAL};

enum NodeType {
    N_INTEGER, N_BUILTIN, N_REFERENCE, N_GLOBAL, N_LAMBDA, N_APPLICATION};

typedef struct Program Program;
struct Program {
    Hold* root;
    Node* main;
    Array* globals;
};

static inline void deleteProgram(Program program) {
    release(program.root);
    deleteArray(program.globals);
}

// =============================================
// Functions to test if a node is a certain type
// =============================================

static inline bool isInteger(Node* node) {
    return isLeafNode(node) && getType(node) == INTEGER;
}

static inline bool isName(Node* node) {
    return isLeafNode(node) && getType(node) == NAME;
}

static inline bool isOperator(Node* node) {
    return isLeafNode(node) && getType(node) == OPERATOR;
}

static inline bool isParameter(Node* node) {
    return isLeafNode(node) && getType(node) == PARAMETER;
}

static inline bool isBuiltin(Node* node) {
    return isLeafNode(node) && (
            getType(node) >= BUILTIN && getType(node) < GLOBAL);
}

static inline bool isReference(Node* node) {
    return isLeafNode(node) && getType(node) < 0;
}

static inline bool isGlobal(Node* node) {
    return isLeafNode(node) && getType(node) >= GLOBAL;
}

static inline bool isSymbol(Node* node) {
    return isName(node) || isOperator(node);
}

static inline bool isApplication(Node* node) {
    return isBranchNode(node) && !isParameter(getLeft(node));
}

static inline bool isLambda(Node* node) {
    return isBranchNode(node) && isParameter(getLeft(node));
}

static inline enum NodeType getNodeType(Node* node) {
    if (isBranchNode(node))
        return isParameter(getLeft(node)) ? N_LAMBDA : N_APPLICATION;
    long long type = getType(node);
    if (type < 0)
        return N_REFERENCE;
    if (type == 0)
        return N_INTEGER;
    if (type < GLOBAL)
        return N_BUILTIN;
    return N_GLOBAL;
}

// ====================================
// Functions to get a value from a node
// ====================================

static inline unsigned long long getDebruijnIndex(Node* reference) {
    assert(isReference(reference));
    return (unsigned long long)(-getType(reference));
}

static inline unsigned long long getBuiltinCode(Node* builtin) {
    assert(isBuiltin(builtin));
    return (unsigned long long)getType(builtin);
}

static inline unsigned long long getGlobalIndex(Node* global) {
    assert(isGlobal(global));
    return (unsigned long long)(getType(global) - GLOBAL);
}

static inline long long getInteger(Node* integer) {
    assert(isInteger(integer));
    return getValue(integer);
}

static inline Node* getParameter(Node* lambda) {
    assert(isLambda(lambda));
    return getLeft(lambda);
}

static inline Node* getBody(Node* lambda) {
    assert(isLambda(lambda));
    return getRight(lambda);
}

// ================================
// Functions to construct new nodes
// ================================

static inline Node* newInteger(int location, long long value) {
    Node* result = newLeafNode(location, INTEGER);
    setValue(result, value);
    return result;
}

static inline Node* newName(int location) {
    return newLeafNode(location, NAME);
}

static inline Node* newParameter(int location) {
    return newLeafNode(location, PARAMETER);
}

static inline Node* newOperator(int location) {
    return newLeafNode(location, OPERATOR);
}

static inline Node* newReference(int location, unsigned long long debruijn) {
    return newLeafNode(location, -(long long)debruijn);
}

static inline Node* newLambda(int location, Node* parameter, Node* body) {
    assert(isParameter(parameter));
    return newBranchNode(location, parameter, body);
}

static inline Node* newApplication(int location, Node* left, Node* right) {
    return newBranchNode(location, left, right);
}

// =======================================
// Functions to convert between node types
// =======================================

static inline void convertSymbolToReference(Node* symbol,
    unsigned long long debruijn) {
    assert(isSymbol(symbol) && debruijn > 0);
    setType(symbol, -(long long)debruijn);
}

static inline void convertSymbolToBuiltin(Node* symbol,
    unsigned long long code) {
    assert(isSymbol(symbol) && code >= BUILTIN);
    setType(symbol, (long long)code);
}

static inline void convertSymbolToGlobal(Node* symbol,
        unsigned long long index) {
    assert(isSymbol(symbol));
    setType(symbol, (long long)(GLOBAL + index));
}

static inline void convertNameToParameter(Node* symbol) {
    assert(isName(symbol));
    setType(symbol, PARAMETER);
}

static inline void convertOperatorToName(Node* symbol) {
    assert(isOperator(symbol));
    setType(symbol, NAME);
}

#endif
