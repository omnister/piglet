
typedef struct stack {
    char *saved;
    int count;
    struct stack *next;
} STACK;

/* public functions */
void stack_push(STACK **stack, char *pointer);
void stack_free(STACK **stack);
char *stack_pop(STACK **stack);
char *stack_walk(STACK **stack);	/* returns next entry, modifies stack */ 
char *stack_top(STACK **stack);	        /* just return top of stack */ 
int  stack_depth(STACK **stack);	/* return depth of stack */ 
