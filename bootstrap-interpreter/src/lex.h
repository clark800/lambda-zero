typedef enum {END, SYMBOLIC, NUMERIC, CHARACTER, STRING, INVALID}
TokenType;

typedef struct {
    Tag tag;
    TokenType type;
} Token;

bool isSpaceCharacter(char c);
Token lex(const char* start);
const char* skip(Token token);
Token newStartToken(void);
