#include <stdio.h>

static const unsigned int MAX_LEXEME_LENGTH = 0xff;
static const unsigned int MAX_LINE = 0xffff;
static const unsigned int MAX_COLUMN = 0xffff;

typedef struct {
    unsigned short file, line, column;
} Location;

typedef struct {
    Location location;
    unsigned short length;
    const char* start;
} Lexeme;

extern const Lexeme EMPTY;

Lexeme newLexeme(const char* start, unsigned short length, Location location);
Lexeme newLiteralLexeme(const char* start, Location location);
unsigned short newFilename(const char* filename);
Location newLocation(unsigned short file,
    unsigned short line, unsigned short column);
bool isThisLexeme(Lexeme a, const char* b);
bool isSameLexeme(Lexeme a, Lexeme b);
void printLocation(Location location, FILE* stream);
