#include <stdbool.h>
#include <stdlib.h>  // exit
#include <string.h>
#include "util.h"
#include "tag.h"

const Lexeme EMPTY = {.location={0}, .length=0, .start=""};
const char* FILENAMES[2048] = {0};
const unsigned int MAX_FILENAMES = sizeof(FILENAMES) / sizeof(const char*);
unsigned short FILE_COUNT = 0;

Lexeme newLexeme(const char* start, unsigned short length, Location location) {
    return (Lexeme){.location=location, .length=length, .start=start};
}

Lexeme newLiteralLexeme(const char* start) {
    unsigned short length = (unsigned short)strlen(start);
    return (Lexeme){.location={0}, .length=length, .start=start};
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

Tag newTag(Lexeme lexeme, char fixity) {
    return (Tag){.fixity=fixity, .lexeme=lexeme};
}

Tag newLiteralTag(const char* name, Location location, char fixity) {
    Lexeme lexeme = newLexeme(name, (unsigned short)strlen(name), location);
    return newTag(lexeme, fixity);
}

Tag addPrefix(Tag tag, char prefix) {
    return tag.prefix = prefix, tag;
}

char getTagFixity(Tag tag) {
    return tag.fixity;
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

bool isThisTag(Tag a, const char* b) {
    return a.prefix == '\0' && isThisLexeme(a.lexeme, b);
}

bool isSameTag(Tag a, Tag b) {
    return a.prefix == b.prefix && isSameLexeme(a.lexeme, b.lexeme);
}

void printTag(Tag tag, FILE* stream) {
    Lexeme lexeme = tag.lexeme;
    if (tag.prefix == '\0' && lexeme.length > 0 && lexeme.start[0] == '\n') {
        fputs("(end of line)", stream);
    } else {
        if (tag.prefix != '\0')
            fputc(tag.prefix, stream);
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
    printLocation(tag.lexeme.location, stream);
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
