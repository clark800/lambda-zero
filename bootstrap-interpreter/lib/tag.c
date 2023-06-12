#include <stdbool.h>
#include <stdlib.h>  // exit
#include <string.h>
#include "tag.h"

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
