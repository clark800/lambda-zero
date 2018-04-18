#ifndef ERRORS_H
#define ERRORS_H

#include <stdbool.h>
#include "lib/tree.h"
#include "closure.h"

extern bool TEST;

void lexerErrorIf(bool condition, const char* lexeme, const char* message);
void syntaxError(const char* message, Node* token);
void syntaxErrorIf(bool condition, const char* message, Node* token);
void printRuntimeError(const char* message, Closure* closure);
void runtimeError(const char* message, Closure* closure);
void printMemoryError(const char* label, long long bytes);
void usageError(const char* name);
void readError(const char* filename);

#endif
