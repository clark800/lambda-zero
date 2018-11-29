typedef enum {END, SPACE, VSPACE, NEWLINE, SYMBOLIC, NUMERIC, CHARACTER,
    STRING, COMMENT, INVALID} TokenType;

typedef struct {
    Tag tag;
    TokenType type;
} Token;
