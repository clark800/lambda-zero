#include <stdbool.h>
#include <string.h>
#include "util.h"
#include "tag.h"

String newString(const char* start, unsigned int length) {
    return (String){start, length, '\0', 0};
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

Tag addPrefix(Tag tag, char prefix) {
    return tag.lexeme.prefix = prefix, tag;
}

Tag setTagFixity(Tag tag, char fixity) {
    return tag.lexeme.fixity = fixity, tag;
}

char getTagFixity(Tag tag) {
    return tag.lexeme.fixity;
}

bool isThisString(String a, const char* b) {
    return a.prefix == '\0' && a.length == strlen(b) &&
        strncmp(a.start, b, a.length) == 0;
}

bool isSameString(String a, String b) {
    return a.length == b.length && a.prefix == b.prefix &&
        strncmp(a.start, b.start, a.length) == 0;
}

void printString(String string, FILE* stream) {
    if (string.prefix != '\0')
        fputc(string.prefix, stream);
    fwrite(string.start, sizeof(char), string.length, stream);
}

void printLine(const char* line, FILE* stream) {
    size_t length = 0;
    for (; line[length] != '\0' && line[length] != '\n'; ++length);
    fwrite(line, sizeof(char), length, stream);
}

void printTag(Tag tag, const char* quote, FILE* stream) {
    if (tag.lexeme.length > 0 && tag.lexeme.start[0] == '\n') {
        fputs("(end of line)", stream);
    } else {
        fputs(quote, stream);
        printString(tag.lexeme, stream);
        fputs(quote, stream);
    }
    fputs(" at ", stream);
    if (tag.location.file != NULL) {
        printLine(tag.location.file, stream);
        fputs(" " , stream);
    }
    fputs("line ", stream);
    fputll((long long)tag.location.line, stream);
    fputs(" column ", stream);
    fputll((long long)tag.location.column, stream);
}
