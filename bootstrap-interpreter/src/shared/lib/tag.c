#include <stdbool.h>
#include <string.h>
#include "util.h"
#include "tag.h"

String newString(const char* start, unsigned int length) {
    return (String){start, length};
}

String toString(const char* start) {
    return newString(start, (unsigned int)strlen(start));
}

Location newLocation(const char* file, unsigned int line, unsigned int column) {
    return (Location){file, line, column};
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

bool contains(String a, char c) {
    return memchr(a.start, c, a.length) != NULL;
}

void printString(String string, FILE* stream) {
    fwrite(string.start, sizeof(char), string.length, stream);
}

void printLine(const char* line, FILE* stream) {
    size_t length = 0;
    for (; line[length] != '\0' && line[length] != '\n'; ++length);
    fwrite(line, sizeof(char), length, stream);
}

void printTag(Tag tag, const char* quote, FILE* stream) {
    fputs(quote, stream);
    printString(tag.lexeme, stream);
    fputs(quote, stream);
    fputs(" at ", stream);
    if (tag.location.file != NULL) {
        printLine(tag.location.file, stream);
        fputs(" " , stream);
    }
    fputs("line ", stream);
    fputll(tag.location.line, stream);
    fputs(" column ", stream);
    fputll(tag.location.column, stream);
}
