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


typedef struct savetok {
    char *word;
    TOKEN tok;
} SAVETOK;

typedef struct lexer {
    int bufp;		/* next free position in buf */
    FILE *token_stream;
    SAVETOK tokbuf[BUFSIZE];
} LEXER;

extern TOKEN  token_get(LEXER *lp, char *word);
extern TOKEN  token_look();
extern int    token_unget(LEXER *lp, TOKEN token, char *word); 
extern LEXER *token_stream_open( FILE *fp );
extern int    token_stream_close( LEXER *lp );
extern int    token_flush(LEXER *lp);
extern void   token_set_stream( FILE *fp );
extern FILE  *token_get_stream();

