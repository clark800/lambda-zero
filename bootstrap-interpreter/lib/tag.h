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

typedef struct {
    char fixity;
    char prefix;
    Lexeme lexeme;
} Tag;

extern const Lexeme EMPTY;

Lexeme newLexeme(const char* start, unsigned short length, Location location);
Lexeme newLiteralLexeme(const char* start);
unsigned short newFilename(const char* filename);
Location newLocation(unsigned short file,
    unsigned short line, unsigned short column);
Tag newTag(Lexeme lexeme, char fixity);
Tag newLiteralTag(const char* name, Location location, char fixity);
Tag addPrefix(Tag tag, char prefix);
char getTagFixity(Tag tag);
bool isThisLexeme(Lexeme a, const char* b);
bool isSameLexeme(Lexeme a, Lexeme b);
bool isThisTag(Tag a, const char* b);
bool isSameTag(Tag a, Tag b);
void printTag(Tag tag, FILE* stream);
void printTagWithLocation(Tag tag, FILE* stream);
void syntaxError(const char* message, Tag tag);
void syntaxErrorIf(bool condition, const char* message, Tag tag);
