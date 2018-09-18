typedef enum {END, SYMBOLIC, NUMERIC, CHARACTER, STRING, COMMENT, INVALID}
    TokenType;

typedef struct {
    Tag tag;
    TokenType type;
} Token;

bool isSpaceCharacter(char c);
Token lex(Token token);
Token newStartToken(const char* start);
