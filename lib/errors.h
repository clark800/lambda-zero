#ifndef ERRORS_H
#define ERRORS_H

#include <stdlib.h>
#include <stdio.h>

typedef const char* strings[];

static inline void errorArray(int count, const char* strs[]) {
    for (int i = 0; i < count; i++)
        fputs(strs[i], stderr);
}

static inline void error(const char* type, const char* message) {
    errorArray(3, (strings){type, " error: ", message});
    exit(1);
}

#endif
