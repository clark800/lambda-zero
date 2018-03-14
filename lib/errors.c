#include <stdlib.h>
#include <stdio.h>
#include "errors.h"

int VERBOSITY = 0;

void errorArray(int count, const char* strs[]) {
    for (int i = 0; i < count; i++)
        fputs(strs[i], stderr);
}

void error(const char* type, const char* message) {
    errorArray(3, (strings){type, " error: ", message});
    exit(1);
}
