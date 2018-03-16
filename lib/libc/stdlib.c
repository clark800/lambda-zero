#include "libc.h"

void exit(int status) {
    _exit(status);
}

int abs(int i) {
    return i < 0 ? -i : i;
}

long long llabs(long long n) {
    return n < 0 ? -n : n;
}

long long strtoll(const char* nptr, const char** endptr, int base) {
    assert(base == 10);
    bool negative = nptr[0] == '-';
    int sign = negative ? -1 : 1;
    long long n = 0;
    long long limit = negative ? LLONG_MIN : LLONG_MAX;
    long long cutoff = limit / base;
    int cutoffDigit = (int)(limit % (negative ? -base : base));

    while (isspace(nptr[0]))
        nptr += 1;
    if (negative || nptr[0] == '+')
        nptr += 1;
    for (; isdigit(nptr[0]); nptr += 1) {
        int digit = sign * (nptr[0] - '0');
        if (llabs(n) > llabs(cutoff) ||
            (n == cutoff && abs(digit) > abs(cutoffDigit))) {
            errno = ERANGE;
            n = negative ? LLONG_MIN : LLONG_MAX;
            break;
        }
        n = n * 10 + digit;
    }
    if (endptr != NULL)
        *endptr = nptr;
    return n;
}

#define HEADER(block) (((Header*)block) - 1)
#define BLOCK(header) (((Header*)header) + 1)

typedef struct Header Header;
struct Header {
    size_t size;  /* malloc size, not including header */
};

static size_t roundUp(size_t size, size_t multiple) {
    size_t mod = size % multiple;
    return mod == 0 ? size : size + multiple - mod;
}

static size_t mmapSize(size_t size) {
    return roundUp(size + sizeof(Header), PAGE_SIZE);
}

static size_t usableSize(void* ptr) {
    return mmapSize(HEADER(ptr)->size) - sizeof(Header);
}

void* malloc(size_t size) {
    void* result;
    if (size <= 0)
        return NULL;
    result = mmap(NULL, mmapSize(size), PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if ((unsigned long)result >= (unsigned long)(-125))
        return NULL;    /* out of memory */
    ((Header*)result)->size = size;
    return BLOCK(result);
}

void* realloc(void *ptr, size_t size) {
    void* result;
    if (ptr == NULL)
        return malloc(size);
    if (size <= usableSize(ptr)) {
        HEADER(ptr)->size = size;
        return ptr;
    }
    result = malloc(size);
    if (result != NULL) {
        memcpy(result, ptr, HEADER(ptr)->size);
        free(ptr);
    }
    return result;
}

void free(void* ptr) {
    assert(ptr != NULL);
    munmap(HEADER(ptr), mmapSize(HEADER(ptr)->size));
}

/*void* calloc(size_t count, size_t size) {
    size_t totalSize = count * size;
    void* result = malloc(totalSize);
    return result == NULL ? NULL : memset(result, 0, totalSize);
}*/
