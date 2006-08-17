
typedef struct stack {
    char *saved;
    struct stack *next;
} STACK;

/* public functions */
void stack_push(STACK **stack, char *pointer);
void stack_free(STACK **stack);
char *stack_pop(STACK **stack);
char *stack_walk(STACK **stack);	/* each call returns a new entry, modifies stack */ 
