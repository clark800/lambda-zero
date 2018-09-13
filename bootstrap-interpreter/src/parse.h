typedef struct {
    Hold* root;
    Node* entry;
    Array* globals;
} Program;

extern bool TRACE_PARSING;
Program parse(const char* input);
void deleteProgram(Program program);
