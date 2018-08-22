void printError(const char* type, const char* message, Tag tag);
void printTagLine(Tag tag, const char* quote);

void syntaxError(const char* message, Node* token);
void syntaxErrorIf(bool condition, const char* message, Node* token);
void memoryError(const char* label, long long bytes);
void usageError(const char* name);
void readError(const char* filename);
