#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <stdio.h>
#include "lib/tree.h"
#include "lib/stack.h"

extern bool DEBUG;
extern bool PROFILE;

void serializeClosure(Node* closure, FILE* stream);
void serializeInteger(long long n, FILE* stream);

void debug(const char* message);
void debugAST(const char* label, Node* node);
void debugParseState(Node* token, Stack* stack);
void debugEvalState(Node* term, Stack* stack, Stack* env);
void debugLoopCount(int loopCount);

#endif
