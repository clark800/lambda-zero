typedef struct {
    Hold* root;
    Node* entry;
    Array* globals;
} Program;

extern int DEBUG;
Program parse(const char* input);
void deleteProgram(Program program);
