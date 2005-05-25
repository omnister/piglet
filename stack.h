
typedef struct stack {
    char *saved;
    struct stack *next;
} STACK;

/* public functions */
void stack_push(STACK **stack, char *pointer);
char *stack_pop(STACK **stack);

