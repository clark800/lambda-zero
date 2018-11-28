typedef struct Stack Stack;
typedef struct Iterator Iterator;

Stack* newStack(void);
void deleteStack(Stack* stack);
bool isEmpty(Stack* stack);
void push(Stack* stack, Node* node);
Hold* pop(Stack* stack);
Node* peek(Stack* stack, size_t i);
Iterator* iterate(Stack* stack);
Iterator* next(Iterator* iterator);
Node* cursor(Iterator* iterator);
bool end(Iterator* iterator);
