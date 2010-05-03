#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "rlgetc.h"
#include "opt_parse.h"

/*
 *
 *        +--------------+---------<--+---------------+
 *        v              v            ^               |
 * (ADD)--0---(R)------+-1------------+--|xy|---|xy|----|prim|->
 *        |            | |            |                  |cmd |
 *        +-(R)<layer>-+ +-(:W<width>-+
 *                       |            |
 *                       +-(:FILL)----+
 */

static double x1, y1;
void draw_box();

OPTS opts;

int add_rect(LEXER *lp, int *layer)
{
    enum {START,NUM1,NUM2,END} state = START;

    int done=0;
    TOKEN token;
    char *word;
    double x2,y2;
    int debug=0;

    if (debug) printf("layer %d\n",*layer);
    rl_saveprompt();
    rl_setprompt("ADD_RECT> ");

    opt_set_defaults(&opts);

    while (!done) {
	token = token_look(lp,&word);
	if (debug) printf("got %s: %s\n", tok2str(token), word); 
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    db_checkpoint(lp);
	    if (token == OPT ) {
		token_get(lp,&word); 
		if (opt_parse(word, RECT_OPTS, &opts) == -1) {
		    state = END;
		} else {
		    state = START;
		}
	    } else if (token == NUMBER) {
		state = NUM1;
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just eat it up */
		state = START;
	    } else if (token == EOC || token == CMD) {
		 state = END;
	    } else {
		token_err("RECT", lp, "expected OPT or NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:		/* get pair of xy coordinates */
	    if (token == NUMBER) {
		if (getnum(lp, "RECT", &x1, &y1)) {
		    rubber_set_callback(draw_box);
		    state = NUM2;
		} else {
		    state = END;	
		}
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("   cancelling ADD RECT\n");
		state = END;	
	    } else {
		token_err("RECT", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case NUM2:		/* get pair of xy coordinates */
	    if (token == NUMBER) {
		if (getnum(lp, "RECT", &x2, &y2)) {
		    state = START;
		    if (x1 == x2 || y1 == y2) {
			printf("    Can't add a rectangle of zero width or height: %g %g %g %g\n", 
			    x1, y1, x2, y2);
		    } else {
			db_add_rect(currep, *layer, opt_copy(&opts), x1, y1, x2, y2);
		    }
		    rubber_clear_callback();
		    need_redraw++; 
		} else {
		    state = END; 
		}
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("ADD RECT: cancelling ADD RECT\n");
	        state = END;
	    } else {
		token_err("RECT", lp, "expected NUMBER", token);
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
    rubber_clear_callback();
    rl_restoreprompt();
    return(1);
}

void draw_box(x2, y2, count) 
double x2, y2;
int count; /* number of times called */
{
	static double x1old, x2old, y1old, y2old;
	BOUNDS bb;
	bb.init=0;
	
	if (count == 0) {		/* first call */
	    db_drawbounds(x1,y1,x2,y2,D_RUBBER); 		/* draw new shape */
	} else if (count > 0) {		/* intermediate calls */
	    db_drawbounds(x1old,y1old,x2old,y2old,D_RUBBER);    /* erase old shape */
	    db_drawbounds(x1,y1,x2,y2,D_RUBBER); 		/* draw new shape */
	} else {			/* last call, cleanup */
	    db_drawbounds(x1old,y1old,x2old,y2old,D_RUBBER);    /* erase old shape */
	}

	/* save old values */
	x1old=x1;
	y1old=y1;
	x2old=x2;
	y2old=y2;
	jump(&bb, D_RUBBER);
}


