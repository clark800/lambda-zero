#include <stdbool.h>
#include <string.h>
#include "rstring.h"

String EMPTY = {"\0", 0};

String newString(const char* start, unsigned int length) {
    return (String){start, length};
}

bool isThisString(String a, const char* b) {
    return a.length == strlen(b) && strncmp(a.start, b, a.length) == 0;
}

bool isSameString(String a, String b) {
    return a.length == b.length && strncmp(a.start, b.start, a.length) == 0;
}
