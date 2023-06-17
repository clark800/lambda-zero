typedef struct {
    Hold* root;
    Node* entry;
    Array* globals;
} Program;

Program parse(const char* input);
void deleteProgram(Program program);
