#ifndef PARSE_H
#define PARSE_H

#include <stdbool.h>
#include "ast.h"

Program parse(const char* input, bool optimize, bool showDebug);

#endif
