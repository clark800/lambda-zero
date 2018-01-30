#ifndef SCAN_H
#define SCAN_H

#include <stddef.h>
#include <stdbool.h>

const char* getFirstLexeme(const char* input);
const char* getNextLexeme(const char* lastLexeme);
size_t getLexemeLength(const char* lexeme);
bool isSameLexeme(const char* a, const char* b);

#endif
