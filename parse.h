#ifndef PARSE_H
#define PARSE_H

#include <stdbool.h>
#include "lib/tree.h"
#include "ast.h"

extern bool DEBUG;

Program parse(const char* input, bool optimize);

#endif
