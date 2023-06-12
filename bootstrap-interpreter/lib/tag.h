#include <stdio.h>

static const unsigned int MAX_LEXEME_LENGTH = 0xff;
static const unsigned int MAX_LINE = 0xffff;
static const unsigned int MAX_COLUMN = 0xffff;

typedef struct {
    const char* start;
    unsigned char length;
    char prefix;
    char fixity;  // should be in Tag, but putting it here reduces sizeof(Tag)
} String;

typedef struct {
    unsigned short file, line, column;
} Location;

typedef struct {
    String lexeme;
    Location location;
} Tag;

extern const String EMPTY;

String newString(const char* start, unsigned char length);
String toString(const char* start);
unsigned short newFilename(const char* filename);
Location newLocation(unsigned short file,
    unsigned int line, unsigned short column);
Tag newTag(String lexeme, Location location, char fixity);
Tag newLiteralTag(const char* name, Location location, char fixity);
Tag addPrefix(Tag tag, char prefix);
char getTagFixity(Tag tag);
bool isThisString(String a, const char* b);
bool isSameString(String a, String b);
void printTag(Tag tag, FILE* stream);
void printTagWithLocation(Tag tag, FILE* stream);
void syntaxError(const char* message, Tag tag);
void syntaxErrorIf(bool condition, const char* message, Tag tag);
