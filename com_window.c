#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "lex.h"
#include "rlgetc.h"

static double x1, y1;
int fit=0;		/* don't fit */
int nest=9;		/* default nesting level */
int bounds=0;		/* default pick boundary display level */

OPTS opts;

int com_window(lp, arg)
LEXER *lp;
char *arg;
{
    enum {START,NUM1,COM1,NUM2,NUM3,COM2,NUM4,END} state = START;

    extern int fit, nest;
    double scale=1.0;	/* default scale */
    
    double dx, dy, xmin, ymin, xmax, ymax, tmp;
    double x2, y2;
    int n;

    TOKEN token;
    int done=0;
    int error=0;
    char word[BUFSIZE];
    int debug=0;

    if (debug) printf("com_window\n");
    rl_saveprompt();
    rl_setprompt("WIN> ");

    opt_set_defaults(&opts);

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
			if (scale > 10.0 || scale < 0.10) {
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
		} else if (strncasecmp(word, ":B", 2) == 0) {
		    if(sscanf(word+2, "%d", &bounds) != 1) {
		        weprintf("WIN invalid bound level %s\n", word+2);
		        state=END;
		    } else {
			db_set_bounds(bounds);
		    }
		} else {
	    	     printf("WIN: bad option: %s\n", word);
		     state = END;
		}
		break;
	    } else if (token == NUMBER) {
		state = NUM1;
	    } else if (token == EOL) {
		token_get(lp, word); 	/* just eat it up */
		state = START;
	    } else if (token == EOC || token == CMD) {
		 do_win(0, 0.0, 0.0, 0.0, 0.0, scale);
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
	    if (token == NUMBER) {
		token_get(lp, word);
		sscanf(word, "%lf", &x1);	/* scan it in */
		state = COM1;
	    } else if (token == EOL) {
		token_get(lp, word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("   cancelling WIN\n");
		state = END;	
	    } else {
		printf("WIN: expected NUMBER, got: %s\n",
		    tok2str(token));
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
		printf("WIN: expected COMMA: got: %s!\n", 
		    tok2str(token));
	        state = END;
	    }
	    break;
	case NUM2:
	    if (token == NUMBER) {
		token_get(lp, word);
		sscanf(word, "%lf", &y1);	/* scan it in */
		rubber_set_callback(draw_bounds);
		state = NUM3;
	    } else if (token == EOL) {
		token_get(lp, word); 	/* just ignore it */
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
	    if (token == NUMBER) {
		token_get(lp, word);
		sscanf(word, "%lf", &x2);	/* scan it in */
		state = COM2;
	    } else if (token == EOL) {
		token_get(lp, word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		if (debug) printf("WIN: doing pan\n");
		do_win(2, x1, y1, 0.0, 0.0, scale);
		fit = 0;
		scale = 1;
	        state = END;
	    } else {
		printf("WIN: expected COORD, got: %s\n",
		    tok2str(token));
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
		printf("WIN: expected COMMA, got:%s\n", 
		    tok2str(token));
		state = END;	
	    }
	    break;
	case NUM4:
	    if (token == NUMBER) {
		state = START;
		token_get(lp, word);
		sscanf(word, "%lf", &y2);	/* scan it in */
		rubber_clear_callback();
		if (x1==x2 && y1==y2) {
		    do_win(2, x1, y1, 0.0, 0.0, scale);  /* pan */
		    fit = 0;
		    scale = 1;
		} else {
		    do_win(4, x1, y1, x2, y2, scale);    /* zoom */
		    fit = 0;
		    scale = 1;
		}
	    } else if (token == EOL) {
		token_get(lp, word); /* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("WIN: cancelling WIN\n");
		state = END; 
	    } else {
		printf("WIN: expected NUMBER, got: %s\n",
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
    rl_restoreprompt();
    return(1);
}

do_win(int n, double x1, double y1, double x2, double y2, double scale) {

    extern int fit, nest;
    double dx, dy, xmin, ymin, xmax, ymax, tmp;
    int debug=0;

    xwin_window_get(&xmin, &ymin, &xmax, &ymax);

    if (debug) printf("in do_win, %d %g %g %g %g %g\n",
    	n, x1, y1, x2, y2, scale);

    if (n==2) {
	dx=(xmax-xmin);
	dy=(ymax-ymin);
	xmin=x1-dx/2.0;
	ymin=y1-dx/2.0;
	xmax=x1+dx/2.0;
	ymax=y1+dx/2.0;
    } else if (n==4) {
    	xmin=x1;
	ymin=y1;
	xmax=x2;
	ymax=y2;
    } else if (n==0) {
	xwin_window_get(&xmin, &ymin, &xmax, &ymax);
    } else {
	eprintf("WIN: bad number of points");
	return(0);
    }

    if (debug) printf("setting xmin, ymin, xmax, ymax %g %g %g %g\n",
    	xmin, ymin, xmax, ymax);

    if (fit) {
	db_bounds(&xmin, &ymin, &xmax, &ymax);
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
	if (debug) printf("%doing scale \n");
	dx=(xmax-xmin);
	dy=(ymax-ymin);
	xmin=((xmax+xmin)/2.0)-((dx*scale)/2.0);
	xmax=((xmax+xmin)/2.0)+((dx*scale)/2.0);
	ymin=((ymax+ymin)/2.0)-((dy*scale)/2.0);
	ymax=((ymax+ymin)/2.0)+((dy*scale)/2.0);
    } 

    if (debug) printf("%g %g %g %g\n", xmin, ymin, xmax, ymax);

    if (xmin == 0 && ymin == 0 && xmax == 0 && ymax == 0) {
	;	/* don't set the window to a null size */
    } else {
	xwin_window_set(xmin,ymin,xmax,ymax);
    }
    

}

void draw_bounds(x2, y2, count) 
double x2, y2;
int count; /* number of times called */
{
	static double x1old, x2old, y1old, y2old;
	BOUNDS bounds;

	if (count == 0) {		/* first call */
	    jump(); /* draw new shape */
	    draw(x1,y1, &bounds, D_RUBBER);
	    draw(x1,y2, &bounds, D_RUBBER);
	    draw(x2,y2, &bounds, D_RUBBER);
	    draw(x2,y1, &bounds, D_RUBBER);
	    draw(x1,y1, &bounds, D_RUBBER);

	} else if (count > 0) {		/* intermediate calls */

	    jump(); /* erase old shape */
	    draw(x1old,y1old, &bounds, D_RUBBER);
	    draw(x1old,y2old, &bounds, D_RUBBER);
	    draw(x2old,y2old, &bounds, D_RUBBER);
	    draw(x2old,y1old, &bounds, D_RUBBER);
	    draw(x1old,y1old, &bounds, D_RUBBER);

	    jump(); /* draw new shape */
	    draw(x1,y1, &bounds, D_RUBBER);
	    draw(x1,y2, &bounds, D_RUBBER);
	    draw(x2,y2, &bounds, D_RUBBER);
	    draw(x2,y1, &bounds, D_RUBBER);
	    draw(x1,y1, &bounds, D_RUBBER);

	} else {			/* last call, cleanup */
	    jump(); /* erase old shape */
	    draw(x1old,y1old, &bounds, D_RUBBER);
	    draw(x1old,y2old, &bounds, D_RUBBER);
	    draw(x2old,y2old, &bounds, D_RUBBER);
	    draw(x2old,y1old, &bounds, D_RUBBER);
	    draw(x1old,y1old, &bounds, D_RUBBER);
	}

	/* save old values */
	x1old=x1;
	y1old=y1;
	x2old=x2;
	y2old=y2;
	jump();
}

