/* FIXME: "^" function is only partially working and is quite flaky */
/* it doesn't back up until you move the mouse (?) and can eat up */
/* the last coord and cause a segmentation violation */

#include "db.h"
#include "token.h"
#include "lex.h"
#include "xwin.h"
#include "rubber.h"
#include <math.h>
#include <string.h>	/* for strncmp */
#include "rlgetc.h"
#include "opt_parse.h"
#include "lock.h"

static COORDS *CP;

DB_TAB dbtab; 
DB_DEFLIST dbdeflist;
DB_LINE dbline;
void draw_line(); 

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

OPTS opts;

double x1=0.0;
double yy1=0.0;

int add_line(LEXER *lp, int *layer)
{
    enum {START,NUM1,NUM2,END,ERR} state = START;

    int count;
    TOKEN token;
    char *word;
    double x2,y2;
    double xold, yold;
    int debug=0;
    int done=0;
    int nsegs=0;

    if (debug) {printf("layer %d\n",*layer);}
    rl_saveprompt();
    rl_setprompt("ADD_LINE> ");

    x2 = y2 = xold = yold = 0.0;
    opt_set_defaults(&opts);

    while (!done) {
	token = token_look(lp, &word);
	if (debug) {printf("got %s: %s\n", tok2str(token), word);}
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	    case START:		/* get option or first xy pair */
		db_checkpoint(lp);
		nsegs=0;
		if (debug) printf("in start\n");
		if (token == OPT ) {
		    token_get(lp,&word); 
		    if (opt_parse(word, LINE_OPTS, &opts) == -1) {
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
		    token_err("LINE", lp, "expected OPT or COORD", token);
		    state = END; 
		} 
		xold=yold=0.0;
		break;
	    case NUM1:		/* get pair of xy coordinates */
		xold=x1;
		yold=yy1;
		if (getnum(lp, "LINE", &x1, &yy1)) {
		    nsegs++;
		    
		    CP = coord_new(x1,yy1);
		
		    coord_append(CP, x1,yy1);
		    setlockpoint(x1,yy1);

		    if (debug) coord_print(CP);

		    rubber_set_callback(draw_line);
		    x2 = x1; y2 = yy1;
		    state = NUM2;
		} else if ((token=token_look(lp, &word)) == EOL) {
		    token_get(lp,&word); 	/* just ignore it */
		} else if (token == EOC || CMD) {
		    state = END; 
		} else {
		    token_err("LINE", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case NUM2:		/* get pair of xy coordinates */
		if (debug) printf("in num2\n");
		xold=x2;
		yold=y2;
		if (getnum(lp, "LINE", &x2, &y2)) {
		    nsegs++;

		    /* two identical clicks terminates this line */
		    /* but keeps the ADD L command in effect */

		    if (nsegs==1 && x1==x2 && yy1==y2) {
	    		rubber_clear_callback();
			printf("error: a line must have finite length\n");
			state = START;
		    } else if (x2==xold && y2==yold && nsegs) {
		    	if (debug) coord_print(CP);
			printf("dropping coord\n");
			coord_drop(CP);  /* drop last coord */
		    	if (debug) coord_print(CP);
			db_add_line(currep, *layer, opt_copy(&opts), CP);
			need_redraw++;
			rubber_clear_callback();
			state = START;
		    } else {
			rubber_clear_callback();
			if (debug) printf("doing append\n");

			/* only apply lock if this is an interactive edit */
			/* and not if reading from a file */

			if (strcmp(lp->name, "STDIN") == 0) {
			    lockpoint(&x2, &y2, currep->lock_angle); 
			}

			coord_swap_last(CP, x2, y2);
			coord_append(CP, x2,y2);
			setlockpoint(x2,y2);
			rubber_set_callback(draw_line);
			rubber_draw(x2,y2, 0);
			state = NUM2;	/* loop till EOC */
		    }
		} else if ((token=token_look(lp, &word)) == EOL) {
		    token_get(lp,&word); 	/* just ignore it */
		} else if (token == BACK) {
		    token_get(lp,&word); 	/* eat it */
		    if (debug) printf("dropping coord\n");
		    rubber_clear_callback(); 
		    rubber_draw(x2, y2, 0);
		    printf("num coords %d\n", coord_count(CP));
		    if ((count = coord_count(CP)) >= 3) {
			coord_drop(CP);  /* drop last coord */
			coord_swap_last(CP, x2, y2);
			coord_get(CP, count-2, &x2, &y2);
			setlockpoint(x2,y2);
		    } else {
		    	printf("can't drop last point!\n");
		    }
		    rubber_set_callback(draw_line); 
		    rubber_draw(x2, y2, 0);
		} else if (token == EOC || token == CMD) {
		    state = END; 
		} else {
		    token_err("LINE", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case END:
		if (debug) printf("in end\n");
		if (token == EOC && nsegs >= 2) {
			coord_drop(CP);  /* drop last coord */
			db_add_line(currep, *layer, opt_copy(&opts), CP);
			need_redraw++;
		    	; /* add geom */
		} else if (token == CMD) {
			/* should a CMD terminate a line? */
			;
		} else {
		    token_flush_EOL(lp);
		}
	    	rubber_clear_callback();
		done++;
		break;
	    default:
	    	break;
	}
    }
    rl_restoreprompt();
    return(1);
}


void draw_line(x2, y2, count) 
double x2, y2;
int count; /* number of times called */
{
	static double x1old, x2old, y1old, y2old;
	int debug=0;
	BOUNDS bb;
	static int called = 0;

	bb.init=0;


	/* DB_TAB dbtab;  */
	/* DB_DEFLIST dbdeflist; */
	/* DB_LINE dbline; */

	lockpoint(&x2, &y2, currep->lock_angle); 

        dbtab.dbhead = &dbdeflist;
        dbtab.next = NULL;
	dbtab.name = "callback";

        dbdeflist.u.l = &dbline;
        dbdeflist.type = LINE;

        dbline.layer=1;
        dbline.opts=&opts;
        dbline.coords=CP;
	
	if (debug) {printf("in draw_line\n");}

	if (count == 0) {		/* first call */
	    jump(&bb, D_RUBBER); /* draw new shape */
	    do_line(&dbdeflist, &bb, D_RUBBER);
	    called++;
	} else if (count > 0) {		/* intermediate calls */
	    jump(&bb, D_RUBBER); /* erase old shape */
	    do_line(&dbdeflist, &bb, D_RUBBER);
	    jump(&bb, D_RUBBER); /* draw new shape */
	    coord_swap_last(CP, x2, y2);
	    do_line(&dbdeflist, &bb, D_RUBBER);
	    called++;
	} else {			/* last call, cleanup */
	    if (called) {
		jump(&bb, D_RUBBER); /* erase old shape */
		do_line(&dbdeflist, &bb, D_RUBBER);
	    }
	    called = 0;
	}

	/* save old values */
	x1old=x1;
	y1old=yy1;
	x2old=x2;
	y2old=y2;
	jump(&bb, D_RUBBER);
}

