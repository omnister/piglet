#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "lex.h"

#define NORM   0	/* draw() modes */
#define RUBBER 1

static double x1, y1;

DB_TAB dbtab; 
DB_DEFLIST dbdeflist;
DB_CIRC dbcirc;

OPTS opts;


int add_circ(lp, layer)
LEXER *lp;
int *layer;
{
    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,END} state = START;

    int done=0;
    int error=0;
    TOKEN token;
    char word[BUFSIZE];
    double x2,y2;
    int debug=0;


    opt_set_defaults(&opts);
    opts.rotation = 2.0; 	/* default degrees resolution */
    opts.width = 0.0;		/* default width */

    if (debug) printf("layer %d\n",*layer);
    rl_setprompt("ADD_CIRCLE> ");

    while (!done) {
	token = token_look(lp, word);
	if (debug) printf("got %s: %s\n", tok2str(token), word); 
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	    case START:		/* get option or first xy pair */
		if (token == OPT ) {
		    token_get(lp, word); /* ignore for now */
		    /* FIXME: do bound checking on opts */
		    if (opt_parse(word, "WRY", &opts) == -1) {
		    	state = END;
		    } else {
			state = START;
		    }
		} else if (token == NUMBER) {
		    state = NUM1;
		} else if (token == EOL) {
		    token_get(lp, word); 	/* just eat it up */
		    state = START;
		} else if (token == EOC || token == CMD) {
		    state = END;	
		} else {
		    token_err("CIRCLE", lp, "expected OPT or NUMBER", token);
		    state = END;	/* error */
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
		    state = END; 
		} else {
		    token_err("CIRCLE", lp, "expected NUMBER", token);
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
		    token_err("CIRCLE", lp, "expected COMMA", token);
		    state = END;	
		}
		break;
	    case NUM2:
		if (token == NUMBER) {
		    token_get(lp, word);
		    sscanf(word, "%lf", &y1);	/* scan it in */
		    rubber_set_callback(draw_circle);
		    state = NUM3;
		} else if (token == EOL) {
		    token_get(lp, word); 	/* just ignore it */
		} else if (token == EOC || token == CMD) {
		    state = END; 
		} else {
		    token_err("CIRCLE", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case NUM3:		/* get pair of xy coordinates */
		if (token == NUMBER) {
		    token_get(lp, word);
		    sscanf(word, "%lf", &x2);	/* scan it in */
		    state = COM2;
		} else if (token == EOL) {
		    token_get(lp, word); 	/* just ignore it */
		} else if (token == EOC || token == CMD) {
		    state = END; 
		} else {
		    token_err("CIRCLE", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case COM2:		
		if (token == EOL) {
		    token_get(lp, word); 	/* just ignore it */
		} else if (token == COMMA) {
		    token_get(lp, word);
		    state = NUM4;
		} else if (token == EOL) {
		    token_get(lp, word); /* just ignore it */
		} else {
		    token_err("CIRCLE", lp, "expected COMMA", token);
		    state = END;	
		}
		break;
	    case NUM4:
		if (token == NUMBER) {
		    token_get(lp, word);
		    sscanf(word, "%lf", &y2);	/* scan it in */
		    state = START;
		    db_add_circ(currep, *layer, opt_copy(&opts), x1, y1, x2, y2);
		    rubber_clear_callback();
		    need_redraw++;
		} else if (token == EOL) {
		    token_get(lp, word); /* just ignore it */
		} else if (token == EOC || token == CMD) {
		    state = END; 
		} else {
		    token_err("CIRCLE", lp, "expected NUMBER", token);
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
	    	rubber_clear_callback();
		done++;
		break;
	}
    }
    return(1);
}

void draw_circle(x2, y2, count) 
double x2, y2;
int count; /* number of times called */
{
	static double x1old, x2old, y1old, y2old;
	int i;

	int debug=0;
	BOUNDS bb;

	bb.init=0;

        dbtab.dbhead = &dbdeflist;
        dbtab.dbtail = &dbdeflist;
        dbtab.next = NULL;
	dbtab.name = "callback";

        dbdeflist.u.c = &dbcirc;
        dbdeflist.type = CIRC;

        dbcirc.layer=1;
        dbcirc.x1 = x1; dbcirc.y1 = y1;
        dbcirc.opts = &opts;
	
	if (debug) {printf("in draw_circle\n");}

	if (count == 0) {		/* first call */
	    jump(); /* draw new shape */
	    dbcirc.x2 = x2; dbcirc.y2 = y2;
	    do_circ(&dbdeflist, &bb, 1);

	} else if (count > 0) {		/* intermediate calls */
	    jump(); /* erase old shape */
	    do_circ(&dbdeflist, &bb, 1);
	    jump(); /* draw new shape */
	    dbcirc.x2 = x2; dbcirc.y2 = y2;
	    do_circ(&dbdeflist, &bb, 1);
	} else {			/* last call, cleanup */
	    jump(); /* erase old shape */
	    do_circ(&dbdeflist, &bb, 1);
	}

	jump();
}

