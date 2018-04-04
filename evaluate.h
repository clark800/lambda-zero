#ifndef EVALUATE_H
#define EVALUATE_H

#include <stdbool.h>
#include "lib/tree.h"
#include "lib/array.h"
#include "closure.h"

extern bool TRACE;

Hold* evaluateClosure(Closure* closure, const Array* globals);
Hold* evaluateTerm(Node* term, const Array* globals);

#endif
