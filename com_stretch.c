#include <ctype.h>
#include <math.h>
#include <string.h>

#include "db.h"
#include "xwin.h"
#include "token.h"
#include "rubber.h"
#include "lex.h"
#include "rlgetc.h"
#include "lock.h"

#define POINT  0
#define REGION 1

/*
 *
 * STR [[<component>[<layer>]] [:P] { <xysel> <xyref> <xyloc> }...
 * STR [[<component>[<layer>]] :R { <xyll> <xyur> <xyref> <xyloc> }...
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
 *
 */


static double x1, yy1;
static double x2, y2;
static double x3, y3;
static double x4, y4;
void stretch_draw_box();
void stretch_draw_point();
SELPNT *selpnt;
SELPNT *tmp;

DB_DEFLIST *p_best;

int com_stretch(LEXER *lp, char *arg)
{
    enum {START,
          NUM1,	/* xysel, xyll: x1, yy1 */
	  NUM2,	/* xyur:        x2, y2  */
	  NUM3,	/* xyref:       x3, y3  */	
	  NUM4,	/* xyloc:       x4, y4  */
	  END} state = START;

    int done=0;
    TOKEN token;
    char word[BUFSIZE];
    int debug=0;
    DB_DEFLIST *p_prev = NULL;
    double *xmin, *xmax, *ymin, *ymax;
    double d, distance, dbest;
    COORDS *coords;
    double *xsel,*ysel;
    double *xselold,*yselold;
    double *xselfirst,*yselfirst;
    char instname[BUFSIZE];
    int i;

    char *pinst = (char *) NULL;
    int mode=POINT;
    int my_layer=0; 	/* internal working layer */
    int valid_comp=0;
    int comp=ALL;
    
    while (!done) {
	token = token_look(lp,word);
	if (debug) printf("state: %d got %s: %s\n", state, tok2str(token), word); 
	if (token==CMD) {
	    state=END;
	} 
	switch(state) {	
	case START:		/* get option or first xy pair */
	    db_checkpoint(lp);
	    if (token == OPT ) {
		token_get(lp,word); 
		if (word[0]==':') {
		    switch (toupper(word[1])) {
		        case 'R':
			    mode = REGION;
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
		    if (db_lookup(word)) {
		        strncpy(instname, word, BUFSIZE);
			pinst = instname;
		    } else {
			printf("not a valid instance name: %s\n", word);
			state = START;
		    }
		}
	    } else {
		token_err("STR", lp, "expected NUMBER", token);
		state = END;	/* error */
	    }
	    break;
	case NUM1:		/* get pair of xy coordinates */
            if (getnum(lp, "STRETCH", &x1, &yy1)) {
		if (p_prev != NULL) {
		    /* db_highlight(p_prev); */ /* unhighlight it */
		    p_prev = NULL;
		}
		if (mode == POINT) {
		    if ((p_best=db_ident(currep, x1,yy1,1, my_layer, comp, pinst)) != NULL) {
			db_highlight(p_best); 
			state = NUM3;
		    } else {
			printf("    nothing here to STRETCH, try SHO #E?\n");
			state = START;
		    }
		} else {			/* mode == REGION */
		    rubber_set_callback(stretch_draw_box);
		    state = NUM2;
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

	case NUM2:		/* get pair of xy coordinates */
            if (getnum(lp, "STRETCH", &x2, &y2)) {
		rubber_clear_callback();
		state = NUM3;
		selpnt=db_ident_region2(currep, x1,yy1, x2, y2, 2, my_layer, comp, pinst);

		if (selpnt == NULL) {
		    printf("Nothing here to wrap.  Try \"SHO #E\"?\n");
		    state = END;
		} else { 
		     tmp=selpnt;
		     while (tmp!=NULL) {
			if (tmp->p != NULL) {
			    db_highlight(tmp->p);
			}
			tmp = tmp->next;
		     }
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
            if (getnum(lp, "STRETCH", &x3, &y3)) {
		state = NUM4;

		if (mode == POINT) {
		    db_highlight(p_best);		/* unhighlight it */

		    switch (p_best->type) {

		    case ARC:

		        /* we are running through every point in 
			   the component looking for the one closest
			   to the pick point ... */

			xsel = xmin = &(p_best->u.a->x1);
			ysel = ymin = &(p_best->u.a->y1);
			dbest = d = dist(*xmin-x3, *ymin-y3);

			xmin = &(p_best->u.a->x2);
			ymin = &(p_best->u.a->y2);
			d = dist(*xmin-x3, *ymin-y3);
			if (d < dbest) { xsel = xmin; ysel = ymin; dbest = d; }

			xmin = &(p_best->u.a->x3);
			ymin = &(p_best->u.a->y3);
			d = dist(*xmin-x3, *ymin-y3);
			if (d < dbest) { xsel = xmin; ysel = ymin; dbest = d; }

			selpnt_clear(&selpnt);
			selpnt_save(&selpnt, xsel, ysel, NULL);
			rubber_set_callback(stretch_draw_point);

			state = NUM4;
			break;

		    case CIRC:

			xsel = &(p_best->u.c->x2);
			ysel = &(p_best->u.c->y2);
			selpnt_clear(&selpnt);
			selpnt_save(&selpnt, xsel, ysel, NULL);
			rubber_set_callback(stretch_draw_point);

			state = NUM4;
			break;

		    case INST:

			xsel = &(p_best->u.i->x);
			ysel = &(p_best->u.i->y);
			selpnt_clear(&selpnt);
			selpnt_save(&selpnt, xsel, ysel, NULL);
			rubber_set_callback(stretch_draw_point);

			state = NUM4;
			break;

		    case LINE:
			coords = p_best->u.l->coords;

			xsel = &(coords->coord.x);
			ysel = &(coords->coord.y);
			distance = dist((*xsel)-x3, (*ysel)-y3);
			dbest = distance;
			coords = coords->next;
			selpnt_clear(&selpnt);
			selpnt_save(&selpnt, xsel, ysel, NULL);

			while(coords != NULL) {
			    xselold = xsel;
			    yselold = ysel;
			    xsel = &(coords->coord.x);
			    ysel = &(coords->coord.y);

			    /* test midpoint */
			    distance = dist( ((*xsel+*xselold)/2.0)-x3,
					     ((*ysel+*yselold)/2.0)-y3 );
			    if (distance < dbest) {
				dbest = distance;
				selpnt_clear(&selpnt);
			        selpnt_save(&selpnt, xsel, ysel, NULL);
			        selpnt_save(&selpnt, xselold, yselold, NULL);
			    }

			    distance = dist(*xsel-x3, *ysel-y3);
			    if (distance < dbest) {
				dbest = distance;
				selpnt_clear(&selpnt);
			        selpnt_save(&selpnt, xsel, ysel, NULL);
			    }
			    coords = coords->next;
			}
			rubber_set_callback(stretch_draw_point);

			state = NUM4;
			break;

		    case NOTE:

			xsel = &(p_best->u.n->x);
			ysel = &(p_best->u.n->y);
			selpnt_clear(&selpnt);
			selpnt_save(&selpnt, xsel, ysel, NULL);
			rubber_set_callback(stretch_draw_point);

			state = NUM4;
			break;

		    case POLY:
			coords = p_best->u.p->coords;

			xselfirst = xsel = &(coords->coord.x);		/* first point */
			yselfirst = ysel = &(coords->coord.y);

			distance = dist((*xsel)-x3, (*ysel)-y3);
			dbest = distance;
			coords = coords->next;
			selpnt_clear(&selpnt);
			selpnt_save(&selpnt, xsel, ysel, NULL);

			while(coords != NULL) {
			    xselold = xsel;
			    yselold = ysel;
			    xsel = &(coords->coord.x);
			    ysel = &(coords->coord.y);

			    /* test midpoint */
			    distance = dist( ((*xsel+*xselold)/2.0)-x3,	/* mid points */
					     ((*ysel+*yselold)/2.0)-y3 );
			    if (distance < dbest) {
				dbest = distance;
				selpnt_clear(&selpnt);
				selpnt_save(&selpnt, xsel, ysel, NULL);
				selpnt_save(&selpnt, xselold, yselold, NULL);
			    }

			    distance = dist(*xsel-x3, *ysel-y3);		/* next points */
			    if (distance < dbest) {
				dbest = distance;
				selpnt_clear(&selpnt);
				selpnt_save(&selpnt, xsel, ysel, NULL);
			    }
			    coords = coords->next;
			}

			distance = dist( ((*xsel+*xselfirst)/2.0)-x3,	/* between first/last */
					 ((*ysel+*yselfirst)/2.0)-y3 );
			if (distance < dbest) {
			    dbest = distance;
			    selpnt_clear(&selpnt);
			    selpnt_save(&selpnt, xsel, ysel, NULL);
			    selpnt_save(&selpnt, xselfirst, yselfirst, NULL);
			}


			rubber_set_callback(stretch_draw_point);

			state = NUM4;
			break;

		    case RECT:
			xmin = &(p_best->u.r->x1);
			xmax = &(p_best->u.r->x2);
			ymin = &(p_best->u.r->y1);
			ymax = &(p_best->u.r->y2);

			/* check the eight sides + vertices */

			xsel = xmin, ysel = ymin;
			dbest = d = dist(*xmin-x3, *ymin-y3);

			d = dist(*xmin-x3, *ymax-y3);
			if (d < dbest) { xsel = xmin, ysel = ymax; dbest = d; }

			d = dist(*xmax-x3, *ymax-y3);
			if (d < dbest) { xsel = xmax, ysel = ymax; dbest = d; }

			d = dist(*xmax-x3, *ymin-y3);
			if (d < dbest) { xsel = xmax, ysel = ymin; dbest = d; }

			d = dist(*xmin-x3, ((*ymin+*ymax)/2.0)-y3);
			if (d < dbest) { xsel = xmin, ysel = NULL; dbest = d; }

			d = dist(*xmax-x3, ((*ymin+*ymax)/2.0)-y3);
			if (d < dbest) { xsel = xmax, ysel = NULL; dbest = d; }

			d = dist(((*xmin+*xmax)/2.0)-x3, *ymin-y3);
			if (d < dbest) { xsel = NULL, ysel = ymin; dbest = d; }

			d = dist(((*xmin+*xmax)/2.0)-x3, *ymax-y3);
			if (d < dbest) { xsel = NULL, ysel = ymax; dbest = d; }
			
			selpnt_clear(&selpnt);
			/* selpnt_save(xsel, ysel); */

			selpnt_save(&selpnt, xsel, ysel, NULL);

			rubber_set_callback(stretch_draw_point);

			/* xwin_draw_circle(*xsel, *ysel); */

			state = NUM4;
			break;

		    case TEXT:

			xsel = &(p_best->u.t->x);
			ysel = &(p_best->u.t->y);
			selpnt_clear(&selpnt);
			selpnt_save(&selpnt, xsel, ysel, NULL);
			rubber_set_callback(stretch_draw_point);

			state = NUM4;
			break;

		    default:
			printf("    not a stretchable object\n");
			db_notate(p_best);	/* print information */
			state = START;
			break;
		    }
		    /* p_prev=p_best; */
		    selpnt_save(&selpnt, NULL, NULL, p_best);
		} else { 			/* mode == REGION */
		     if (selpnt) {
			if (debug) {
			   printf("setting rubber callback\n");
			}
		        rubber_set_callback(stretch_draw_point);
		     } else {
			if (debug) {
			   printf("NOT setting rubber callback\n");
			}
		     }
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
	case NUM4:		/* get pair of xy coordinates */
            if (getnum(lp, "STRETCH", &x4, &y4)) {
		if (x3 == x4 && y3 == y4) {
		    stretch_draw_point(x3, y3, 0);
		}
		state = START;
		rubber_clear_callback();
		currep->modified++;
		need_redraw++;
	    } else if (token == EOL) {
		token_get(lp,word); 	/* just ignore it */
	    } else if (token == EOC || token == CMD) {
		printf("aborting STR\n");
		rubber_clear_callback();
		stretch_draw_point(x3, y3, 0);
	        state = END;
	    } else {
		token_err("STR", lp, "expected NUMBER", token);
		printf("aborting STR\n");
		rubber_clear_callback();
		stretch_draw_point(x3, y3, 0);
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
	    /* rubber_clear_callback(); */
	    break;
	}
    }
    return(1);
}

void stretch_draw_point(double xx, double yy, int count)
{
        static double xxold, yyold;
        BOUNDS bb;
        bb.init=0;
	int debug = 0;
	SELPNT *tmp;

	if (debug) {
	   printf("in stretch_draw_point: %g %g\n", xx, yy);
	}
        
	setlockpoint(x3, y3);
	lockpoint(&xx, &yy, currep->lock_angle);

        if (count == 0) {               /* first call */
	    for (tmp = selpnt; tmp != NULL; tmp = tmp->next) {
	        if (tmp->xsel != NULL) {
		   *(tmp->xsel) = tmp->xselorig + xx - x3;
		}
	        if (tmp->ysel != NULL) {
		   *(tmp->ysel) = tmp->yselorig + yy - y3;
		}
		if (tmp->p != NULL) {
		    db_highlight(tmp->p);
		}
	    }
        } else if (count > 0) {         /* intermediate calls */
	    for (tmp = selpnt; tmp != NULL; tmp = tmp->next) {
	        if (tmp->xsel != NULL) {
		   *(tmp->xsel) = tmp->xselorig + xxold - x3;
		}
	        if (tmp->ysel != NULL) {
		   *(tmp->ysel) = tmp->yselorig + yyold - y3;
		}
		if (tmp->p != NULL) {
		    db_highlight(tmp->p);
		}
	    }
	    for (tmp = selpnt; tmp != NULL; tmp = tmp->next) {
	        if (tmp->xsel != NULL) {
		   *(tmp->xsel) = tmp->xselorig + xx - x3;
		}
	        if (tmp->ysel != NULL) {
		   *(tmp->ysel) = tmp->yselorig + yy - y3;
		}
		if (tmp->p != NULL) {
		    db_highlight(tmp->p);
		}
	    }
        } else {                        /* last call, cleanup */
	    for (tmp = selpnt; tmp != NULL; tmp = tmp->next) {
	        if (tmp->xsel != NULL) {
		   *(tmp->xsel) = tmp->xselorig + xxold - x3;
		}
	        if (tmp->ysel != NULL) {
		   *(tmp->ysel) = tmp->yselorig + yyold - y3;
		}
		if (tmp->p != NULL) {
		    db_highlight(tmp->p);
		}
	    }
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
        static double x1old, x3old, y1old, y3old;
        BOUNDS bb;
        bb.init=0;
	static int called=0;

        if (count == 0) {               /* first call */
            db_drawbounds(x1,yy1,x3,y3,D_RUBBER);                /* draw new shape */
	    called++;
        } else if (count > 0) {         /* intermediate calls */
            db_drawbounds(x1old,y1old,x3old,y3old,D_RUBBER);    /* erase old shape */
            db_drawbounds(x1,yy1,x3,y3,D_RUBBER);                /* draw new shape */
        } else {                        /* last call, cleanup */
	    if (called) {
		db_drawbounds(x1old,y1old,x3old,y3old,D_RUBBER);    /* erase old shape */
	    }
	    called=0;
        }

        /* save old values */
        x1old=x1;
        y1old=yy1;
        x3old=x3;
        y3old=y3;
        jump(&bb, D_RUBBER);
}

