#include <stdbool.h>
#include <string.h>
#include "tag.h"

String EMPTY = {"\0", 0};

String newString(const char* start, unsigned int length) {
    return (String){start, length};
}

Location newLocation(unsigned int line, unsigned int column) {
    return (Location){line, column};
}

Tag newTag(String lexeme, Location location) {
    return (Tag){lexeme, location};
}

Tag renameTag(Tag tag, const char* name) {
    return newTag(newString(name, (unsigned int)strlen(name)), tag.location);
}

bool isThisString(String a, const char* b) {
    return a.length == strlen(b) && strncmp(a.start, b, a.length) == 0;
}

bool isSameString(String a, String b) {
    return a.length == b.length && strncmp(a.start, b.start, a.length) == 0;
}
