#include "db.h"
#include "xwin.h"
#include "token.h"
#include "lex.h"
#include <math.h>

/*
 *
 * DISTANCE xypnt1 xypnt2
 *        Computes and display both the orthogonal and diagonal distances
 *	  no refresh is done.
 *
 */


int com_distance(lp, layer)
LEXER *lp;
int *layer;
{
    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,END} state = START;

    int npairs=0;
    double xold,yold;
    double x1, y1;

    int done=0;
    int error=0;
    TOKEN token;
    char word[BUFSIZE];
    int debug=0;

    if (debug) printf("layer %d\n",*layer);
    rl_setprompt("DISTANCE> ");

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
		state = START;
	    } else if (token == NUMBER) {
		state = NUM1;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just eat it up */
		state = START;
	    } else if (token == EOC || token == CMD) {
		 state = END;
	    } else {
		token_err("DISTANCE", lp, "expected NUMBER", token);
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
		token_err("DISTANCE", lp, "expected NUMBER", token);
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
		token_err("DISTANCE", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;
	case NUM2:
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y1);	/* scan it in */
		npairs++; 
		xwin_draw_point(x1, y1);
		if (npairs >=2) {
		   printf("(%g,%g) (%g,%g) dx=%g, dy=%g, dxy=%g\n",
		   xold, yold, x1, y1, fabs(x1-xold), fabs(y1-yold),
		   sqrt(pow((x1-xold),2.0)+pow((y1-yold),2.0)));
		}
		xold=x1; yold=y1;
		state = NUM1;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("POINT: cancelling DISTANCE\n");
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
