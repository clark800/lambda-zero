void printError(const char* type, const char* message, Tag tag);
void printTagLine(Tag tag, const char* quote);

void tokenErrorIf(bool condition, const char* message, Tag tag);
void syntaxError(const char* message, Node* node);
void syntaxErrorIf(bool condition, const char* message, Node* node);
void memoryError(const char* label, long long bytes);
void usageError(const char* name);
void readError(const char* filename);
