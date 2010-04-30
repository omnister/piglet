#define BUFSIZE 1024

typedef enum {
    IDENT, 	/* identifier */			
    CMD,	/* command (an ident found in commands[]) */
    QUOTE, 	/* quoted string */
    NUMBER, 	/* number */
    OPT,	/* option */
    EOL,	/* newline or carriage return */
    EOC,	/* end of command */
    BACK,	/* backup "^" */
    COMMA,	/* comma */
    END,	/* end of file */
    RAW,	/* individual character mode */
    COMMENT,	/* $$ comment */
    UNKNOWN	/* unknown token */
} TOKEN;

typedef struct savetok {
    char *word;
    TOKEN tok;
} SAVETOK;

/* in working on macro function, I'm not sure whether it makes sense */
/* to pushback at token or character level, so we have two different */
/* pushback buffers for now.  I'll clean this up later after the */
/* architecture settles  RCW: 9/17/08 */

typedef struct lexer {
    char *name;			/* name of stream */
    FILE *token_stream;		/* file pointer to stream */
    char word[1024];		/* token string value */
    SAVETOK tokbuf[BUFSIZE];	/* can pushback up to BUFSIZE tokens */
    int bufp;			/* next free position in tokbuf */
    char pushback[BUFSIZE];	/* can pushback up to BUFSIZE characters */
    int pbufp;			/* next free position in pushback */
    int mode;			/* MAIN, EDI, PRO, ... etc */
    int line;			/* line number */
    int parse;			/* parse mode: 0=normal, 1=raw */
} LEXER;

extern int add_inst(LEXER *lp, char *inst_name);
extern int is_comp(char c);
extern int default_layer();
extern void parse(LEXER *lp);
extern int add_arc(LEXER *lp, int *layer);
extern int add_circ(LEXER *lp, int *layer);
extern int add_line(LEXER *lp, int *layer);
extern void add_note(LEXER *lp, int *layer);
extern int add_oval(LEXER *lp, int *layer);
extern int add_poly(LEXER *lp, int *layer);
extern int add_rect(LEXER *lp, int *layer);
extern void add_text(LEXER *lp, int *layer);
extern int lookup_command(char *name);

#define MAIN 0
#define PRO 1
#define SEA 2
#define MAC 3
#define EDI 4

extern TOKEN  token_get(LEXER *lp, char ** word);
extern TOKEN  token_look(LEXER *lp, char **word);
extern int    token_unget(LEXER *lp, TOKEN token, char *word); 
extern LEXER *token_stream_open( FILE *fp, char *name );
extern int    token_stream_close( LEXER *lp );
extern int    token_flush(LEXER *lp);
extern int    token_flush_EOL(LEXER *lp);
extern int    token_err(char *module, LEXER *lp, char *expected, TOKEN token);
extern char  *tok2str(TOKEN token);
extern void   token_set_mode(LEXER *lp, int mode);
extern int    getnum(LEXER *lp, char *cmd, double *px, double *py);
