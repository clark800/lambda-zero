#include <stdbool.h>
#include <string.h>
#include "util.h"   // fputll
#include "lexeme.h"

const Lexeme EMPTY = {.location={0}, .length=0, .start=""};
const char* FILENAMES[2048] = {0};
const unsigned int MAX_FILENAMES = sizeof(FILENAMES) / sizeof(const char*);
unsigned short FILE_COUNT = 0;

Lexeme newLexeme(const char* start, unsigned short length, Location location) {
    return (Lexeme){.location=location, .length=length, .start=start};
}

Lexeme newLiteralLexeme(const char* start, Location location) {
    return newLexeme(start, (unsigned short)strlen(start), location);
}

unsigned short newFilename(const char* filename) {
    if (FILE_COUNT >= MAX_FILENAMES - 1)
        return 0;
    FILENAMES[++FILE_COUNT] = filename;
    return FILE_COUNT;
}

Location newLocation(unsigned short file,
        unsigned short line, unsigned short column) {
    return (Location){.file=file, .line=line, .column=column};
}

bool isThisLexeme(Lexeme a, const char* b) {
    // strncmp(NULL, NULL, 0) is undefined behavior, so we check for 0 length
    return a.length == strlen(b) &&
        (a.length == 0 || strncmp(a.start, b, a.length) == 0);
}

bool isSameLexeme(Lexeme a, Lexeme b) {
    // strncmp(NULL, NULL, 0) is undefined behavior, so we check for 0 length
    return a.length == b.length &&
        (a.length == 0 || strncmp(a.start, b.start, a.length) == 0);
}

static void printLine(const char* line, FILE* stream) {
    size_t length = 0;
    for (; line[length] != '\0' && line[length] != '\n'; ++length);
    fwrite(line, sizeof(char), length, stream);
}

void printLocation(Location location, FILE* stream) {
    if (location.file != 0) {
        printLine(FILENAMES[location.file], stream);
        fputs(" ", stream);
    }
    fputs("line ", stream);
    fputll((long long)location.line, stream);
    fputs(" column ", stream);
    fputll((long long)location.column, stream);
}
