#ifndef LEX_H
#define LEX_H

#include <stdbool.h>
#include <stdio.h>
#include "lib/tree.h"
#include "ast.h"

Hold* getFirstToken(const char* input);
Hold* getNextToken(Hold* lastToken);
bool isSameToken(Node* tokenA, Node* tokenB);
bool isSpace(Node* node);
bool isThisToken(Node* token, const char* tokenString);
void printToken(Node* token, FILE* stream);
Node* newEOF(void);

static inline bool isNewline(Node* node) {
    return isLeaf(node) && isThisToken(node, "\n");
}

static inline bool isDefinition(Node* node) {
    return isApplication(node) && isThisToken(node, "=");
}

static inline bool isOpenParen(Node* node) {
    return isLeaf(node) && isThisToken(node, "(");
}

static inline bool isCloseParen(Node* node) {
    return isLeaf(node) && isThisToken(node, ")");
}

static inline bool isEOF(Node* node) {
    return isLeaf(node) && isThisToken(node, "\0");
}

static inline bool isComma(Node* node) {
    return isLeaf(node) && isThisToken(node, ",");
}

static inline bool isCommaList(Node* node) {
    return isApplication(node) && isThisToken(node, ",");
}

static inline bool isTuple(Node* node) {
    // must check the body to exclude the definition of ","
    return isLambda(node) && isComma(getParameter(node)) &&
        (isComma(getBody(node)) || isCommaList(getBody(node)));
}

static inline bool isList(Node* node) {
    return isLambda(node) && isThisToken(node, "[");
}

#endif
