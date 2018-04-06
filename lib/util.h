#ifndef LLTOA_H
#define LLTOA_H

#include <stdio.h>

#define READ_CHUNK_SIZE 16384

void* error(const char* message);
void* smalloc(size_t size);
char* lltoa(long long n, char* buffer, int radix);
void fputll(long long n, FILE* stream);
char* readfile(FILE* stream);

#endif
