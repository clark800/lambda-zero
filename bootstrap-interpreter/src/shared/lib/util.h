#include <stdio.h>

void* error(const char* message);
void* smalloc(size_t size);
char* lltoa(long long n, char* buffer, int radix);
void fputll(long long n, FILE* stream);
