#ifndef AST_H
#define AST_H

#include <assert.h>
#include <stdbool.h>
#include "lib/tree.h"
#include "lex.h"

// a symbol identifies a thing e.g. the parameter *named* x
// constructed from (x -> y) is not a symbol
// but the parameter name x is a symbol
// a parameter is not considered an atom because it is semantically
// a part of the lambda, not an object in it's own right

// node = branch | leaf
// branch = application | abstraction
// during parsing:
// symbol = name | operator
// leaf = symbol | parameter
// after parsing symbols are converted to atoms:
// atom = reference | integer | builtin
// leaf = atom | parameter

// INTEGER must be zero and BUILTIN must be last
enum {INTEGER, NAME, OPERATOR, PARAMETER, BUILTIN};

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
    return isLeafNode(node) && getType(node) >= BUILTIN;
}

static inline bool isReference(Node* node) {
    return isLeafNode(node) && getType(node) < 0;
}

static inline bool isSymbol(Node* node) {
    return isName(node) || isOperator(node);
}

static inline bool isApplication(Node* node) {
    return isBranchNode(node) && !isParameter(getLeft(node));
}

static inline bool isAbstraction(Node* node) {
    return isBranchNode(node) && isParameter(getLeft(node));
}

static inline bool isCommaBranch(Node* node) {
    return isBranchNode(node) && isThisToken(node, ",");
}

static inline bool isNewline(Node* node) {
    return isLeafNode(node) && isThisToken(node, "\n");
}

static inline bool isOpenParen(Node* node) {
    return isLeafNode(node) && isThisToken(node, "(");
}

static inline bool isCloseParen(Node* node) {
    return isLeafNode(node) && isThisToken(node, ")");
}

static inline bool isEOF(Node* node) {
    return isLeafNode(node) && isThisToken(node, "\0");
}

static inline bool isSpace(Node* node) {
    return isLeafNode(node) && isThisToken(node, " ");
}

static inline bool isAssignment(Node* node) {
    return isBranchNode(node) && !isAbstraction(node) && isThisToken(node, "=");
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

static inline long long getInteger(Node* integer) {
    assert(isInteger(integer));
    return getValue(integer);
}

static inline Node* getParameter(Node* abstraction) {
    assert(isAbstraction(abstraction));
    return getLeft(abstraction);
}

static inline Node* getBody(Node* abstraction) {
    assert(isAbstraction(abstraction));
    return getRight(abstraction);
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

static inline void convertNameToParameter(Node* symbol) {
    assert(isName(symbol));
    setType(symbol, PARAMETER);
}

static inline void convertOperatorToName(Node* symbol) {
    assert(isOperator(symbol));
    setType(symbol, NAME);
}

#endif
