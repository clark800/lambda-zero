#include <stdlib.h>     // llabs
#include <stdio.h>
#include "util.h"

void* error(const char* message) {
    fputs(message, stderr);
    exit(1);
    return NULL;
}

void* smalloc(size_t size) {
    void* p = malloc(size);
    return p == NULL && size > 0 ? error("\nError: out of memory\n") : p;
}

static char getDigitCharacter(long long n) {
    return n < 0 || n >= 16 ? '#' : (char)(n < 10 ? '0' + n : 'a' + (n - 10));
}

static void swap(char* a, char* b) {
    char c = *a;
    *a = *b;
    *b = c;
}

static char* reverse(char* string, unsigned int length) {
    for (unsigned int i = 0; i < length / 2; ++i)
       swap(&string[i], &string[length - i - 1]);
    return string;
}

char* lltoa(long long n, char* buffer, int radix) {
    if (radix < 2 || radix > 16)
        return NULL;
    unsigned int i = 0;
    if (n == 0) {
        buffer[i++] = '0';
        buffer[i++] = '\0';
        return buffer;
    }
    int negative = n < 0;
    if (negative)
        radix = -radix;
    for (; n != 0; n = n / abs(radix))
        buffer[i++] = getDigitCharacter(llabs(n % radix));
    if (negative)
        buffer[i++] = '-';
    buffer[i] = '\0';
    return reverse(buffer, i);
}

void fputll(long long n, FILE* stream) {
    char buffer[3 * sizeof(long long)];
    fputs(lltoa(n, buffer, 10), stream);
}
