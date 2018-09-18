typedef enum {
    END, SPACE, SYMBOLIC, NUMERIC, CHARACTER, STRING, COMMENT, INVALID
} TokenType;

typedef struct {
    Tag tag;
    TokenType type;
} Token;

Token lex(Token token);
Token newStartToken(const char* start);
