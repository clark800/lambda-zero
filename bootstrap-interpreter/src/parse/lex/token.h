typedef enum {END, SPACE, VSPACE, NEWLINE, SYMBOLIC, NUMERIC, CHARACTER,
    STRING, COMMENT, INVALID} TokenType;

typedef struct {
    Lexeme lexeme;
    TokenType type;
} Token;
