#define BUFSIZE 100

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
    UNKNOWN	/* unknown token */
} TOKEN;

/* 
	tokens required for a simple shell:

	T_WORD, <alphanum>*	IDENT
	T_SEMI, ;		EOC
	T_NL,   '\n'		EOL
	T_EOF,  <EOF>		END
	T_BAR,  |
	T_AMP,  &
	T_GT,   >
	T_GTGT, >>
	T_LT,   <
	T_VAR,  $<WORD>
	T_ERROR

	need to add T_VAR to piglet
	and take "|&><" out of IDENT regexp
	add in T_BAR, T_AMP, T_GT, T_GTGT, T_LT

*/


#define MAIN 0
#define PRO 1
#define SEA 2
#define MAC 3
#define EDI 4


typedef struct savetok {
    char *word;
    TOKEN tok;
} SAVETOK;

typedef struct lexer {
    char *name;			/* name of stream */
    FILE *token_stream;		/* file pointer to stream */
    char word[1024];		/* token string value */
    SAVETOK tokbuf[BUFSIZE];
    int bufp;			/* next free position in buf */
    int mode;			/* MAIN, EDI, PRO, ... etc */
    int line;			/* line number */
    int parse;			/* parse mode: 0=normal, 1=raw */
} LEXER;

extern TOKEN  token_get(LEXER *lp, char ** word);
extern TOKEN  token_look();
extern int    token_unget(LEXER *lp, TOKEN token, char *word); 
extern LEXER *token_stream_open( FILE *fp, char *name );
extern int    token_stream_close( LEXER *lp );
extern int    token_flush(LEXER *lp);
extern int    token_flush_EOL(LEXER *lp);
extern int    token_err(char *module, LEXER *lp, char *expected, TOKEN token);
extern char  *tok2str(TOKEN token);
extern void   token_set_mode(LEXER *lp, int mode);
extern int    getnum(LEXER *lp, char *cmd, double *px, double *py);
