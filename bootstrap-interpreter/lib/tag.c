#include <stdbool.h>
#include <stdlib.h>  // exit
#include <string.h>
#include "util.h"
#include "tag.h"

const String EMPTY = {"", 0, '\0', 0};
const char* FILENAMES[2048] = {0};
const unsigned int MAX_FILENAMES = sizeof(FILENAMES) / sizeof(const char*);
unsigned short FILE_COUNT = 0;

String newString(const char* start, unsigned char length) {
    return (String){start, length, '\0', 0};
}

String toString(const char* start) {
    return newString(start, (unsigned char)strlen(start));
}

unsigned short newFilename(const char* filename) {
    if (FILE_COUNT >= MAX_FILENAMES - 1)
        return 0;
    FILENAMES[++FILE_COUNT] = filename;
    return FILE_COUNT;
}

Location newLocation(unsigned short file,
        unsigned int line, unsigned short column) {
    return (Location){.file=file, .line=line, .column=column};
}

Tag newTag(String lexeme, Location location, char fixity) {
    lexeme.fixity = fixity;
    return (Tag){lexeme, location};
}

Tag newLiteralTag(const char* name, Location location, char fixity) {
    String lexeme = newString(name, (unsigned char)strlen(name));
    return newTag(lexeme, location, fixity);
}

Tag addPrefix(Tag tag, char prefix) {
    return tag.lexeme.prefix = prefix, tag;
}

char getTagFixity(Tag tag) {
    return tag.lexeme.fixity;
}

bool isThisString(String a, const char* b) {
    // strncmp(NULL, NULL, 0) is undefined behavior, so we check for 0 length
    return a.prefix == '\0' && a.length == strlen(b) &&
        (a.length == 0 || strncmp(a.start, b, a.length) == 0);
}

bool isSameString(String a, String b) {
    // strncmp(NULL, NULL, 0) is undefined behavior, so we check for 0 length
    return a.length == b.length && a.prefix == b.prefix &&
        (a.length == 0 || strncmp(a.start, b.start, a.length) == 0);
}

void printTag(Tag tag, FILE* stream) {
    String lexeme = tag.lexeme;
    if (lexeme.prefix == '\0' && lexeme.length > 0 && lexeme.start[0] == '\n') {
        fputs("(end of line)", stream);
    } else {
        if (lexeme.prefix != '\0')
            fputc(lexeme.prefix, stream);
        fwrite(lexeme.start, sizeof(char), lexeme.length, stream);
    }
}

static void printLine(const char* line, FILE* stream) {
    size_t length = 0;
    for (; line[length] != '\0' && line[length] != '\n'; ++length);
    fwrite(line, sizeof(char), length, stream);
}

static void printLocation(Location location, FILE* stream) {
    if (location.file != 0) {
        printLine(FILENAMES[location.file], stream);
        fputs(" ", stream);
    }
    fputs("line ", stream);
    fputll((long long)location.line, stream);
    fputs(" column ", stream);
    fputll((long long)location.column, stream);
}

void printTagWithLocation(Tag tag, FILE* stream) {
    fputs("'", stream);
    printTag(tag, stream);
    fputs("' at ", stream);
    printLocation(tag.location, stream);
}

void syntaxError(const char* message, Tag tag) {
    fputs("Syntax error: ", stderr);
    fputs(message, stderr);
    fputs(" ", stderr);
    printTagWithLocation(tag, stderr);
    fputs("\n", stderr);
    exit(1);
}

void syntaxErrorIf(bool condition, const char* message, Tag tag) {
    if (condition)
        syntaxError(message, tag);
}
