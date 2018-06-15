typedef struct {
    unsigned int line, column;
} Position;

bool isSpaceCharacter(char c);
bool isQuoteCharacter(char c);
bool isDelimiterCharacter(char c);
bool isOperandCharacter(char c);
bool isOperatorCharacter(char c);

const char* getFirstLexeme(const char* input);
const char* getNextLexeme(const char* lastLexeme);
unsigned int getLexemeLength(const char* lexeme);
bool isSameLexeme(const char* a, const char* b);
int getLexemeLocation(const char* lexeme);
const char* getLexemeByLocation(int location);
Position getPosition(unsigned int location);
