
extern int add_inst(LEXER *lp, char *inst_name);
extern int is_comp(char c);
extern int default_layer();
extern void parse(LEXER *lp);
extern int add_arc(LEXER *lp, int *layer);
extern int add_circ(LEXER *lp, int *layer);
extern int add_line(LEXER *lp, int *layer);
extern int add_note(LEXER *lp, int *layer);
extern int add_oval(LEXER *lp, int *layer);
extern int add_poly(LEXER *lp, int *layer);
extern int add_rect(LEXER *lp, int *layer);
extern int add_text(LEXER *lp, int *layer);
extern int lookup_command(char *name);

