#ifndef EVALUATE_H
#define EVALUATE_H

#include <stdbool.h>
#include "lib/tree.h"

extern bool PROFILE;
extern bool TRACE;

Node* getClosureTerm(Node* closure);
Node* getClosureEnv(Node* closure);
Hold* evaluate(Node* term, Node* env);

#endif
