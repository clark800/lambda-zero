#include "lexeme.h"
#include <stdio.h>

typedef struct {
    char fixity;
    char prefix;
    Lexeme lexeme;
} Tag;

Tag newTag(Lexeme lexeme, char fixity);
Tag newLiteralTag(const char* name, Location location, char fixity);
Tag addPrefix(Tag tag, char prefix);
Lexeme getLexeme(Tag tag);
char getTagFixity(Tag tag);
bool isThisTag(Tag a, const char* b);
bool isSameTag(Tag a, Tag b);
void printTag(Tag tag, FILE* stream);
void printTagWithLocation(Tag tag, FILE* stream);
void syntaxError(const char* message, Tag tag);
void syntaxErrorIf(bool condition, const char* message, Tag tag);
