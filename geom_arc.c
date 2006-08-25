#include "db.h"
#include "token.h"
#include "lex.h"
#include "xwin.h"
#include "rubber.h"
#include "rlgetc.h"
#include "opt_parse.h"
#include <math.h>

DB_TAB dbtab; 
DB_DEFLIST dbdeflist;
DB_ARC dbarc;
void draw_arc(); 

OPTS opts;

double x1, yy1, x2, y2;

int add_arc(lp, layer)
LEXER *lp;
int *layer;
{
    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,NUM5,COM3,NUM6,END,ERR} state = START;

    int debug=0;
    int done=0;
    int nsegs;
    TOKEN token;
    char word[BUFSIZE];
    double x3, y3;
    static double xold, yold;

    if (debug) {printf("layer %d\n",*layer);}

    rl_saveprompt();
    rl_setprompt("ADD_ARC> ");

    opt_set_defaults(&opts);

    while (!done) {
	token = token_look(lp, word);
	if (debug) {printf("got %s: %s\n", tok2str(token), word);}
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	    case START:		/* get option or first xy pair */
		nsegs=0;
		if (debug) printf("in start\n");
		if (token == OPT ) {
		    token_get(lp,word); 
		    if (opt_parse(word, ARC_OPTS, &opts) == -1) {
		    	state = END;
		    } else {
			state = START;
		    }
		} else if (token == NUMBER) {
		    state = NUM1;
		} else if (token == EOL) {
		    token_get(lp,word); 	/* just eat it up */
		    state = START;
		} else if (token == EOC || token == CMD) {
		    state = END; 
		} else {
		    token_err("ARC", lp, "expected OPT or COORD", token);
		    state = END; 
		} 
		xold=yold=0.0;
		break;
	    case NUM1:		/* get pair of xy coordinates */
		if (debug) printf("in num1\n");
		if (token == NUMBER) {
		    token_get(lp,word);
		    xold=x1;
		    sscanf(word, "%lf", &x1);	/* scan it in */
		    state = COM1;
		} else if (token == EOL) {
		    token_get(lp,word); 	/* just ignore it */
		} else if (token == EOC || token == CMD) {
		    state = END; 
		} else {
		    token_err("ARC", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case COM1:		
		if (debug) printf("in com1\n");
		if (token == EOL) {
		    token_get(lp,word); /* just ignore it */
		} else if (token == COMMA) {
		    token_get(lp,word);
		    state = NUM2;
		} else {
		    token_err("ARC", lp, "expected COMMA", token);
		    state = END;	
		}
		break;
	    case NUM2:
		if (debug) printf("in num2\n");
		if (token == NUMBER) {
		    token_get(lp,word);
		    sscanf(word, "%lf", &yy1);	/* scan it in */
		    state = NUM3;
		} else if (token == EOL) {
		    token_get(lp,word); 	/* just ignore it */
		} else if (token == EOC || CMD) {
		    state = END; 
		} else {
		    token_err("ARC", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case NUM3:		/* get pair of xy coordinates */
		if (debug) printf("in num3\n");
		if (token == NUMBER) {
		    token_get(lp,word);
		    sscanf(word, "%lf", &x2);	/* scan it in */
		    state = COM2;
		} else if (token == EOL) {
		    token_get(lp,word); 	/* just ignore it */
		} else if (token == EOC || token == CMD) {
		    state = END; 
		} else {
		    token_err("ARC", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case COM2:		
		if (debug) printf("in com2\n");
		if (token == EOL) {
		    token_get(lp,word); 	/* just ignore it */
		} else if (token == COMMA) {
		    token_get(lp,word);
		    state = NUM4;
		} else if (token == EOL) {
		    token_get(lp,word); /* just ignore it */
		} else {
		    token_err("ARC", lp, "expected COMMA", token);
		    state = END;	
		}
		break;
	    case NUM4:
		if (debug) printf("in num4\n");
		if (token == NUMBER) {
		    token_get(lp,word);
		    sscanf(word, "%lf", &y2);	/* scan it in */
		    rubber_set_callback(draw_arc);
		    state = NUM5;
		} else if (token == EOL) {
		    token_get(lp,word); /* just ignore it */
		} else if (token == EOC || CMD) {
		    state = END; 
		} else {
		    token_err("ARC", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case NUM5:		/* get pair of xy coordinates */
		if (debug) printf("in num5\n");
		if (token == NUMBER) {
		    token_get(lp,word);
		    sscanf(word, "%lf", &x3);	/* scan it in */
		    state = COM3;
		} else if (token == EOL) {
		    token_get(lp,word); 	/* just ignore it */
		} else if (token == EOC || token == CMD) {
		    state = END; 
		} else {
		    token_err("ARC", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case COM3:		
		if (debug) printf("in com3\n");
		if (token == EOL) {
		    token_get(lp,word); 	/* just ignore it */
		} else if (token == COMMA) {
		    token_get(lp,word);
		    state = NUM6;
		} else if (token == EOL) {
		    token_get(lp,word); /* just ignore it */
		} else {
		    token_err("ARC", lp, "expected COMMA", token);
		    state = END;	
		}
		break;
	    case NUM6:
		if (debug) printf("in num4\n");
		if (token == NUMBER) {
		    token_get(lp,word);
		    sscanf(word, "%lf", &y3);	/* scan it in */
		    db_add_arc(currep, *layer, opt_copy(&opts), x1, yy1, x2, y2, x3, y3);
		    need_redraw++;
		    rubber_clear_callback();
		    state=START;
		} else if (token == EOL) {
		    token_get(lp,word); /* just ignore it */
		} else if (token == EOC || CMD) {
		    state = END; 
		} else {
		    token_err("ARC", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case END:
	    default:
		if (debug) printf("in end\n");
	    	rubber_clear_callback();
		if (token == EOC || token == CMD) {
		    ;
		} else {
		    token_flush_EOL(lp);
		}
		done++;
		break;
	}
    }
    rl_restoreprompt();
    return(1);
}

void draw_arc(x3, y3, count) 
double x3, y3;
int count; /* number of times called */
{
	static double x3old, y3old;
	int debug=0;
	BOUNDS bb;

	bb.init=0;


	/* DB_TAB dbtab;  */
	/* DB_DEFLIST dbdeflist; */
	/* DB_ARC dbarc; */

        dbtab.dbhead = &dbdeflist;
        dbtab.dbtail = &dbdeflist;
        dbtab.next = NULL;
	dbtab.name = "callback";

        dbdeflist.u.a = &dbarc;
        dbdeflist.type = ARC;

        dbarc.layer=1;
        dbarc.opts=&opts;
	dbarc.x1 = x1;
	dbarc.y1 = yy1;
	dbarc.x2 = x2;
	dbarc.y2 = y2;
	
	if (debug) {printf("in draw_arc\n");}

	if (count == 0) {		/* first call */
	    jump(&bb, D_RUBBER); /* draw new shape */
	    dbarc.x3 = x3;
	    dbarc.y3 = y3;
	    do_arc(&dbdeflist, &bb, D_RUBBER);

	} else if (count > 0) {		/* intermediate calls */
	    jump(&bb, D_RUBBER); /* erase old shape */
	    dbarc.x3 = x3old;
	    dbarc.y3 = y3old;
	    do_arc(&dbdeflist, &bb, D_RUBBER);
	    jump(&bb, D_RUBBER); /* draw new shape */
	    dbarc.x3 = x3;
	    dbarc.y3 = y3;
	    do_arc(&dbdeflist, &bb, D_RUBBER);
	} else {			/* last call, cleanup */
	    jump(&bb, D_RUBBER); /* erase old shape */
	    dbarc.x3 = x3old;
	    dbarc.y3 = y3old;
	    do_arc(&dbdeflist, &bb, D_RUBBER);
	}

	/* save old values */
	x3old=x3;
	y3old=y3;
	jump(&bb, D_RUBBER);
}

