#include <stdio.h>
#include <string.h>             /* for strchr() */
#include <ctype.h>

#include "token.h"
#include "eprintf.h"
#include "db.h"
#include "xwin.h"
#include "rlgetc.h"
#include "lex.h" 	/* for lookup_command() */

static char promptbuf[128];


LEXER *token_stream_open(FILE *fp, char *name)  {
    LEXER *lp;
    lp = (LEXER *) emalloc(sizeof(struct lexer));
    lp->name = strsave(name);
    lp->bufp = 0;  /* no characters in pushback buf */
    lp->token_stream = fp;
    lp->mode = MAIN;
    lp->line = 1;	/* keep track of line number for stream */
    return (lp);
}

int token_err(char *modulename, LEXER *lp, char *expected, TOKEN token) 
{
    if (strcmp(lp->name, "STDIN") == 0) {
	printf("%s: %s, got %s\n", modulename, expected, tok2str(token));
    } else {
	printf("%s: in %s, line %d: %s, got %s\n", 
	    modulename, lp->name, lp->line, expected, tok2str(token));
    }
    return(0);
}

int token_stream_close(LEXER *lp)  {
    int retcode;
    retcode=fclose(lp->token_stream);
    free(lp);
    return(retcode);
}

/* lookahead to see the next token */
TOKEN token_look(LEXER *lp, char *word)
{
    TOKEN token;
    token=token_get(lp, word);
    token_unget(lp, token, word);
    return token;
}

/* stuff back a token */
int token_unget(LEXER *lp, TOKEN token, char *word) 
{
    int debug=0;
    if (debug) printf("ungetting %s\n", word);
    if (lp->bufp >= BUFSIZE) {
	eprintf("ungettoken: too many characters");
	return(-1);
    } else {
	lp->tokbuf[lp->bufp].word = (char *) estrdup(word);
	lp->tokbuf[lp->bufp++].tok = token;
	return(0);
    }
}

int token_flush_EOL(LEXER *lp) 
{
    char word[BUFSIZE];
    TOKEN token;
    while ((token=token_get(lp, word) != EOL)) {
	;
    }
    return(0);
}

TOKEN token_get(LEXER *lp, char *word) /* collect and classify token */
{
    enum {NEUTRAL,INQUOTE,INWORD,INOPT,INNUM} state = NEUTRAL;
    int c,d;
    char *w;
    int debug=0;

    if (lp->bufp > 0) {		/* characters in pushback buffer */
	strcpy(word, lp->tokbuf[--(lp->bufp)].word);
	free(lp->tokbuf[lp->bufp].word);
	lp->tokbuf[lp->bufp].word = (char *) NULL;
	return(lp->tokbuf[lp->bufp].tok);
    }

    w=word;
    while((c=rlgetc(lp->token_stream)) != EOF) {
	switch(state) {
	    case NEUTRAL:
		switch(c) {
		    case ' ':
		    case '\t':
			continue;
		    case '\n':
		    case '\r':
			*w++ = c;
			*w = '\0';
			if (debug) printf("returning EOL: %s \n", word);
			lp->line++;
			return(EOL);
		    case ',':
			*w++ = c;
			*w = '\0';
			if (debug) printf("returning COMMA: %s \n", word);
			return(COMMA);
		    case '"':
			state = INQUOTE;
			strcpy(promptbuf, rl_saveprompt());
			strcat(promptbuf, "(in quote)> ");
			rl_setprompt(promptbuf);
			continue;
		    case '^':
			*w++ = c;
			*w = '\0';
			if (debug) printf("returning BACK: %s \n", word);
			return(BACK);
			continue;
		    case ':':
			state = INOPT;
			*w++ = c;
			continue;
		    case ';':
			*w++ = c;
			*w = '\0';
			if (debug) printf("returning EOC: %s \n", word);
			return(EOC);
		    case '0':
		    case '1':
		    case '2':
		    case '3':
		    case '4':
		    case '5':
		    case '6':
		    case '7':
		    case '8':
		    case '9':
			*w++ = c;
			state = INNUM;
			continue;
		    case '+':
		    case '-':
		    case '#':
			*w++ = c;
			c = rlgetc(lp->token_stream);
			rl_ungetc(c,lp->token_stream);
			if (isdigit(c) || c=='.') {
			    state = INNUM;
			} else {
			    state = INOPT;
			}
			continue;
		    case '@':
			*w++ = c;
			c = rlgetc(lp->token_stream);
			rl_ungetc(c,lp->token_stream);
			state = INOPT;
			continue;
		    case '.':
			*w++ = c;
			c = rlgetc(lp->token_stream);
			if (isdigit(c)) {
			    rl_ungetc(c,lp->token_stream);
			    state = INNUM;
			} else {
			    rl_ungetc(c,lp->token_stream);
			    state = INOPT;
			}
			continue;	
		    default:
			state = INWORD;
			*w++ = c;
			continue;
		}
	    case INNUM:
		if (isdigit(c) || c=='.') {
		    *w++ = c;
		    continue;
		} else if (isalnum(c) || (c=='_') || (c=='.') ) {
		    *w++ = c;
		    state = INWORD;
		    continue;
		} else {
		    rl_ungetc(c,lp->token_stream);
		    *w = '\0';
		    if (debug) printf("returning NUMBER: %s \n", word);
		    return(NUMBER);
		}
	    case INOPT:
		if (isalnum(c)) {
		    *w++ = c;
		    continue;
		} else if (strchr("+-.",c) != NULL) {
		    *w++ = c;
		    continue;
		} else {
		    rl_ungetc(c,lp->token_stream);
		    *w = '\0';
		    if (debug) printf("returning OPT: %s \n", word);
		    return(OPT);
		}
	    case INQUOTE:
		switch(c) {
		    case '\\':
			/* escape quotes, but pass everything else along */
			if ((d = rlgetc(lp->token_stream)) == '"') {
			    *w++ = d;
			} else {
			    *w++ = c;
			    *w++ = d;
			}
			continue;
		    case '"':
			*w = '\0';
			if (debug) printf("returning QUOTE: %s \n", word);
			rl_restoreprompt();
			return(QUOTE);
		    default:
			*w++ = c;
			continue;
		}
	    case INWORD:
		if (!isalnum(c) && (c!='_') && (c!='.') ) {
		    rl_ungetc(c,lp->token_stream);
		    *w = '\0';
		    if (lookup_command(word)) {
		        if (debug) printf("lookup returns CMD: %s\n", word);
			return(CMD);
		    } else {
		        if (debug) printf("lookup returns IDENT: %s\n", word);
			return(IDENT);
		    }
		}
		*w++ = c;
		continue;
	    default:
		fprintf(stderr,"pig: error in lex loop\n");
		exit(1);
		break;
	} /* switch state */
    } /* while loop */
    return(EOF);
}

char *tok2str(TOKEN token)
{
    switch (token) {
	case IDENT: return("IDENT"); 	break;
	case CMD: return("CMD"); 	break;
	case QUOTE: return("QUOTE"); 	break;
	case NUMBER: return("NUMBER"); 	break;
	case OPT: return("OPT"); 	break;
	case EOL: return("EOL"); 	break;
	case EOC: return("EOC"); 	break;
	case COMMA: return("COMMA"); 	break;
	case BACK:  return("BACK");	break;
	case END: return("END"); 	break;
	default: return("UNKNOWN"); 	break;
    }
}
