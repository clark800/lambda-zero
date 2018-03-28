#ifndef LLTOA_H
#define LLTOA_H

#include <stdio.h>

char* lltoa(long long n, char* buffer, int radix);
void fputll(long long n, FILE* stream);

#endif
