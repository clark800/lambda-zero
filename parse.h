#ifndef PARSE_H
#define PARSE_H

#include <stdbool.h>
#include "ast.h"

extern bool TRACE_PARSING;

Program parse(const char* input, bool optimize);

#endif
