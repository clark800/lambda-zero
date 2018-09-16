typedef enum {END, IDENTIFIER, PUNCTUATION, NUMBER, CHARACTER, STRING}
TokenType;

typedef struct {
    Tag tag;
    TokenType type;
} Token;

Token lex(const char* start);
const char* skip(Token token);
Token newStartToken(void);
