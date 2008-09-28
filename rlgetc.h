
extern void rl_setprompt(char* prompt);
extern char *rl_saveprompt();
extern void rl_restoreprompt();
extern void initialize_readline();
extern int rl_readin_file(FILE *fp);
extern int rlgetc();
extern int rl_ungetc();
extern int rl_ungets();

