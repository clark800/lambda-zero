#include <stdio.h>

typedef struct {
    const char* start;
    unsigned int length;
    char prefix;
    char fixity;  // should be in Tag, but putting it here reduces sizeof(Tag)
} String;

typedef struct {
    const char* file;
    unsigned int line, column;
} Location;

typedef struct {
    String lexeme;
    Location location;
} Tag;

String newString(const char* start, unsigned int length);
String toString(const char* start);
Location newLocation(const char* file, unsigned int line, unsigned int column);
Tag newTag(String lexeme, Location location);
Tag renameTag(Tag tag, const char* name);
Tag addPrefix(Tag tag, char prefix);
Tag setTagFixity(Tag tag, char fixity);
char getTagFixity(Tag tag);
bool isThisString(String a, const char* b);
bool isSameString(String a, String b);
void printString(String string, FILE* stream);
void printTag(Tag tag, const char* quote, FILE* stream);
