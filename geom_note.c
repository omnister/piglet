#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "opt_parse.h"

#define NORM   0	/* draw() modes */
#define RUBBER 1

static double x1, y1;

OPTS opts;

/* [:Mmirror] [:Rrot] [:Yyxratio] [:Zslant] [:Fsize] "string" xy EOC" */

int add_note(lp, layer)
LEXER *lp;
int *layer;
{
    enum {START,NUM1,COM1,NUM2,END} state = START;

    int done=0;
    int error=0;
    TOKEN token;
    char word[BUFSIZE];
    char str[BUFSIZE];
    double x2,y2;
    int debug=0;

    DB_TAB *ed_rep;

    opt_set_defaults( &opts );

    rl_setprompt("ADD_NOTE> ");
    str[0] = 0;

    while (!done) {
	token = token_look(lp, word);

	if (debug) printf("got %s: %s\n", tok2str(token), word); 

	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	    case START:		/* get option or first xy pair */
		if (token == OPT ) {
		    token_get(lp, word); 
		    if (opt_parse(word, "MRYZF", &opts) == -1) {
			state = END;
		    } else {
			state = START;
		    }
		} else if (token == NUMBER) {
		    state = NUM1;
		} else if (token == QUOTE) {
		    token_get(lp,str); 
		    state = START;
		} else if (token == EOL) {
		    token_get(lp, word); 	/* just eat it up */
		    state = START;
		} else if (token == EOC || token == CMD) {
		    state = END;	/* error */
		} else {
		    token_err("NOTE", lp, "expected OPT or NUMBER", token);
		    state = END; 
		}
		break;
	    case NUM1:		/* get pair of xy coordinates */
		if (token == NUMBER) {
		    token_get(lp, word);
		    sscanf(word, "%lf", &x1);	/* scan it in */
		    state = COM1;
		} else if (token == EOL) {
		    token_get(lp, word); 	/* just ignore it */
		} else if (token == EOC || token == CMD) {
		    printf(" cancelling ADD NOTE\n");
		    state = END; 
		} else {
		    token_err("NOTE", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case COM1:		
		if (token == EOL) {
		    token_get(lp, word); /* just ignore it */
		} else if (token == COMMA) {
		    token_get(lp, word);
		    state = NUM2;
		} else {
		    token_err("NOTE", lp, "expected COMMA", token);
		    state = END;	
		}
		break;
	    case NUM2:
		if (token == NUMBER) {
		    token_get(lp, word);
		    sscanf(word, "%lf", &y1);	/* scan it in */

		    if (strlen(str) == 0) {
		    	printf("ADD NOTE: no string given\n");
			state = START;
		    } else {
			db_add_note(currep, *layer, opt_copy(&opts),
			    strsave(str), x1, y1);
			need_redraw++;
			state = START;
	            }
		} else if (token == EOL) {
		    token_get(lp, word); 	/* just ignore it */
		} else if (token == EOC || token == CMD) {
		    printf(" cancelling ADD NOTE\n");
		    state = END; 
		} else {
		    token_err("NOTE", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case END:
	    default:
		if (token == EOC || token == CMD) {
			;
		} else {
		    token_flush_EOL(lp);
		}
		done++;
		break;
	}
    }
    return(1);
}

