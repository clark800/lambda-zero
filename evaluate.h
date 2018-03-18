#ifndef EVALUATE_H
#define EVALUATE_H

#include <stdbool.h>
#include "lib/tree.h"
#include "lib/array.h"

extern bool PROFILE;
extern bool TRACE;

Hold* evaluate(Node* term, Node* locals, const Array* globals);

#endif
