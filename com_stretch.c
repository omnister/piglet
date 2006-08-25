#include <ctype.h>
#include <math.h>
#include <string.h>

#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "lex.h"
#include "rlgetc.h"

#define POINT  0
#define REGION 1

/*
 *
 * STR [[<component>[<layer>]] [:P] { <xysel> <xyref> <xyloc> }...
 * STR [[<component>[<layer>]] :R { <xysel> <xyll> <xyur> <xyref> <xyloc> }...
 *
 *  classic piglet:
 *
 *    STRETCH [descriptor] xysel xyselref xyloc
 *    STRETCH [descriptor] :G xyfrom xyto xyselref [xyselref ...]
 *    STRETCH [descriptor] :W xyfrom xyto xyll xyur [xyll xyur ...]
 *    STRETCH [descriptor] :V xysel xyedge xyloc
 *
 *      :p stretch by point (default mode: requires single coordinate) 
 *      :r stretch by region (requires 2 points to define a region)
 *
 * NOTE: default is stretch by point.  However, if you change to stretch
 *       by region with :r, you can later go back to stretch by point with
 *       the :p option.
 *
 */

/* 
 *
 * need a list for holding 
 * struct selpnt {
 *    double *xsel;
 *    double *ysel;
 *    double xselorig;
 *    double yselorig;
 *    selpnt *next;
 * }
 *
 * selpnt *getpnts(p_best, x4, y4, x2, y2, PNT|REGION);
 *
 */

static double x1, yy1;
static double x2, y2;
static double x3, y3;
static double x4, y4;
static double x5, y5;
void stretch_draw_box();
void stretch_draw_point();
STACK *stack;

double *xsel;
double *ysel;
double xsel_orig;
double ysel_orig;
DB_DEFLIST *p_best;

int com_stretch(LEXER *lp, char *arg)
{
    enum {START,
          NUM1,COM1,NUM2,	/* xysel: x1, yy1 */
	  NUM3,COM2,NUM4,	/* xll:   x2, y2  */
	  NUM5,COM3,NUM6,       /* xur:   x3, y3  */
	  NUM7,COM4,NUM8,	/* xyref: x4, y4  */	
	  NUM9,COM5,NUM10,	/* xyloc: x5, y5  */
	  END} state = START;

    int done=0;
    TOKEN token;
    char word[BUFSIZE];
    int debug=0;
    DB_DEFLIST *p_prev = NULL;
    double xmin, xmax, ymin, ymax;
    int mode=POINT;
    double distance, dbest;
    COORDS *coords;
    double xx, yy;

    int my_layer=0; 	/* internal working layer */
    int valid_comp=0;
    int i;

    int comp=ALL;
    
    rl_saveprompt();
    rl_setprompt("STR> ");

    while (!done) {
	token = token_look(lp,word);
	if (debug) printf("got %s: %s\n", tok2str(token), word); 
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    if (token == OPT ) {
		token_get(lp,word); 
		if (word[0]==':') {
		    switch (toupper(word[1])) {
		        case 'R':
			    /* mode = REGION; */
			    /* ignore for now */
			    break;
			case 'P':
			    mode = POINT;
			    break;
			default:
			    printf("STR: bad option: %s\n", word);
			    state=END;
			    break;
		    } 
		} else {
		    printf("STR: bad option: %s\n", word);
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
	    } else if (token == IDENT) {
		token_get(lp,word);
	    	state = NUM1;
		/* check to see if is a valid comp descriptor */
		valid_comp=0;
		if ((comp = is_comp(toupper(word[0])))) {
		    if (strlen(word) == 1) {
			valid_comp++;	/* no layer given */
		    } else {
			valid_comp++;
			/* check for any non-digit characters */
			/* to allow instance names like "L1234b" */
			for (i=0; i<strlen(&word[1]); i++) {
			    if (!isdigit(word[1+i])) {
				valid_comp=0;
			    }
			}
			if (valid_comp) {
			    if(sscanf(&word[1], "%d", &my_layer) == 1) {
				if (debug) printf("given layer=%d\n",my_layer);
			    } else {
				valid_comp=0;
			    }
			} 
			if (valid_comp) {
			    if (my_layer > MAX_LAYER) {
				printf("layer must be less than %d\n",
				    MAX_LAYER);
				valid_comp=0;
				done++;
			    }
			    if (!show_check_modifiable(currep, comp, my_layer)) {
				printf("layer %d is not modifiable!\n",
				    my_layer);
				token_flush_EOL(lp);
				valid_comp=0;
				done++;
			    }
			}
		    }
		} else { 
		    /* here need to handle a valid cell name */
		    printf("looks like a descriptor to me: %s\n", word);
		}
	    } else {
		token_err("STR", lp, "expected NUMBER", token);
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
		token_err("STR", lp, "expected NUMBER", token);
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
		token_err("STR", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;
	case NUM2:
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &yy1);	/* scan it in */

		if (p_prev != NULL) {
		    /* db_highlight(p_prev); */ /* unhighlight it */
		    p_prev = NULL;
		}
		if ((p_best=db_ident(currep, x1,yy1,1, my_layer, comp, 0)) != NULL) {
		    db_highlight(p_best); 
		} else {
		    state = START;
		}

		if (mode==POINT) {
		    state = NUM7;
		} else {			/* mode == REGION */
		    state = NUM3;
		}

	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("STR: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("STR", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;

	case NUM3:		/* get pair of xy coordinates */
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &x2);	/* scan it in */
		state = COM2;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		state = END;	
	    } else {
		token_err("STR", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case COM2:		
	    if (token == EOL) {
		token_get(lp,word); /* just ignore it */
	    } else if (token == COMMA) {
		token_get(lp,word);
		state = NUM4;
	    } else {
		token_err("STR", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;
	case NUM4:
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y2);	/* scan it in */
		rubber_set_callback(stretch_draw_box);
		state = NUM5;

	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("STR: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("STR", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case NUM5:		/* get pair of xy coordinates */
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &x3);	/* scan it in */
		state = COM3;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		state = END;	
	    } else {
		token_err("STR", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case COM3:		
	    if (token == EOL) {
		token_get(lp,word); /* just ignore it */
	    } else if (token == COMMA) {
		token_get(lp,word);
		state = NUM6;
	    } else {
		token_err("STR", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;
	case NUM6:
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y3);	/* scan it in */
		state = NUM7;
		rubber_clear_callback();
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("STR: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("STR", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case NUM7:		/* get pair of xy coordinates */
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &x4);	/* scan it in */
		state = COM4;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		state = END;	
	    } else {
		token_err("STR", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case COM4:		
	    if (token == EOL) {
		token_get(lp,word); /* just ignore it */
	    } else if (token == COMMA) {
		token_get(lp,word);
		state = NUM8;
	    } else {
		token_err("STR", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;

	case NUM8:
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y4);	/* scan it in */
		state = NUM9;
		db_highlight(p_best);		/* unhighlight it */

		switch (p_best->type) {

		case CIRC:

		    xsel = &(p_best->u.c->x2);
		    ysel = &(p_best->u.c->y2);
		    xsel_orig = *xsel;
		    ysel_orig = *ysel;
		    rubber_set_callback(stretch_draw_point);

		    state = NUM9;
		    break;

		case INST:

		    xsel = &(p_best->u.i->x);
		    ysel = &(p_best->u.i->y);
		    xsel_orig = *xsel;
		    ysel_orig = *ysel;
		    rubber_set_callback(stretch_draw_point);

		    state = NUM9;
		    break;

		case LINE:
		    coords = p_best->u.l->coords;

		    xx = coords->coord.x;
		    yy = coords->coord.y;
		    distance = dist(xx-x4, yy-y4);
		    dbest = distance;
		    xsel = &(coords->coord.x);
		    ysel = &(coords->coord.y);
		    coords = coords->next;

		    while(coords != NULL) {
			xx = coords->coord.x;
			yy = coords->coord.y;
			distance = dist(xx-x4, yy-y4);
			if (distance < dbest) {
			    xsel = &(coords->coord.x);
			    ysel = &(coords->coord.y);
			    dbest = distance;
			}
			coords = coords->next;
		    }
		    xsel_orig = *xsel;
		    ysel_orig = *ysel;
		    rubber_set_callback(stretch_draw_point);
		    /* xwin_draw_circle(xsel_orig, ysel_orig); */

		    state = NUM9;
		    break;

		case NOTE:

		    xsel = &(p_best->u.n->x);
		    ysel = &(p_best->u.n->y);
		    xsel_orig = *xsel;
		    ysel_orig = *ysel;
		    rubber_set_callback(stretch_draw_point);

		    state = NUM9;
		    break;
		    
		case POLY:
		    coords = p_best->u.p->coords;

		    xx = coords->coord.x;
		    yy = coords->coord.y;
		    distance = dist(xx-x4, yy-y4);
		    dbest = distance;
		    xsel = &(coords->coord.x);
		    ysel = &(coords->coord.y);
		    coords = coords->next;

		    while(coords != NULL) {
			xx = coords->coord.x;
			yy = coords->coord.y;
			distance = dist(xx-x4, yy-y4);
			if (distance < dbest) {
			    xsel = &(coords->coord.x);
			    ysel = &(coords->coord.y);
			    dbest = distance;
			}
			coords = coords->next;
		    }
		    xsel_orig = *xsel;
		    ysel_orig = *ysel;
		    rubber_set_callback(stretch_draw_point);
		    /* xwin_draw_circle(xsel_orig, ysel_orig); */

		    state = NUM9;
		    break;


		case RECT:
		    xmin = p_best->u.r->x1;
		    xmax = p_best->u.r->x2;
		    ymin = p_best->u.r->y1;
		    ymax = p_best->u.r->y2;

		    if (fabs(xmin-x4) < fabs(xmax-x4)) {
		       xsel = &(p_best->u.r->x1);
		    } else {
		       xsel = &(p_best->u.r->x2);
		    }

		    if (fabs(ymin-y4) < fabs(ymax-y4)) {
		       ysel = &(p_best->u.r->y1);
		    } else {
		       ysel = &(p_best->u.r->y2);
		    }

		    xsel_orig = *xsel;
		    ysel_orig = *ysel;
		    rubber_set_callback(stretch_draw_point);

		    /* xwin_draw_circle(*xsel, *ysel); */

		    state = NUM9;
		    break;

		case TEXT:

		    xsel = &(p_best->u.t->x);
		    ysel = &(p_best->u.t->y);
		    xsel_orig = *xsel;
		    ysel_orig = *ysel;
		    rubber_set_callback(stretch_draw_point);

		    state = NUM9;
		    break;

		default:
		    printf("    not a stretchable object\n");
		    db_notate(p_best);	/* print information */
		    state = START;
		    break;
		}

		p_prev=p_best;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("STR: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("STR", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case NUM9:		/* get pair of xy coordinates */
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &x5);	/* scan it in */
		state = COM5;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		state = END;	
	    } else {
		token_err("STR", lp, "expected NUMBER", token);
		state = END; 
	    }
	    break;
	case COM5:		
	    if (token == EOL) {
		token_get(lp,word); /* just ignore it */
	    } else if (token == COMMA) {
		token_get(lp,word);
		state = NUM10;
	    } else {
		token_err("STR", lp, "expected COMMA", token);
	        state = END;
	    }
	    break;
	case NUM10:
	    if (token == NUMBER) {
		token_get(lp,word);
		sscanf(word, "%lf", &y5);	/* scan it in */
		state = START;
		rubber_clear_callback();
		need_redraw++;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("STR: cancelling POINT\n");
	        state = END;
	    } else {
		token_err("STR", lp, "expected NUMBER", token);
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
	    rubber_clear_callback();
	    break;
	}
    }
    if (p_prev != NULL) {
	db_highlight(p_prev);	/* unhighlight any remaining component */
    }
    rl_restoreprompt();
    return(1);
}

void stretch_draw_point(double xx, double yy, int count)
{
        static double xxold, yyold;
        BOUNDS bb;
        bb.init=0;

        if (count == 0) {               /* first call */
	    *xsel = xsel_orig+xx-x4;
	    *ysel = ysel_orig+yy-y4;
	    xwin_draw_circle(*xsel, *ysel);
	    db_highlight(p_best);
        } else if (count > 0) {         /* intermediate calls */
	    *xsel = xsel_orig+xxold-x4;
	    *ysel = ysel_orig+yyold-y4;
	    xwin_draw_circle(*xsel, *ysel);
	    db_highlight(p_best);	/* erase old shape */
	    *xsel = xsel_orig+xx-x4;
	    *ysel = ysel_orig+yy-y4;
	    xwin_draw_circle(*xsel, *ysel);
	    db_highlight(p_best);	/* draw new shape */
        } else {                        /* last call, cleanup */
	    *xsel = xsel_orig+xxold-x4;
	    *ysel = ysel_orig+yyold-y4;
	    xwin_draw_circle(*xsel, *ysel);
	    db_highlight(p_best);	/* erase old shape */
        }

        /* save old values */
        xxold=xx;
        yyold=yy;
        jump(&bb, D_RUBBER);
}

void stretch_draw_box(x3, y3, count)
double x3, y3;
int count; /* number of times called */
{
        static double x2old, x3old, y2old, y3old;
        BOUNDS bb;
        bb.init=0;

        if (count == 0) {               /* first call */
            db_drawbounds(x2,y2,x3,y3,D_RUBBER);                /* draw new shape */
        } else if (count > 0) {         /* intermediate calls */
            db_drawbounds(x2old,y2old,x3old,y3old,D_RUBBER);    /* erase old shape */
            db_drawbounds(x2,y2,x3,y3,D_RUBBER);                /* draw new shape */
        } else {                        /* last call, cleanup */
            db_drawbounds(x2old,y2old,x3old,y3old,D_RUBBER);    /* erase old shape */
        }

        /* save old values */
        x2old=x2;
        y2old=y2;
        x3old=x3;
        y3old=y3;
        jump(&bb, D_RUBBER);
}

