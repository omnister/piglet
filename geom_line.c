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

static double x1, yy1;
static double xsnap, ysnap;

void setlockpoint(x,y) 
double x, y;
{
    int debug = 0;

    if (debug) printf("setting snap to %g %g\n", x, y);

    xsnap = x;
    ysnap = y;
}

void lockpoint(px, py, lock) 
double *px, *py;
double lock;
{
    double dx, dy;
    double theta;
    double locktheta;
    double snaptheta;
    double radius;

    if (lock != 0.0) {

	dx = *px - xsnap;
	dy = *py - ysnap;
	radius = sqrt(dx*dx + dy*dy);

	/* compute angle (range is +/- M_PI) */
	theta = atan2(dy,dx);
	if (theta < 0.0) {
	    theta += 2.0*M_PI;
	}

	locktheta = 2.0*M_PI*lock/360.0;
	snaptheta = locktheta*floor((theta+(locktheta/2.0))/locktheta);

	/*
	printf("angle = %g, locked %g\n", 
		theta*360.0/(2.0*M_PI), snaptheta*360.0/(2.0*M_PI));
	fflush(stdout);
	*/ 

    	/* overwrite px, py, to produce vector with same radius, but proper theta */
	*px = xsnap+radius*cos(snaptheta);
	*py = ysnap+radius*sin(snaptheta);

	snapxy(px, py);	   /* snap computed points to grid */
    }
}


int add_line(lp, layer)
LEXER *lp;
int *layer;
{
    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,END,ERR} state = START;

    int debug=0;
    int done=0;
    int count;
    int nsegs;
    TOKEN token;
    char word[BUFSIZE];
    double x2,y2;
    static double xold, yold;

    if (debug) {printf("layer %d\n",*layer);}
    rl_saveprompt();
    rl_setprompt("ADD_LINE> ");

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
		    if (opt_parse(word, LINE_OPTS, &opts) == -1) {
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
		    token_err("LINE", lp, "expected OPT or COORD", token);
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
		    token_err("LINE", lp, "expected NUMBER", token);
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
		    token_err("LINE", lp, "expected COMMA", token);
		    state = END;	
		}
		break;
	    case NUM2:
		if (debug) printf("in num2\n");
		if (token == NUMBER) {
		    token_get(lp,word);
		    yold=y2;
		    sscanf(word, "%lf", &yy1);	/* scan it in */
		    nsegs++;
		    
		    CP = coord_new(x1,yy1);
		
		    coord_append(CP, x1,yy1);
		    setlockpoint(x1,yy1);

		    if (debug) coord_print(CP);

		    rubber_set_callback(draw_line);
		    state = NUM3;
		} else if (token == EOL) {
		    token_get(lp,word); 	/* just ignore it */
		} else if (token == EOC || CMD) {
		    state = END; 
		} else {
		    token_err("LINE", lp, "expected NUMBER", token);
		    state = END; 
		}
		break;
	    case NUM3:		/* get pair of xy coordinates */
		if (debug) printf("in num3\n");
		if (token == NUMBER) {
		    token_get(lp,word);
		    xold=x2;
		    sscanf(word, "%lf", &x2);	/* scan it in */
		    state = COM2;
		} else if (token == EOL) {
		    token_get(lp,word); 	/* just ignore it */
		} else if (token == BACK) {
		    token_get(lp,word); 	/* eat it */
		    if (debug) printf("dropping coord\n");
		    rubber_clear_callback(); 
		    rubber_draw(x2, y2);
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
		    rubber_draw(x2, y2);
		} else if (token == EOC || token == CMD) {
		    state = END; 
		} else {
		    token_err("LINE", lp, "expected NUMBER", token);
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
		    token_err("LINE", lp, "expected COMMA", token);
		    state = END;	
		}
		break;
	    case NUM4:
		if (debug) printf("in num4\n");
		if (token == NUMBER) {
		    token_get(lp,word);
		    yold=y2;
		    sscanf(word, "%lf", &y2);	/* scan it in */

		    nsegs++;

		    /* two identical clicks terminates this line */
		    /* but keeps the ADD L command in effect */

		    if (nsegs==1 && x1==x2 && yy1==y2) {
	    		rubber_clear_callback();
			printf("error: a line must have finite length\n");
			state = START;
		    } else if (x2==xold && y2==yold) {
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
			rubber_draw(x2,y2);
			state = NUM3;	/* loop till EOC */
		    }

		} else if (token == EOL) {
		    token_get(lp,word); /* just ignore it */
		} else if (token == EOC || CMD) {
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
	BOUNDS bb;

	bb.init=0;

	int debug=0;

	/* DB_TAB dbtab;  */
	/* DB_DEFLIST dbdeflist; */
	/* DB_LINE dbline; */

	lockpoint(&x2, &y2, currep->lock_angle); 

        dbtab.dbhead = &dbdeflist;
        dbtab.dbtail = &dbdeflist;
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

	} else if (count > 0) {		/* intermediate calls */
	    jump(&bb, D_RUBBER); /* erase old shape */
	    do_line(&dbdeflist, &bb, D_RUBBER);
	    jump(&bb, D_RUBBER); /* draw new shape */
	    coord_swap_last(CP, x2, y2);
	    do_line(&dbdeflist, &bb, D_RUBBER);
	} else {			/* last call, cleanup */
	    jump(&bb, D_RUBBER); /* erase old shape */
	    do_line(&dbdeflist, &bb, D_RUBBER);
	}

	/* save old values */
	x1old=x1;
	y1old=yy1;
	x2old=x2;
	y2old=y2;
	jump(&bb, D_RUBBER);
}

