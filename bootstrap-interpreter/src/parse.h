typedef struct {
    Hold* root;
    Node* entry;
    Array* globals;
} Program;

extern bool TRACE_PARSING;
Program parse(const char* input);
bool isIO(Program program);
void deleteProgram(Program program);
