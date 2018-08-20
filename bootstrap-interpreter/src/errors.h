extern bool TEST;

void printError(const char* type, const char* message, Tag tag);
void printTagLine(Tag tag, const char* quote);

void lexerErrorIf(bool condition, Tag tag, const char* message);
void syntaxError(const char* message, Node* token);
void syntaxErrorIf(bool condition, const char* message, Node* token);
void printMemoryError(const char* label, long long bytes);
void usageError(const char* name);
void readError(const char* filename);
