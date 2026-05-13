
#include "token.h"

extern void rl_setprompt(char* prompt);
extern char *rl_saveprompt();
extern void rl_restoreprompt();
extern void initialize_readline();
extern int rl_readin_file(FILE *fp);
extern int rlgetc(LEXER *lp);
extern int rl_ungetc(LEXER *lp, int c);
extern int rl_ungets(LEXER *lp, char *s);

