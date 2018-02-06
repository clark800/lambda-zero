#ifndef PARSE_H
#define PARSE_H

#include "lib/tree.h"
#include "lib/stack.h"

Hold* parse(const char* input);
void syntaxErrorIf(bool condition, Node* token, const char* message);

#endif
