void printError(const char* type, const char* message, Node* node);
void printTagLine(Node* node, const char* quote);

void syntaxError(const char* message, Node* token);
void syntaxErrorIf(bool condition, const char* message, Node* token);
void memoryError(const char* label, long long bytes);
void usageError(const char* name);
void readError(const char* filename);
