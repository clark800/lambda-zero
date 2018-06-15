extern bool TEST;

void printTokenError(const char* type, const char* message, Node* token);
void printTokenAndLocationLine(Node* token, const char* quote);

void lexerErrorIf(bool condition, const char* lexeme, const char* message);
void syntaxError(const char* message, Node* token);
void syntaxErrorIf(bool condition, const char* message, Node* token);
void printMemoryError(const char* label, long long bytes);
void usageError(const char* name);
void readError(const char* filename);
