#include <stdio.h>

static const unsigned int MAX_LEXEME_LENGTH = 0xff;
static const unsigned int MAX_COLUMN = 0xffff;
static const unsigned int MAX_LINE = 0xffffffff;

typedef struct {
    const char* start;
    unsigned char length;
    char prefix;
    char fixity;  // should be in Tag, but putting it here reduces sizeof(Tag)
} String;

typedef struct {
    unsigned int line;
    unsigned short column, file;
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
Tag newTag(String lexeme, Location location);
Tag renameTag(Tag tag, const char* name);
Tag addPrefix(Tag tag, char prefix);
Tag setTagFixity(Tag tag, char fixity);
char getTagFixity(Tag tag);
bool isThisString(String a, const char* b);
bool isSameString(String a, String b);
void printString(String string, FILE* stream);
void printTag(Tag tag, const char* quote, FILE* stream);
void throwError(const char* message, Tag tag);
