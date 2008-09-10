#include <strings.h>

#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "lex.h"
#include "rlgetc.h"
#include "eprintf.h"

static double x1, y1;
int fit=0;		/* don't fit */
int nest=9;		/* default nesting level */
int lastwin=0;		/* go back to last window */
int bounds=0;		/* default pick boundary display level */

OPTS opts;
void draw_bounds();
int do_win();

int com_window(LEXER *lp, char *arg)
{
    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,END} state = START;

    double scale=1.0;	/* default scale */
    extern int fit, nest;
    
    TOKEN token;
    int done=0;
    char *word;
    int debug=0;
    double x2, y2;

    if (debug) printf("com_window\n");

    opt_set_defaults(&opts);

    while (!done) {
	token = token_look(lp, &word);
	if (debug) printf("got %s: %s\n", tok2str(token), word); 
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    if (token == OPT ) {
		token_get(lp, &word); 
		state = START;
		if (strncasecmp(word, ":F", 2) == 0) {
		     fit++;
		} else if (strncasecmp(word, ":X", 2) == 0) {
		    if(sscanf(word+2, "%lf", &scale) != 1) {
		        weprintf("WIN invalid scale argument: %s\n", word+2);
			state = END;
		    } else {
			if (scale < 0.0) {
			    scale = 1.0/(-scale);
			}
			if (scale > 100.0 || scale < 0.01) {
			    weprintf("WIN: scale out of range %s\n", word+2);
			    state = END;
			}
		    }
		} else if (strncasecmp(word, ":N", 2) == 0) {
		    if(sscanf(word+2, "%d", &nest) != 1) {
		        weprintf("WIN invalid nest level %s\n", word+2);
		        state=END;
		    } else {
			db_set_nest(nest);
		    }
		} else if (strncasecmp(word, ":O", 2) == 0) {
			db_set_fill(FILL_TOGGLE);
		} else if (strncasecmp(word, ":Z", 2) == 0) {
			lastwin++;
		} else {
	    	     printf("WIN: bad option: %s\n", word);
		     state = END;
		}
		break;
	    } else if (token == NUMBER) {
		state = NUM1;
	    } else if (token == EOL) {
		token_get(lp, &word); 	/* just eat it up */
		state = START;
	    } else if (token == EOC || token == CMD) {
		 token_get(lp, &word); 
		 if (debug) printf("calling do_win 1 inside com window\n");
		 do_win(lp, 0, 0.0, 0.0, 0.0, 0.0, scale);
		 fit = 0;
		 scale = 1;
		 state = END;
	    } else {
		printf("WIN: expected OPT or COORD, got: %s\n",
		    tok2str(token));
		state = END;	/* error */
	    }
	    break;
	case NUM1:		/* get pair of xy coordinates */
            if (getnum(lp, "WIN", &x1, &y1)) {
		rubber_set_callback(draw_bounds);
		state = NUM3;
	    } else if (token == EOL) {
		token_get(lp, &word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("WIN: cancelling WIN\n");
	        state = END;
	    } else {
		printf("WIN: expected NUMBER, got: %s\n",
		    tok2str(token));
		state = END; 
	    }
	    break;
	case NUM3:		/* get pair of xy coordinates */
            if (getnum(lp, "WIN", &x2, &y2)) {
		state = START;
		token_get(lp, &word);
		sscanf(word, "%lf", &y2);	/* scan it in */
		rubber_clear_callback();
		if (x1==x2 && y1==y2) {
		     if (debug) printf("calling do_win 3 inside com window\n");
		    do_win(lp, 2, x1, y1, 0.0, 0.0, scale);  /* pan */
		    fit = 0;
		    scale = 1;
		} else {
		    if (debug) printf("calling do_win 4 inside com window\n");
		    do_win(lp, 4, x1, y1, x2, y2, scale);    /* zoom */
		    fit = 0;
		    scale = 1;
		}
	    } else if (token == EOL) {
		token_get(lp, &word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		if (debug) printf("WIN: doing pan\n");
		if (debug) printf("calling do_win 2 inside com window\n");
		do_win(lp, 2, x1, y1, 0.0, 0.0, scale);
		fit = 0;
		scale = 1;
	        state = END;
	    } else {
		printf("WIN: expected COORD, got: %s\n",
		    tok2str(token));
		state = END; 
	    }
	    break;
	case END:
	default:
	    if (token == EOC) {
		token_flush_EOL(lp);	/* don't leave a dangling EOL */
	    } else if (token == CMD) {
		;
	    } else {
		token_flush_EOL(lp);
	    }
	    done++;
	    break;
	}
    }
    rubber_clear_callback();
    return(1);
}

int do_win(LEXER *lp, int n, double x1, double y1, double x2, double y2, double scale) {

    extern int fit, nest;
    double dx, dy, xmin, ymin, xmax, ymax, tmp;
    int debug=0;

    if (currep != NULL) {
	xmin = currep->vp_xmin;
	ymin = currep->vp_ymin;
	xmax = currep->vp_xmax;
	ymax = currep->vp_ymax;
	if (debug) printf("setting xmin inside do_win() = %g\n", xmin);
    } else {
	xmin = -100.0;
	ymin = -100.0;
	xmax =  100.0;
	ymax =  100.0;
    }

    if (debug) printf("in do_win, %d %g %g %g %g %g\n",
    	n, x1, y1, x2, y2, scale);

    if (n==2) {
	dx=(xmax-xmin);
	dy=(ymax-ymin);
	xmin=x1-dx/2.0;
	ymin=y1-dx/2.0;
	xmax=x1+dx/2.0;
	ymax=y1+dx/2.0;
	if (debug) printf("setting xmin inside do_win() = %g\n", xmin);

    } else if (n==4) {		/* how cell bounds get loaded during readin */
	if (currep != NULL) {
	    // currep->vp_xmin=x1;
	    // currep->vp_xmax=x2;
	    // currep->vp_ymin=y1;
	    // currep->vp_ymax=y2;

	    currep->minx=x1;
	    currep->maxx=x2;
	    currep->miny=y1;
	    currep->maxy=y2;
	}
    	xmin=x1;
	ymin=y1;
	xmax=x2;
	ymax=y2;
	if (debug) printf("setting currep->xmin inside do_win() = %g %g %g %g\n", xmin, ymin, xmax, ymax);
    } else if (n==0) {
    	;
    } else {
	eprintf("WIN: bad number of points");
	return(0);
    }

    if (debug) printf("setting xmin, ymin, xmax, ymax %g %g %g %g\n",
    	xmin, ymin, xmax, ymax);

    if (fit) {
	if (currep != NULL ) {
	    xmin = currep->minx;
	    ymin = currep->miny;
	    xmax = currep->maxx;
	    ymax = currep->maxy;
	} else {
	    xmin = -100.0;
	    ymin = -100.0;
	    xmax = 100.0;
	    ymax = 100.0;
	}
    }

    if (debug) printf("%g %g %g %g %d\n", xmin, ymin, xmax, ymax, n);

    if (xmax < xmin) {		/* canonicalize the selection rectangle */
	tmp = xmax; xmax = xmin; xmin = tmp;
    }
    if (ymax < ymin) {
	tmp = ymax; ymax = ymin; ymin = tmp;
    }

    if (fit) {
	if (debug) printf("doing fit \n");
	dx=(xmax-xmin);
	dy=(ymax-ymin);
	xmin-=dx/40.0;
	xmax+=dx/40.0;
	ymin-=dy/40.0;
	ymax+=dy/40.0;
    } 

    if (scale != 1.0) {
	if (debug) printf("doing scale \n");
	dx=(xmax-xmin)/2.0;
	dy=(ymax-ymin)/2.0;
	xmin=((xmax+xmin)/2.0)-(dx*scale);
	xmax=((xmax+xmin)/2.0)+(dx*scale);
	ymin=((ymax+ymin)/2.0)-(dy*scale);
	ymax=((ymax+ymin)/2.0)+(dy*scale);
    } 

    if (currep != NULL && lastwin) {
        if (debug) printf("setting old values\n");
	xmin = currep->old_xmin;
	ymin = currep->old_ymin; 
	xmax = currep->old_xmax;
	ymax = currep->old_ymax;
    }

    if (currep != NULL) {
        if (debug) printf("saving old values values\n");
	currep->old_xmin = currep->vp_xmin;
	currep->old_ymin = currep->vp_ymin;
	currep->old_xmax = currep->vp_xmax;
	currep->old_ymax = currep->vp_ymax;

	currep->vp_xmin=xmin;
	currep->vp_ymin=ymin;
	currep->vp_xmax=xmax;
	currep->vp_ymax=ymax;
    }

    if (debug) printf("%g %g %g %g\n", xmin, ymin, xmax, ymax);

    if (xmin == 0 && ymin == 0 && xmax == 0 && ymax == 0) {
	;	/* don't set the window to a null size */
    } else {
	if (debug) printf("calling xwin_window_set = %g %g %g %g\n",
	    xmin, ymin, xmax, ymax);
	xwin_window_set(xmin,ymin,xmax,ymax);
    }
    if (debug) printf("leaving dowin()\n");
    return(0);
}

void draw_bounds(x2, y2, count) 
double x2, y2;
int count; /* number of times called */
{
	static double x1old, x2old, y1old, y2old;
	static int called = 0;
	BOUNDS bb;

	if (count == 0) {		/* first call */
	    jump(&bb, D_RUBBER); /* draw new shape */
	    draw(x1,y1, &bb, D_RUBBER);
	    draw(x1,y2, &bb, D_RUBBER);
	    draw(x2,y2, &bb, D_RUBBER);
	    draw(x2,y1, &bb, D_RUBBER);
	    draw(x1,y1, &bb, D_RUBBER);
	    called++;

	} else if (count > 0) {		/* intermediate calls */

	    jump(&bb, D_RUBBER); /* erase old shape */
	    draw(x1old,y1old, &bb, D_RUBBER);
	    draw(x1old,y2old, &bb, D_RUBBER);
	    draw(x2old,y2old, &bb, D_RUBBER);
	    draw(x2old,y1old, &bb, D_RUBBER);
	    draw(x1old,y1old, &bb, D_RUBBER);

	    jump(&bb, D_RUBBER); /* draw new shape */
	    draw(x1,y1, &bb, D_RUBBER);
	    draw(x1,y2, &bb, D_RUBBER);
	    draw(x2,y2, &bb, D_RUBBER);
	    draw(x2,y1, &bb, D_RUBBER);
	    draw(x1,y1, &bb, D_RUBBER);
	    called++;

	} else {			/* last call, cleanup */
	    if (called) {
		jump(&bb, D_RUBBER); /* erase old shape */
		draw(x1old,y1old, &bb, D_RUBBER);
		draw(x1old,y2old, &bb, D_RUBBER);
		draw(x2old,y2old, &bb, D_RUBBER);
		draw(x2old,y1old, &bb, D_RUBBER);
		draw(x1old,y1old, &bb, D_RUBBER);
	    }
	    called=0;
	}

	/* save old values */
	x1old=x1;
	y1old=y1;
	x2old=x2;
	y2old=y2;
	jump(&bb, D_RUBBER); 
}

