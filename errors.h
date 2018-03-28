#ifndef ERRORS_H
#define ERRORS_H

extern int VERBOSITY;

void lexerErrorIf(bool condition, const char* lexeme, const char* message);
void syntaxError(const char* message, Node* token);
void syntaxErrorIf(bool condition, const char* message, Node* token);
void runtimeError(const char* message, Node* token);
void memoryError(const char* label, long long bytes);
void usageError(const char* name);
void readError(const char* filename);

#endif
