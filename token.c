#include <stdio.h>
#include <string.h>             /* for strchr() */

#include "token.h"

#include "db.h"
#include "xwin.h"

/* need token_look() */

int bufp = 0;		/* next free position in buf */

static FILE *token_stream;

struct savetok {
    char *word;
    TOKEN tok;
} tokbuf[BUFSIZE];

FILE *token_get_stream() {
    return (token_stream);
}

void token_set_stream(FILE *fp)  {
    extern FILE *token_stream;
    token_stream = fp;
}

/* lookahead to see the next token */
TOKEN token_look(word)
char *word;
{
    TOKEN token;
    token=token_get(word);
    token_unget(token, word);
    return token;
}

/* stuff back a token */
int token_unget(TOKEN token, char *word) 
{
    if (bufp >= BUFSIZE) {
	eprintf("ungettoken: too many characters");
	return(-1);
    } else {
	tokbuf[bufp].word = (char *) estrdup(word);
	tokbuf[bufp++].tok = token;
	return(0);
    }
}

int token_flush() 
{
    char word[BUFSIZE];
    TOKEN token;
    while (token=token_get(word) != EOL) {
	;
    }
    return(0);
}

TOKEN token_get(char *word) /* collect and classify token */
{
    enum {NEUTRAL,INQUOTE,INWORD,INOPT,INNUM} state = NEUTRAL;
    int c;
    char *w;
    extern FILE *token_stream;
    int debug=0;

    
    if (bufp > 0) {
	strcpy(word, tokbuf[--bufp].word);
	free(tokbuf[bufp].word);
	tokbuf[bufp].word = (char *) NULL;
	return(tokbuf[bufp].tok);
    }

    w=word;
    while((c=rlgetc(token_stream)) != EOF) {
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
			return(EOL);
		    case ',':
			*w++ = c;
			*w = '\0';
			if (debug) printf("returning COMMA: %s \n", word);
			return(COMMA);
		    case '"':
			state = INQUOTE;
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
		    case '+':
		    case '-':
			state = INNUM;
			*w++ = c;
			continue;
		    case '.':
			*w++ = c;
			c = rlgetc(token_stream);
			if (isdigit(c)) {
			    rl_ungetc(c,token_stream);
			    state = INNUM;
			} else {
			    rl_ungetc(c,token_stream);
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
		} else {
		    rl_ungetc(c,token_stream);
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
		    rl_ungetc(c,token_stream);
		    *w = '\0';
		    if (debug) printf("returning OPT: %s \n", word);
		    return(OPT);
		}
	    case INQUOTE:
		switch(c) {
		    case '\\':
			*w++ = rlgetc(token_stream);
			continue;
		    case '"':
			*w = '\0';
			if (debug) printf("returning QUOTE: %s \n", word);
			return(QUOTE);
		    default:
			*w++ = c;
			continue;
		}
	    case INWORD:
		if (!isalnum(c) && (c!='_') ) {
		    rl_ungetc(c,token_stream);
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
    if (token_stream != stdin) {
	xwin_display_set_state(D_ON);
	token_set_stream(stdin);
    } else {
	return(EOF);
    }
}

char *tok2str(token)
TOKEN token;
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
