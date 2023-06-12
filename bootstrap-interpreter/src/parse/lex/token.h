typedef enum {END, SPACE, VSPACE, NEWLINE, SYMBOLIC, NUMERIC, CHARACTER,
    STRING, COMMENT} TokenType;

typedef struct {
    Lexeme lexeme;
    TokenType type;
} Token;
