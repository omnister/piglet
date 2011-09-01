#include <string.h> 
#include <math.h>

#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "rlgetc.h"

#define MAXDBUF 20

/*
 *
 * DISTANCE xypnt1 xypnt2
 *        Computes and display both the orthogonal and diagonal distances
 *	  no refresh is done.
 *
 */

static double x1, yy1; 	/* cant call it "y1" because of math lib */
void draw_dist(); 

int com_distance(LEXER *lp, char *arg)
{
    enum {START,NUM1,NUM2,END} state = START;

    double x2,y2;

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
		token_err("DISTANCE", lp, "expected NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:
	    if (token==NUMBER) {
		if (getnum(lp, "DISTANCE", &x1, &yy1)) {
		    rubber_set_callback(draw_dist);
		    state = NUM2;
		} else {
		    state = END;
		}
	    } else if (token == EOL) {	
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("DISTANCE: cancelling DISTANCE\n");
	        state = END;
	    } else {
		token_err("DISTANCE", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case NUM2:
	    if (token==NUMBER) {
		if (getnum(lp, "DISTANCE", &x2, &y2)) {
		    printf("xy1=(%g,%g) xy2=(%g,%g) dx=%g, dy=%g, dxy=%g theta=%g (deg.)\n",
		    x1, yy1, x2, y2, fabs(x1-x2), fabs(yy1-y2),
		    sqrt(pow((x1-x2),2.0)+pow((yy1-y2),2.0)),
		    360.0*atan2(y2-yy1, x2-x1)/(2.0*M_PI));
		    rubber_clear_callback();
		    state = NUM1;
		} else {
		   state = END;
		}
	    } else if (token == EOL) {
		token_get(lp,&word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("DISTANCE: cancelling DISTANCE\n");
	        state = END;
	    } else {
		token_err("DISTANCE", lp, "expected NUMBER", token);
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

// #define RES 6 /* defined in db.h */

/* return nearest exact multiple of 1/(10^RES)) */
double grid(double num) {
   return (num-fmod(num,1.0/pow(10.0,RES)));
}

void draw_dist(double x2, double y2, int count) 
{
	static double x1old, x2old, y1old, y2old;
	static char dxbuf[MAXDBUF];
	static char dybuf[MAXDBUF];
	static char dxybuf[MAXDBUF];
	static char dxbufold[MAXDBUF];
	static char dybufold[MAXDBUF];
	static char dxybufold[MAXDBUF];
	BOUNDS bb;

	bb.init=0;

	snprintf(dxbuf,  MAXDBUF, "%g", grid(fabs(x1-x2)));
	snprintf(dybuf,  MAXDBUF, "%g", grid(fabs(yy1-y2))),
	snprintf(dxybuf, MAXDBUF, "%g", grid(sqrt(pow((x1-x2),2.0)+pow((yy1-y2),2.0))));

	if (count == 0) {		/* first call */
	    jump(&bb, D_RUBBER); /* draw new shape */
	    draw(x1,yy1, &bb, D_RUBBER);
	    draw(x2,y2, &bb, D_RUBBER);
	    draw(x2,yy1, &bb, D_RUBBER);
	    draw(x1,yy1, &bb, D_RUBBER);
	    xwin_draw_point(x1, yy1);
	    xwin_draw_point(x2, y2);
	    xwin_draw_text(x2, (yy1+2.0*y2)/3.0, dybuf);
	    xwin_draw_text((x1+x2)/2.0, yy1, dxbuf);
	    xwin_draw_text((2.0*x1+x2)/3.0, (2.0*yy1+y2)/3.0, dxybuf);
	} else if (count > 0) {		/* intermediate calls */
	    jump(&bb, D_RUBBER); /* erase old shape */
	    draw(x1old,y1old, &bb, D_RUBBER);
	    draw(x2old,y2old, &bb, D_RUBBER);
	    draw(x2old,y1old, &bb, D_RUBBER);
	    draw(x1old,y1old, &bb, D_RUBBER);
	    xwin_draw_point(x1old, y1old);
	    xwin_draw_point(x2old, y2old);
	    xwin_draw_text(x2old, (y1old+2.0*y2old)/3.0, dybufold);
	    xwin_draw_text((x1old+x2old)/2.0, y1old, dxbufold);
	    xwin_draw_text((2.0*x1old+x2old)/3.0, (2.0*y1old+y2old)/3.0, dxybufold);
	    jump(&bb, D_RUBBER); /* draw new shape */
	    draw(x1,yy1, &bb, D_RUBBER);
	    draw(x2,y2, &bb, D_RUBBER);
	    draw(x2,yy1, &bb, D_RUBBER);
	    draw(x1,yy1, &bb, D_RUBBER);
	    xwin_draw_text(x2, (yy1+2.0*y2)/3.0, dybuf);
	    xwin_draw_text((x1+x2)/2.0, yy1, dxbuf);
	    xwin_draw_text((2.0*x1+x2)/3.0, (2.0*yy1+y2)/3.0, dxybuf);
	    xwin_draw_point(x1, yy1);
	    xwin_draw_point(x2, y2);
	} else {			/* last call, cleanup */
	    jump(&bb, D_RUBBER); /* erase old shape */
	    draw(x1old,y1old, &bb, D_RUBBER);
	    draw(x2old,y2old, &bb, D_RUBBER);
	    draw(x2old,y1old, &bb, D_RUBBER);
	    draw(x1old,y1old, &bb, D_RUBBER);
	    xwin_draw_point(x1old, y1old);
	    xwin_draw_point(x2old, y2old);
	    xwin_draw_text(x2old, (y1old+2.0*y2old)/3.0, dybufold);
	    xwin_draw_text((x1old+x2old)/2.0, y1old, dxbufold);
	    xwin_draw_text((2.0*x1old+x2old)/3.0, (2.0*y1old+y2old)/3.0, dxybufold);
	}

	/* save old values */
	x1old=x1;
	y1old=yy1;
	x2old=x2;
	y2old=y2;
	strncpy(dxbufold, dxbuf, MAXDBUF);
	strncpy(dybufold, dybuf, MAXDBUF);
	strncpy(dxybufold, dxybuf, MAXDBUF);
	jump(&bb, D_RUBBER); 
}

