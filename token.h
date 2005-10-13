#define BUFSIZE 100

typedef enum {
    IDENT, 	/* identifier */
    CMD,	/* command */
    QUOTE, 	/* quoted string */
    NUMBER, 	/* number */
    OPT,	/* option */
    EOL,	/* newline or carriage return */
    EOC,	/* end of command */
    BACK,	/* backup "^" */
    COMMA,	/* comma */
    END,	/* end of file */
    UNKNOWN	/* unknown token */
} TOKEN;


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
    SAVETOK tokbuf[BUFSIZE];
    int bufp;			/* next free position in buf */
    int mode;			/* MAIN, EDI, PRO, ... etc */
    int line;			/* line number */
} LEXER;

extern TOKEN  token_get(LEXER *lp, char *word);
extern TOKEN  token_look();
extern int    token_unget(LEXER *lp, TOKEN token, char *word); 
extern LEXER *token_stream_open( FILE *fp, char *name );
extern int    token_stream_close( LEXER *lp );
extern int    token_flush(LEXER *lp);
extern int    token_flush_EOL(LEXER *lp);
extern int    token_err(char *module, LEXER *lp, char *expected, TOKEN token);
extern char  *tok2str(TOKEN token);
