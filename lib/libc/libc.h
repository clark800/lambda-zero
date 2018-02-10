#ifndef LIBC_H
#define LIBC_H

#include "sys.h"

// errno.h
#define ERANGE 34
extern int errno;

// stddef.h
#define NULL ((void*)0)

// stdbool.h
#define true 1
#define false 0
typedef _Bool bool;

// stdint.h
typedef __INTPTR_TYPE__ intptr_t;

// limits.h
#define LLONG_MAX __LONG_LONG_MAX__
#define LLONG_MIN (-LLONG_MAX - 1LL)

// assert.h
void __assert(const char* filename, const char* line, const char* expression);

#define STRING(x) #x
#define STRINGLINE(x) STRING(x)
#define LINE STRINGLINE(__LINE__)
#define assert(e) ((e) ? (void)0 : __assert(__FILE__, LINE, #e))

// stdlib.h
int abs(int i);
long long llabs(long long n);
long long strtoll(const char* nptr, char** endptr, int base);
void exit(int status);
void* malloc(size_t size);
void* realloc(void *ptr, size_t size);
void free(void* ptr);
//void* calloc(size_t count, size_t size);

// stdio.h
#define EOF -1
typedef struct FILE FILE;
#define stdin ((FILE*)0)
#define stdout ((FILE*)1)
#define stderr ((FILE*)2)
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE *stream);
size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fputs(const char* str, FILE* stream);
int putchar(int c);
void setbuf(FILE* stream, char* buffer);
//int fflush(FILE* stream);

// ctype.h
int isalpha(int c);
int isspace(int c);
int iscntrl(int c);
int isdigit(int c);
//int isalnum(int c);
//int isblank(int c);
//int ispunct(int c);
//int isprint(int c);

// string.h
void* memcpy(void *dest, const void *src, size_t n);
void* memset(void* s, int c, size_t n);
size_t strlen(const char* s);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
int strncmp(const char* s1, const char* s2, size_t n);
const char* strchr(const char* s, int c);
size_t strspn(const char* s, const char* accept);
size_t strcspn(const char* s, const char* accept);
//int strcmp(const char* s1, const char* s2);
//const char* strpbrk(const char* s, const char* accept);

#endif
