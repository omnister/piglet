#include <ctype.h>

#include "db.h"
#include "xwin.h"
#include "token.h"
#include "lex.h"
#include "rlgetc.h"

#define POINT  0
#define REGION 1

/*
 *
 * IDE [:r xy1 xy2] | [[:p] xypnt] ... EOC
 *	highlight and print out details of rep under xypnt.
 *	  no refresh is done.
 * OPTIONS:
 *      :r identify by region (requires 2 points to define a region)
 *      :p identify by point (requires single coordinate)
 *
 * NOTE: default is pick by point.  However, if you change to pick
 *       by region with :r, you can go back to picking by point with
 *       the :p option.
 *
 */

static double x1, y1;

int com_identify(lp, layer)
LEXER *lp;
int *layer;
{
    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,END} state = START;

    int done=0;
    TOKEN token;
    char word[BUFSIZE];
    int debug=0;
    DB_DEFLIST *p_best;
    DB_DEFLIST *p_prev = NULL;
    double area;
    int mode=POINT;
    
    rl_saveprompt();
    rl_setprompt("IDENT> ");

    while (!done) {
	token = token_look(lp,word);
	if (debug) printf("got %s: %s\n", tok2str(token), word); 
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    if (token == OPT ) {
		token_get(lp,word); /* ignore for now */
		if (word[0]==':') {
		    switch (toupper(word[1])) {
		        case 'R':
			    mode = REGION;
			    break;
			case 'P':
			    mode = POINT;
			    break;
			default:
			    printf("IDENT: bad option: %s\n", word);
			    state=END;
			    break;
		    }
		} else {
		    printf("IDENT: bad option: %s\n", word);
		    state = END;
		}
		state = START;
	    } else if (token == NUMBER) {
		state = NUM1;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just eat it up */
		state = START;
	    } else if (token == EOC || token == CMD) {
		 state = END;
	    } else {
		token_err("IDENT", lp, "expected NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:		/* get pair of xy coordinates */
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &x1);	/* scan it in */
		state = COM1;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		state = END;	
	    } else {
		token_err("IDENT", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case COM1:		
	    if (token == EOL) {
		token_get(lp,word); /* just ignore it */
	    } else if (token == COMMA) {
		token_get(lp,word);
		state = NUM2;
	    } else {
		token_err("IDENT", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;
	case NUM2:
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y1);	/* scan it in */

		if (p_prev != NULL) {
		    db_highlight(p_prev);	/* unhighlight it */
		}

		if ((p_best=db_ident(currep, x1,y1,0, 0, 0, 0)) != NULL) {
		    db_highlight(p_best);	
		    db_notate(p_best);		/* print information */
		    area = db_area(p_best);
		    if (area >= 0) {
			printf("   area = %g\n", db_area(p_best));
		    }
		    p_prev=p_best;
		}

		state = NUM1;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("IDENT: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("IDENT", lp, "expected NUMBER", token);
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
    if (p_prev != NULL) {
	db_highlight(p_prev);	/* unhighlight any remaining component */
    }
    rl_restoreprompt();
    return(1);
}
