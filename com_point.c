#include "db.h"
#include "xwin.h"
#include "token.h"
#include "lex.h"
#include "rlgetc.h"

/*
 *
 * POI xypnt ...
 *        cross is placed and xy val written above
 *	  no refresh is done.
 *
 */

static double x1, y1;

int com_point(LEXER *lp, char *arg)
{
    enum {START,NUM1,END} state = START;

    int done=0;
    TOKEN token;
    char *word;
    int debug=0;

    while (!done) {
	token = token_look(lp,&word);
	if (debug) printf("got %s: %s\n", tok2str(token), word); 
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    if (token == OPT ) {
		token_get(lp,&word); /* ignore for now */
		state = START;
	    } else if (token == NUMBER) {
		state = NUM1;
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just eat it up */
		state = START;
	    } else if (token == EOC || token == CMD) {
		 state = END;
	    } else {
		token_err("POINT", lp, "expected NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:
	    if (token == NUMBER) {
		if (getnum(lp, "POINT", &x1, &y1)) {
		    xwin_draw_point(x1, y1);
		    state = NUM1;
		} else {
		    state = END;
		}
	    } else if (token == EOL) {	
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("POINT: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("POINT", lp, "expected NUMBER", token);
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
