#define BUFSIZE 100

typedef enum {
    IDENT, 	/* identifier */
    CMD,	/* command */
    QUOTE, 	/* quoted string */
    NUMBER, 	/* number */
    OPT,	/* option */
    EOL,	/* newline or carriage return */
    EOC,	/* end of command */
    COMMA,	/* comma */
    END,	/* end of file */
    UNKNOWN	/* unknown token */
} TOKEN;


extern TOKEN token_get(char *word);
extern TOKEN token_look();
extern int   token_unget(TOKEN token, char *word); 
extern int   token_flush();

