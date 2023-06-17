typedef struct {
    Hold* root;
    Term* entry;
    Array* globals;
} Program;

Program parse(const char* input);
void deleteProgram(Program program);
