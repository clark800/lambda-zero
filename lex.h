#ifndef LEX_H
#define LEX_H

#include <stdbool.h>
#include <stdio.h>
#include "lib/tree.h"

Hold* getFirstToken(const char* input);
Hold* getNextToken(Hold* lastToken);
bool isSameToken(Node* tokenA, Node* tokenB);
bool isThisToken(Node* token, const char* tokenString);
bool isInternalToken(Node* token);
void printToken(Node* token, FILE* stream);
void throwTokenError(const char* type, const char* message, Node* token);
void syntaxError(const char* message, Node* token);
void syntaxErrorIf(bool condition, Node* token, const char* message);
Node* newEOF(void);

#endif
