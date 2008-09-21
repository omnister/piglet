
typedef enum{FALSE, TRUE} BOOLEAN;

BOOLEAN assign(char **p, char *s);     		/* initialize name or value */
struct varslot *EVset(char *name, char *val); 	/* add name & value to the env */
struct varslot *Macroset(char *name, char *val);/* add name & value to list of macros */
char *EVget(char *name);                      	/* get value of variable */
char *Macroget(char *name);                     /* get value of macro definition */
BOOLEAN EVinit();			      	/* initialize symbol table from env */
BOOLEAN EVupdate();                           	/* build env from symbol table */
void EVprint();                               	/* print the environment */
BOOLEAN EVexport(char *name);		      	/* set variable to be exported */


